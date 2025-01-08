/**
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

#include "os/mynewt.h"
#include "sysinit/sysinit.h"
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "hal/hal_i2c.h"
#include "bus/bus.h"
#include <syscfg/syscfg.h>
#include <sensor/sensor.h>
#include "console/console.h"

/*
 * test for a console UART write and a led blinking on GPIO3 of the nicla sense me
 * note the VDDIO_EXT powers the external pin of the GPIO so it needs to be set to 3.3V
 * for the led to blink.
 */

static const struct bus_i2c_node_cfg g_bq25120_i2c_node_cfg = {
    .node_cfg = {
        .bus_name = "i2c0",
    },
    .addr = 0x6A,
    .freq = 100,
};


static volatile int g_task1_loops;

/* For LED toggling */
int g_led_pin = 10;

/* The timer callout */
static struct os_callout blinky_callout;

/*
 * Event callback function for timer events. It toggles the led pin.
 */
static void
timer_ev_cb(struct os_event *ev)
{
    assert(ev != NULL);

    ++g_task1_loops;
    hal_gpio_toggle(g_led_pin);
    console_printf("hal_gpio_toggle\n");

    os_callout_reset(&blinky_callout, OS_TICKS_PER_SEC);
}

static void
init_timer(void)
{
    /*
     * Initialize the callout for a timer event.
     */
    os_callout_init(&blinky_callout, os_eventq_dflt_get(),
                    timer_ev_cb, NULL);

    os_callout_reset(&blinky_callout, OS_TICKS_PER_SEC);
}

/**
 * main
 *
 * The main task for the project. This function initializes packages,
 * and then blinks the BSP LED in a loop.
 *
 * @return int NOTE: this function should never return!
 */
int
mynewt_main(int argc, char **argv)
{
    int rc = 0;
    uint8_t i2c_num = 0;
    struct hal_i2c_master_data data;

    sysinit();
    console_printf("mynewt_main\n");

    rc = hal_gpio_init_out(g_led_pin, 1);
    console_printf("mynewt_main hal_gpio_init_out %d\n", rc);


    rc = hal_i2c_init(i2c_num, (void *) &g_bq25120_i2c_node_cfg );
    console_printf("mynewt_main hal_i2c_init %d\n", rc);

    data.address = 0x6A;
    data.len = 2;
    uint8_t buf[2];
    // unsigned char buf[2];
    buf[0] = 0x07;
    buf[1] = 0xA8; // 1.8 V 
    buf[1] = 0xE4; // 3.3 V 
    data.buffer = buf;

    rc = hal_i2c_master_write(i2c_num, &data, OS_TICKS_PER_SEC, 1);
    console_printf("mynewt_main hal_i2c_master_write %d\n", rc);

    init_timer();

    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    assert(0);

    return rc;
}

