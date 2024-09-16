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
#include <stdio.h>
#include <string.h>
#include "os/mynewt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "bleenv_sens.h"

#define CSC_ERR_CCC_DESC_IMPROPERLY_CONFIGURED  0x81

/* 2A6D Unit is in pascals with a resolution of 0.1 uint32 */
#define PRESS_VAL 1010555
/* 2A6E - Unit temperature in 0.01 °C sint16 */
#define TEMP_VAL 3751
/* 2A6F = Unit is in percent with a resolution of 0.01 uint16 */
#define HUM_VAL 3751

#define CSC_FEATURES 1234

static const char *manuf_name = "Apache Mynewt";
static const char *model_num = "Mynewt Env Sensor";

static struct ble_env_measurement_state * measurement_state;
uint16_t csc_measurement_handle;
uint16_t csc_control_point_handle;
uint8_t csc_cp_indication_status;

static int
gatt_svr_chr_access_temperature(uint16_t conn_handle,
                                    uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);

static int
gatt_svr_chr_access_pressure(uint16_t conn_handle,
                                uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg);

static int
gatt_svr_chr_access_humidy(uint16_t conn_handle,
                                    uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Cycling Speed and Cadence */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x181A),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: Temperature */
            .uuid = BLE_UUID16_DECLARE(0x2A6E),
            .access_cb = gatt_svr_chr_access_temperature,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Pressure */
            .uuid = BLE_UUID16_DECLARE(0x2A6D),
            .access_cb = gatt_svr_chr_access_pressure,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Humidy */
            .uuid = BLE_UUID16_DECLARE(0x2A6F),
            .access_cb = gatt_svr_chr_access_humidy,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            0, /* No more characteristics in this service */
        }, }
    },

    {
        /* Service: Device Information */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: * Manufacturer name */
            .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
            .access_cb = gatt_svr_chr_access_device_info,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Model number string */
            .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
            .access_cb = gatt_svr_chr_access_device_info,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            0, /* No more characteristics in this service */
        }, }
    },

    {
        0, /* No more services */
    },
};

static int
gatt_svr_chr_access_temperature(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    static const int16_t csc_feature = TEMP_VAL;
    int rc;

    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
    rc = os_mbuf_append(ctxt->om, &csc_feature, sizeof(csc_feature));

    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int
gatt_svr_chr_access_pressure(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    static const uint32_t csc_feature = PRESS_VAL;
    int rc;

    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
    rc = os_mbuf_append(ctxt->om, &csc_feature, sizeof(csc_feature));

    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int
gatt_svr_chr_access_humidy(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;

    static const uint16_t csc_feature = HUM_VAL;
    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
    rc = os_mbuf_append(ctxt->om, &csc_feature, sizeof(csc_feature));

    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

int
gatt_svr_chr_notify_env_measurement(uint16_t conn_handle)
{
    int rc;
    struct os_mbuf *om;
    uint8_t data_buf[11];
    uint8_t data_offset = 1;

    memset(data_buf, 0, sizeof(data_buf));

#if (CSC_FEATURES & CSC_FEATURE_WHEEL_REV_DATA)
    data_buf[0] |= CSC_MEASUREMENT_WHEEL_REV_PRESENT;
    put_le16(&(data_buf[5]), measurement_state->last_wheel_evt_time);
    put_le32(&(data_buf[1]), measurement_state->cumulative_wheel_rev);
    data_offset += 6;
#endif

#if (CSC_FEATURES & CSC_FEATURE_CRANK_REV_DATA)
    data_buf[0] |= CSC_MEASUREMENT_CRANK_REV_PRESENT;
    put_le16(&(data_buf[data_offset]),
             measurement_state->cumulative_crank_rev);
    put_le16(&(data_buf[data_offset + 2]),
             measurement_state->last_crank_evt_time);
    data_offset += 4;
#endif

    om = ble_hs_mbuf_from_flat(data_buf, data_offset);

    rc = ble_gatts_notify_custom(conn_handle, csc_measurement_handle, om);
    return rc;
}

void
gatt_svr_set_cp_indicate(uint8_t indication_status)
{
  csc_cp_indication_status = indication_status;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                           "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
gatt_svr_init(struct ble_env_measurement_state * env_measurement_state)
{
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    measurement_state = env_measurement_state;

    return 0;
}

