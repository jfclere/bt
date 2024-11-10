/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "os/mynewt.h"
#include "console/console.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "bleenv_sens.h"

#include "sysinit/sysinit.h"
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"

#include <defs/error.h>
#include <sensor/sensor.h>
#include "sensor/temperature.h"
#include "sensor/humidity.h"
#include "sensor/pressure.h"
#include <console/console.h>

#include "bus/bus.h"

#include <bme280/bme280.h>

#define LISTENER_CB 1
#define READ_CB 2
#define MY_SENSOR_DEVICE "bme280"
#define MY_SENSOR_POLL_TIME 2000

/* For the LED logic */
int g_led_pin;

/* value stored locally for BLE use */
float temp;
float press;
float humid;
/* getters for the values */
float get_temp()
{
    return temp;
}
float get_press()
{
    return press;
}
float get_humid()
{
    return humid;
}

static int
read_bme280_temp(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type)
{
    struct sensor_temp_data *std = databuf;
    // console_printf("read_bme280!!!\n");
    // console_printf("Temperature=%lf (valid %d)\n", std->std_temp, std->std_temp_is_valid);
    temp = std->std_temp;
    return 0;
}

static int
read_bme280_press(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type)
{
    struct sensor_press_data *std = databuf;
    // console_printf("Pressure=%lf (valid %d)\n", std->spd_press, std->spd_press_is_valid);
    press = std->spd_press;
    return 0;
}

static int
read_bme280_hum(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type)
{
    struct sensor_humid_data *std = databuf;
    // console_printf("Humidity=%lf (valid %d)\n", std->shd_humid, std->shd_humid_is_valid);
    humid  = std->shd_humid;
    return 0;
}

static struct sensor_listener listener_temp = {
   .sl_sensor_type = SENSOR_TYPE_AMBIENT_TEMPERATURE,
   .sl_func = read_bme280_temp,
   .sl_arg = (void *)LISTENER_CB,
};

static struct sensor_listener listener_press = {
   .sl_sensor_type = SENSOR_TYPE_PRESSURE,
   .sl_func = read_bme280_press,
   .sl_arg = (void *)LISTENER_CB,
};

static struct sensor_listener listener_hum = {
   .sl_sensor_type = SENSOR_TYPE_RELATIVE_HUMIDITY,
   .sl_func = read_bme280_hum,
   .sl_arg = (void *)LISTENER_CB,
};

static const struct bus_i2c_node_cfg g_bme280_i2c_node_cfg = {
    .node_cfg = {
        .bus_name = MYNEWT_VAL(BME280_NODE_BUS_NAME),
    },
    .addr = MYNEWT_VAL(BME280_NODE_I2C_ADDRESS),
    .freq = MYNEWT_VAL(BME280_NODE_I2C_FREQUENCY),
};
static struct bme280 bme280;
static struct sensor *bme280_sensor;
static struct sensor_itf g_bme280_sensor_itf;

static int
bme280_sensor_configure(void)
{
    struct os_dev *dev;
    struct bme280_cfg cfg;
    int rc;

    dev = os_dev_open("bme280", OS_TIMEOUT_NEVER, NULL);
    if (dev == NULL)
        return -1;
    assert(dev != NULL);

    memset(&cfg, 0, sizeof(cfg));

    cfg.bc_mode = BME280_MODE_FORCED;
    cfg.bc_iir = BME280_FILTER_OFF;
    cfg.bc_sby_dur = BME280_STANDBY_MS_0_5;
    cfg.bc_boc[0].boc_type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    cfg.bc_boc[1].boc_type = SENSOR_TYPE_PRESSURE;
    cfg.bc_boc[2].boc_type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
    cfg.bc_boc[0].boc_oversample = BME280_SAMPLING_X1;
    cfg.bc_boc[1].boc_oversample = BME280_SAMPLING_X1;
    cfg.bc_boc[2].boc_oversample = BME280_SAMPLING_X1;
    cfg.bc_s_mask = SENSOR_TYPE_AMBIENT_TEMPERATURE|
                       SENSOR_TYPE_PRESSURE|
                       SENSOR_TYPE_RELATIVE_HUMIDITY;

    rc = bme280_config((struct bme280 *)dev, &cfg);
    if (rc)
        return rc;

    os_dev_close(dev);
    return 0;
}

/* Connection handle */
static uint16_t conn_handle;

static uint8_t blecsc_addr_type;

/* Advertised device name  */

#ifdef MYNEWT_VAL_BLE_SENSOR_NAME
static const char *device_name = MYNEWT_VAL(BLE_SENSOR_NAME);
#else
static const char *device_name = "changeit";
#endif

/* Variable holds current env measurement state */
static struct ble_env_measurement_state env_measurement_state;

static int blecsc_gap_event(struct ble_gap_event *event, void *arg);


