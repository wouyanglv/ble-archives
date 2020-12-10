/**
 * Copyright (c) 2012 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* Attention!
 * To maintain compliance with Nordic Semiconductor ASA's Bluetooth profile
 * qualification listings, this section of source code must not be modified.
 */
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_HRS)
#include "ble_bio_sig_svc.h"
#include <string.h>
#include "ble_srv_common.h"
#include "max30003.h"

#define OPCODE_LENGTH 1                                                              /**< Length of opcode inside BioSignal Measurement packet. */
#define HANDLE_LENGTH 2                                                              /**< Length of handle inside BioSignal Measurement packet. */
#define MAX_BIOSIG_LEN   (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH) /**< Maximum size of a transmitted BioSignal Measurement. */


uint64_t initial_timestamp_biosig[3] = {0};                                  /**< Initial BioSignal timestamp value. */
int16_t initial_value_biosig[100] = {0};
uint8_t initial_sample_num[3] = {32,32,32};
uint8_t initial_tot_num = 96;

static bool m_wait_ble_tx;


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_biosig_svc   BioSignal Service structure.
 * @param[in]   p_ble_evt      Event received from the BLE stack.
 */
static void on_connect(ble_biosig_t *p_biosig_svc, ble_evt_t const * p_ble_evt)
{
    p_biosig_svc->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_biosig_svc   BioSignal Service structure.
 * @param[in]   p_ble_evt      Event received from the BLE stack.
 */
static void on_disconnect(ble_biosig_t *p_biosig, ble_evt_t const *p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_biosig->conn_handle = BLE_CONN_HANDLE_INVALID;
}


void ble_biosig_svc_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    ble_biosig_t *p_biosig_svc = (ble_biosig_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_biosig_svc, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_biosig_svc, p_ble_evt);
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            m_wait_ble_tx = false;
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for encoding a BioSignal Measurement.
 *
 * @param[in]   p_biosig_svc     BioSignal Service structure.
 * @param[in]   timestamp                Timestamp of the measurement.
 * @param[in]   measurement              New bio signal measurement.
 * @param[in]   dev_type                 Identifier for the type of bio signal measurement device.
 * @param[out]  p_encoded_buffer Buffer where the encoded data will be written.
 *
 * @return      Size of encoded data.
 */

static uint8_t biosignal_encode(ble_biosig_t *p_biosig_svc, uint64_t * timestamp, uint8_t * sample_num, uint8_t tot_num, int16_t * ecgSamples,  
                        ble_biosig_svc_dev_type dev_type, uint8_t *p_encoded_buffer)
{
    uint8_t len = 0;
    uint32_t sample_period_ns = MAX30003_SAMPLE_PERIOD_NS;
    int     i;

    /*int16_t test_dataset [32] ={0, 0,
                                        1, -1,
                                        5, -5,
                                        10, -10,
                                        16, -16,
                                        50, -50,
                                        100, -100,
                                        256, -256,
                                        500, -500,
                                        1000, -1000,
                                        1024, -1024,
                                        5000, -5000,
                                        8000, -8000,
                                        10000, -10000,
                                        30000, -30000,
                                        32767, -32768
                                        };*/
    
//    SEGGER_RTT_printf(0,"ECG: %d, %d, %d\n",ecgSamples[0], ecgSamples[36], ecgSamples[78]);
//    SEGGER_RTT_printf(0,"Sample num: %d, %d, %d\n",sample_num[0], sample_num[1], sample_num[2]);
//    SEGGER_RTT_printf(0,"tot num: %d\n", tot_num);

    memcpy(&(p_encoded_buffer[len]), &dev_type, sizeof(uint8_t));
    len += sizeof(uint8_t);
    memcpy(&(p_encoded_buffer[len]), &sample_period_ns, sizeof(uint32_t));
    len += sizeof(uint32_t);
    for (int iii = 0; iii < 3; iii++)
    {
      memcpy(&(p_encoded_buffer[len]), &(timestamp[iii]), sizeof(uint64_t));
      len += sizeof(uint64_t);
    }
    for (int kkk = 0; kkk < 3; kkk++)
    {
      memcpy(&(p_encoded_buffer[len]), &(sample_num[kkk]), sizeof(uint8_t));
       len += sizeof(uint8_t);
    }
    memcpy(&(p_encoded_buffer[len]), &(tot_num), sizeof(uint8_t));
       len += sizeof(uint8_t);
    for(i=0;i<tot_num;i++)
    {
       memcpy(&(p_encoded_buffer[len]), &(ecgSamples[i]), sizeof(uint16_t));
       len += sizeof(uint16_t);
    }
    /*for(i=0;i<sample_num;i++)
    {
       memcpy(&(p_encoded_buffer[len]), &(test_dataset[i]), sizeof(uint16_t));
       len += sizeof(uint16_t);
    }*/
 //   SEGGER_RTT_printf(0,"sample_num: %d\r\n", sample_num);
 //  SEGGER_RTT_printf(0," packet len: %d\r\n", len);

    return len;
}


uint32_t ble_biosig_svc_init(ble_biosig_t *p_biosig_svc, ble_biosig_svc_init_t const *p_biosig_init)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;
    uint8_t               encoded_initial_biosig[MAX_BIOSIG_LEN];

    // Initialize service structure
    p_biosig_svc->evt_handler                 = p_biosig_init->evt_handler;
    p_biosig_svc->conn_handle                 = BLE_CONN_HANDLE_INVALID;
    p_biosig_svc->max_biosig_len              = MAX_BIOSIG_LEN;

    // Add service
    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_HEART_RATE_SERVICE);

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_biosig_svc->service_handle);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add bio signal measurement characteristic
    memset(&add_char_params, 0, sizeof(add_char_params));

    add_char_params.uuid              = BLE_UUID_HEART_RATE_MEASUREMENT_CHAR;
    add_char_params.max_len           = MAX_BIOSIG_LEN;
    add_char_params.init_len          = biosignal_encode(p_biosig_svc, initial_timestamp_biosig, initial_sample_num, initial_tot_num, initial_value_biosig, 0, encoded_initial_biosig);
    add_char_params.p_init_value      = encoded_initial_biosig;
    add_char_params.is_var_len        = true;
    add_char_params.char_props.notify = 1;
    add_char_params.cccd_write_access = p_biosig_init->cccd_wr_sec;
    add_char_params.read_access       = p_biosig_init->rd_sec;

    err_code = characteristic_add(p_biosig_svc->service_handle, &add_char_params, &(p_biosig_svc->biosig_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}


uint32_t ble_biosig_svc_measurement_send(ble_biosig_t *p_biosig_svc, uint64_t * timestamp, uint8_t * num_samples, 
                                          uint8_t tot_num, int16_t * ecgSamples, ble_biosig_svc_dev_type dev_type)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (p_biosig_svc->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t                encoded_biosig[MAX_BIOSIG_LEN];
        uint16_t               len;
        uint16_t               hvx_len;
        ble_gatts_hvx_params_t hvx_params;

        len     = biosignal_encode(p_biosig_svc, timestamp, num_samples, tot_num, ecgSamples, dev_type, encoded_biosig);
        hvx_len = len;
        SEGGER_RTT_printf(0, "biosignal len: %d bytes \r\n", len);
        memset(&hvx_params, 0, sizeof(hvx_params));

        // https://devzone.nordicsemi.com/f/nordic-q-a/5646/how-to-enable-indication-for-a-characteristic
        hvx_params.handle = p_biosig_svc->biosig_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = encoded_biosig;

        err_code = sd_ble_gatts_hvx(p_biosig_svc->conn_handle, &hvx_params);
        if (err_code == NRF_SUCCESS) {SEGGER_RTT_printf(0, "NRF SUCCESS - Error code: %d \r\n", err_code);}
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
//        if (err_code == NRF_ERROR_RESOURCES)
//        {
//            m_wait_ble_tx = true;
//            while (m_wait_ble_tx)
//            {
//                nrf_pwr_mgmt_run();
//            }
//            err_code = sd_ble_gatts_hvx(p_biosig_svc->conn_handle, &hvx_params);
//        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}


void ble_biosig_svc_on_gatt_evt(ble_biosig_t *p_biosig_svc, nrf_ble_gatt_evt_t const *p_gatt_evt)
{
    if ( (p_biosig_svc->conn_handle == p_gatt_evt->conn_handle)
        &&  (p_gatt_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        p_biosig_svc->max_biosig_len = p_gatt_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
    }
}
#endif // NRF_MODULE_ENABLED(BLE_HRS)
