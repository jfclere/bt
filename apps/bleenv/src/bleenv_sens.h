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

#include "modlog/modlog.h"
#include "nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Device Information configuration */
#define GATT_DEVICE_INFO_UUID                   0x180A
#define GATT_MANUFACTURER_NAME_UUID             0x2A29
#define GATT_MODEL_NUMBER_UUID                  0x2A24

struct ble_env_measurement_state {
    double temperature;
    double pressure;
    double humidity;
};

/* getters for the values */
float get_temp();
float get_press();
float get_humid();

extern uint16_t csc_measurement_handle; /* XXX */
extern uint16_t csc_control_point_handle; /* XXX */

int gatt_svr_init(struct ble_env_measurement_state * env_measurement_state);
int gatt_svr_chr_notify_env_measurement(uint16_t conn_handle);
void gatt_svr_set_cp_indicate(uint8_t indication_status);

#ifdef __cplusplus
}
#endif