/*
 * Enables advertising with parameters:
 *     o General discoverable mode
 *     o Undirected connectable mode
 */
static void
blecsc_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    /*
     * Set appearance.
     */
    fields.appearance = ble_svc_gap_device_appearance();
    fields.appearance_is_present = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(blecsc_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, blecsc_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

static int
blecsc_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        console_printf("BLE_GAP_EVENT_CONNECT\n");
        hal_gpio_write(g_led_pin, 1);
        /* A new connection was established or a connection attempt failed */
        MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

        if (event->connect.status != 0) {
            /* Connection failed; resume advertising */
            blecsc_advertise();
            conn_handle = 0;
        }
        else {
          conn_handle = event->connect.conn_handle;
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        console_printf("BLE_GAP_EVENT_DISCONNECT\n");
        hal_gpio_write(g_led_pin, 0);
        MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
        conn_handle = 0;
        /* Connection terminated; resume advertising */
        blecsc_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "adv complete\n");
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        console_printf("BLE_GAP_EVENT_SUBSCRIBE %d\n", event->subscribe.attr_handle);
        MODLOG_DFLT(INFO, "subscribe event attr_handle=%d\n",
                    event->subscribe.attr_handle);

        /* XXX: we have event->subscribe.attr_handle = 9 */
        break;

    case BLE_GAP_EVENT_MTU:
        console_printf("BLE_GAP_EVENT_MTU\n");
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.value);
        break;

    }

    return 0;
}

static void
blecsc_on_sync(void)
{
    int rc;

    console_printf("blecsc_on_sync\n");
    /* Figure out address to use while advertising (no privacy) */
    rc = ble_hs_id_infer_auto(0, &blecsc_addr_type);
    assert(rc == 0);

    /* Begin advertising */
    blecsc_advertise();
}

/*
 * Do the bme280 logic
 *
 * @return 0 on success, non-zero on failure
 */
static int my_sensor()
{
    struct os_dev *dev;
    int rc;

   /* sensor part */
    rc = bme280_create_i2c_sensor_dev(&bme280.i2c_node, MY_SENSOR_DEVICE,
                                      &g_bme280_i2c_node_cfg,
                                      &g_bme280_sensor_itf);
    console_printf("my_sensor_app rc = %d\n", rc);
    if (rc)
        return rc;
    dev = os_dev_open(MY_SENSOR_DEVICE, 0, NULL);
    if (dev == NULL) {
        console_printf("my_sensor_app dev == NULL\n");
        return -1;
    } else
        console_printf("my_sensor_app dev YES!!!\n");

    rc = bme280_sensor_configure();
    if (rc) {
        console_printf("my_sensor_app bme280_sensor_configure FAILED\n");
        return rc;
    } else {
        console_printf("my_sensor_app bme280_sensor_configure DONE\n");
    }

    bme280_sensor = sensor_mgr_find_next_bydevname(MY_SENSOR_DEVICE, NULL);

    if (bme280_sensor == NULL) {
        console_printf("my_sensor_app sensor_mgr_find_next_bydevname NULL\n");
        return -2;
    }

    rc = sensor_register_listener(bme280_sensor, &listener_temp);
    console_printf("sensor_register_listener %d\n", rc);
    if (rc)
        return rc;

    rc = sensor_register_listener(bme280_sensor, &listener_press);
    console_printf("sensor_register_listener %d\n", rc);
    if (rc)
        return rc;

    rc = sensor_register_listener(bme280_sensor, &listener_hum);
    console_printf("sensor_register_listener %d\n", rc);
    if (rc)
        return rc;

    rc = sensor_set_poll_rate_ms(MY_SENSOR_DEVICE, MY_SENSOR_POLL_TIME);
    console_printf("my_sensor_app %d\n", rc);
    return rc;
}

/*
 * main
 *
 * The main task for the project. This function initializes the packages,
 * then starts serving events from default event queue.
 *
 * @return int NOTE: this function should never return!
 */
int
mynewt_main(int argc, char **argv)
{
    int rc;

    /* Initialize OS */
    sysinit();

    g_led_pin = LED_BLINK_PIN;
    hal_gpio_init_out(g_led_pin, 0);

    console_printf("bleenv starting!!!\n");

    /* sensor part */
    rc = my_sensor();
    if (rc) {
        console_printf("sensor part failed!!!\n");
    }
    /* restart until we have a sensor */
    assert(rc == 0);

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.sync_cb = blecsc_on_sync;

    /* Initialize gatt service */
    rc = gatt_svr_init(&env_measurement_state);
    console_printf("bleenv gatt_svr_init %d!!!\n", rc);
    assert(rc == 0);

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(device_name);
    console_printf("bleenv gble_svc_gap_device_name_set %d to %s!!!\n", rc, device_name);
    assert(rc == 0);

    /* As the last thing, process events from default event queue */
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    return 0;
}

