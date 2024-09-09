#include "sysinit/sysinit.h"
#include "os/os.h"

#include <defs/error.h>
#include <sensor/sensor.h>
#include "sensor/temperature.h"
#include "sensor/humidity.h"
#include "sensor/pressure.h"
#include <console/console.h>

#include "bus/bus.h"
/*
#include "bus/drivers/i2c_common.h"
#include "bus/drivers/spi_common.h"
 */

#include <bme280/bme280.h>

/**
 * Depending on the type of package, there are different
 * compilation rules for this directory.  This comment applies
 * to packages of type "app."  For other types of packages,
 * please view the documentation at http://mynewt.apache.org/.
 *
 * Put source files in this directory.  All files that have a *.c
 * ending are recursively compiled in the src/ directory and its
 * descendants.  The exception here is the arch/ directory, which
 * is ignored in the default compilation.
 *
 * The arch/<your-arch>/ directories are manually added and
 * recursively compiled for all files that end with either *.c
 * or *.a.  Any directories in arch/ that don't match the
 * architecture being compiled are not compiled.
 *
 * Architecture is set by the BSP/MCU combination.
 */

#define MY_SENSOR_DEVICE "bme280"
#define MY_SENSOR_POLL_TIME 2000

#define LISTENER_CB 1
#define READ_CB 2

static int
read_bme280_temp(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type)
{
    struct sensor_temp_data *std = databuf;
    console_printf("read_bme280!!!\n");
    console_printf("Temperature=%lf (valid %d)\n", std->std_temp, std->std_temp_is_valid);
    return 0;
}

static int
read_bme280_press(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type)
{
    struct sensor_press_data *std = databuf;
    console_printf("Pressure=%lf (valid %d)\n", std->spd_press, std->spd_press_is_valid);
    return 0;
}

static int
read_bme280_hum(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type)
{
    struct sensor_humid_data *std = databuf;
    console_printf("Humidity=%lf (valid %d)\n", std->shd_humid, std->shd_humid_is_valid);
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

static void
bme280_sensor_configure(void)
{
    struct os_dev *dev;
    struct bme280_cfg cfg;
    int rc;

    dev = os_dev_open("bme280", OS_TIMEOUT_NEVER, NULL);
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
    assert(rc == 0);

    os_dev_close(dev);
}

int main(int argc, char **argv)
{
    /* Perform some extra setup if we're running in the simulator. */
#ifdef ARCH_sim
    mcu_sim_parse_args(argc, argv);
#endif

    /* Initialize all packages. */
    sysinit();

    console_printf("my_sensor_app starting!!!\n");

    int rc = bme280_create_i2c_sensor_dev(&bme280.i2c_node, MY_SENSOR_DEVICE,
                                      &g_bme280_i2c_node_cfg,
                                      &g_bme280_sensor_itf);
    console_printf("my_sensor_app rc = %d\n", rc);
    assert(rc == 0);
    struct os_dev *dev;
    dev = os_dev_open(MY_SENSOR_DEVICE, 0, NULL);
    if (dev == NULL)
        console_printf("my_sensor_app dev == NULL\n");
    else
        console_printf("my_sensor_app dev YES!!!\n");
    assert(dev);

    bme280_sensor_configure();
    console_printf("my_sensor_app bme280_sensor_configure DONE\n");

    bme280_sensor = sensor_mgr_find_next_bydevname(MY_SENSOR_DEVICE, NULL);

    if (bme280_sensor == NULL)
        console_printf("my_sensor_app sensor_mgr_find_next_bydevname NULL\n");
    assert(bme280_sensor);

    rc = sensor_register_listener(bme280_sensor, &listener_temp);
    console_printf("sensor_register_listener %d\n", rc);
    assert(rc == 0);
    
    rc = sensor_register_listener(bme280_sensor, &listener_press);
    console_printf("sensor_register_listener %d\n", rc);
    assert(rc == 0);
    
    rc = sensor_register_listener(bme280_sensor, &listener_hum);
    console_printf("sensor_register_listener %d\n", rc);
    assert(rc == 0);
    
    rc = sensor_set_poll_rate_ms(MY_SENSOR_DEVICE, MY_SENSOR_POLL_TIME);
    console_printf("my_sensor_app %d\n", rc);
    assert(rc == 0);

    /* As the last thing, process events from default event queue. */
    console_printf("my_sensor_app waiting....\n");
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }

    return 0;
}
