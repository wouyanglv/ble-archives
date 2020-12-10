/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
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
/** @example examples/ble_peripheral/ble_app_hrs/main.c
 *
 * @brief Heart Rate Service Sample Application main file.
 *
 * This file contains the source code for a sample application using the Heart Rate service
 * (and also Battery and Device Information services). This application uses the
 * @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_bio_sig_svc.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_delay.h"
#include "nrfx_rtc.h"
#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_lesc.h"
#include "nrf_ble_qwr.h"
#include "ble_conn_state.h"
#include "nrf_pwr_mgmt.h"
#include "nrfx_gpiote.h"
#include "app_config.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "spi.h"

#include "bmi160.h"

#define DEVICE_NAME                         "Implantable_MA"                            /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                   "NeuroLux"                              /**< Manufacturer. Will be passed to Device Information Service. */

#define APP_ADV_INTERVAL                    300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define APP_ADV_DURATION                    0                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define APP_BLE_CONN_CFG_TAG                1                                       /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_OBSERVER_PRIO               3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define POWER_LEVEL_MEAS_INTERVAL           APP_TIMER_TICKS(3)                   /**< Power level measurement interval (ticks). */
#define STARTUP_DELAY                       APP_TIMER_TICKS(2000)                   /**< 2 second startup delay. */

#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(7.5, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(60, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                       0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY      APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        1000                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define LESC_DEBUG_MODE                     0                                       /**< Set to 1 to use LESC debug keys, allows you to use a sniffer to inspect traffic. */

#define SEC_PARAM_BOND                      1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                      0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                      1                                       /**< LE Secure Connections enabled. */
#define SEC_PARAM_KEYPRESS                  0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                       0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE              7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE              16                                      /**< Maximum encryption key size. */

#define DEAD_BEEF                           0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


#define TIMESTAMP_PRECISION   1000000000
#define RTC_BIT_WIDTH         24

#define WATERMARK_FRAMES  38
#define BYTES_PER_FRAME 6


BLE_BIOSIG_SVC_DEF(m_biosig_svc);                                   /**< BioSignal service instance. */
BLE_BAS_DEF(m_bas);                                                 /**< Structure used to identify the battery service. */
NRF_BLE_GATT_DEF(m_gatt);                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                 /**< Advertising module instance. */
APP_TIMER_DEF(m_power_level_timer_id);                              /**< Power level timer. */
APP_TIMER_DEF(m_startup_delay_timer_id);                            /**< Startup delay timer. */


static uint16_t m_conn_handle         = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static ble_uuid_t m_adv_uuids[] =                                   /**< Universally unique service identifiers. */
{
    {BLE_UUID_HEART_RATE_SERVICE,           BLE_UUID_TYPE_BLE},
    {BLE_UUID_BATTERY_SERVICE,              BLE_UUID_TYPE_BLE},
    {BLE_UUID_DEVICE_INFORMATION_SERVICE,   BLE_UUID_TYPE_BLE}
};

static bool             m_startup_delay;


struct bmi160_dev sensor;
struct bmi160_fifo_frame fifo_frame;
struct bmi160_cfg m_accel_cfg;
static bool watermark_triggered;
static uint8_t fifo_buff[WATERMARK_FRAMES * BYTES_PER_FRAME];
     

static const nrfx_rtc_t   m_rtc = NRFX_RTC_INSTANCE(2); /**< Declaring an instance of nrf_drv_rtc for RTC2 (RTC0 is used by BLE, RTC1 used by app timer). */
static uint64_t           m_rtc_overflows;
static uint64_t           m_ns_per_tick;
static bool               m_rtc_init;




/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}




/**@brief Function for starting advertising.
 */
void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        ret_code_t err_code;

        err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for advertising wrapper.
 */
static void advertising_start_handler(void *p_event_data, uint16_t event_size) 
{
  advertising_start(false);
}
/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start(false);
            break;

        default:
            break;
    }
}


/**@brief Function for performing power level measurement and updating the Power Level characteristic
 *        in Battery Service.
 */
static uint8_t pl = 100;

static void power_level_update(void)
{
    ret_code_t err_code;
    uint8_t  power_level;

    power_level = pl;
    pl--;
    if (pl == 0)
      pl = 100;

    err_code = ble_bas_battery_level_update(&m_bas, power_level, BLE_CONN_HANDLE_ALL);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}


/**@brief Function for handling the Power Level measurement timer timeout.
 *
 * @details This function will be called each time the power level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void power_level_meas_timeout_handler(void * p_context)
{
//SEGGER_RTT_printf(0,"timeout\n");
    UNUSED_PARAMETER(p_context);
   power_level_update();
    //    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    //    {
//              if(USE_EMG)
//              {
//              read_sensor_data(&m_emg_dev, BLE_BIOSIG_DEV_TYPE_EMG);
//              }
//              if(USE_EEG)
//              {
//              read_sensor_data(&m_eeg_dev, BLE_BIOSIG_DEV_TYPE_EEG);
//              }
//              
       // }
}

/**@brief Function for handling the Startup Delay timer timeout.
 *
 * @details Can now start normal startup
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void startup_delay_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    m_startup_delay = false;
}



/**@brief Function starting the internal LFCLK oscillator.
 *
 * @details This is needed by RTC1 which is used by the Application Timer
 *          (When SoftDevice is enabled the LFCLK is always running and this is not needed).
 */
static void lfclk_request(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}


char *uint64_to_string (uint64_t input)
{
     static char result [21] = "";
     memset(&result [0], 0, sizeof(result));
     char temp [21] = "";
     char c;
     uint8_t base=10;

     while (input)
     {
        int num=input % base;
        input /= base;
        c= '0' + num;

        sprintf(temp, "%c%s", c, result);
        strcpy(result, temp);
     }
     return result;
}

/** @brief: Returns the timestamp (since power up) in usecs.
 */
static uint64_t rtc_timestamp_get()
{
    uint64_t ticks;
    uint64_t timestamp;

    ticks = nrfx_rtc_counter_get(&m_rtc);
    timestamp = ((m_rtc_overflows << RTC_BIT_WIDTH) + ticks) * m_ns_per_tick;

    return timestamp;
}


/** @brief: Function for handling the RTC2 interrupts.
 * Triggered on TICK and OVERFLOW match.
 */
static void rtc_handler(nrfx_rtc_int_type_t int_type)
{
    if (int_type == NRFX_RTC_INT_OVERFLOW)
    {
        //keep track of overflow count to determine time since start
        m_rtc_overflows++;
        nrf_rtc_event_clear(m_rtc.p_reg, NRF_RTC_EVENT_OVERFLOW);
    }
    else if (int_type == NRFX_RTC_INT_TICK)
    {
        //first tick, clear counter and continue initialization
        nrf_rtc_event_clear(m_rtc.p_reg, NRF_RTC_EVENT_TICK);
        nrfx_rtc_tick_disable(&m_rtc);
        nrfx_rtc_counter_clear(&m_rtc);
//        rtc_timestamp_get();  //display current
        m_rtc_init = true;
    }
}


/** @brief Function initialization and configuration of RTC driver instance.
 */
static void rtc_config(void)
{
    uint32_t err_code;

    m_rtc_init = false;
    m_rtc_overflows = 0;
    float sec_per_tick = ( ((float)APP_TIMER_CONFIG_RTC_FREQUENCY + 1.0f) ) / (float)NRFX_RTC_DEFAULT_CONFIG_FREQUENCY;
    m_ns_per_tick = sec_per_tick * TIMESTAMP_PRECISION;
//    SEGGER_RTT_printf(0,"sec per tick: %f\r\n", sec_per_tick);
//    SEGGER_RTT_printf(0,"m_ns_sec per tick: %d\r\n", m_ns_per_tick);


    //Initialize RTC instance
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = 0;
    err_code = nrfx_rtc_init(&m_rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    nrfx_rtc_counter_clear(&m_rtc);
    nrfx_rtc_overflow_enable(&m_rtc, true);
    nrfx_rtc_tick_enable(&m_rtc, true);
    nrfx_rtc_enable(&m_rtc);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create timers.
    err_code = app_timer_create(&m_power_level_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                power_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_startup_delay_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                startup_delay_timeout_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief GATT module event handler.
 */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)
    {
        NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.",
                     p_evt->conn_handle,
                     p_evt->params.att_mtu_effective);
    }

    ble_hrs_on_gatt_evt(&m_biosig_svc, p_evt);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the BioSignal, Battery and Device Information services.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    ble_biosig_svc_init_t     biosig_svc_init;
    ble_bas_init_t     bas_init;
    ble_dis_init_t     dis_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Bio Signal Service.
    memset(&biosig_svc_init, 0, sizeof(biosig_svc_init));

    biosig_svc_init.evt_handler = NULL;
    biosig_svc_init.cccd_wr_sec = SEC_OPEN;
    biosig_svc_init.rd_sec = SEC_OPEN;

    err_code = ble_biosig_svc_init(&m_biosig_svc, &biosig_svc_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    // Here the sec level for the Battery Service can be changed/increased.
    bas_init.bl_rd_sec        = SEC_OPEN;
    bas_init.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init.bl_report_rd_sec = SEC_OPEN;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service.
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);

    dis_init.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
    ret_code_t err_code;

    // Start application timers.
    err_code = app_timer_start(m_power_level_timer_id, POWER_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

#ifdef NRF52_DK
    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);
#endif

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
#ifdef NRF52_DK
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
#endif
            break;

        case BLE_ADV_EVT_IDLE:
            app_sched_event_put(NULL, 0, advertising_start_handler);
//            sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
//            SEGGER_RTT_printf(0, "BLE Connected\r\n");

#ifdef NRF52_DK
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
#endif
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);

            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_2MBPS,
                .tx_phys = BLE_GAP_PHY_2MBPS,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            SEGGER_RTT_printf(0, "PHY update status: %d\r\n", err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected, reason %d.",
                          p_ble_evt->evt.gap_evt.params.disconnected.reason);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            SEGGER_RTT_printf(0, "Disconnected:%d\n",p_ble_evt->evt.gap_evt.params.disconnected.reason);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_2MBPS,
                .tx_phys = BLE_GAP_PHY_2MBPS,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
    
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;
        
        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;

         case BLE_GAP_EVT_AUTH_STATUS:
             NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);
    ble_cfg_t ble_cfg;
    memset(&ble_cfg, 0, sizeof ble_cfg);
    ble_cfg.conn_cfg.conn_cfg_tag = BLE_CONN_CFG_GATTS;
    ble_cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 20;
    
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);
    

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */

void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_KEY_3:
            for(uint8_t i=0; i < 10; i++)
            {
               /* err_code = ble_biosig_svc_measurement_send(&m_biosig_svc, timestamp++, 1, 
                                    0x1234, BLE_BIOSIG_DEV_TYPE_EEG);
                while (err_code == NRF_ERROR_RESOURCES) 
                {
                    nrf_delay_ms(300);
                    SEGGER_RTT_printf(0, "SEND STATUS TRYING\r\n");
                    err_code = ble_biosig_svc_measurement_send(&m_biosig_svc, timestamp++, 1,
                                    0x1234, BLE_BIOSIG_DEV_TYPE_EEG);
                }
                nrf_delay_ms(100);*/
            }
//            SEGGER_RTT_printf(0, "BLE: %lu\r\n", timestamp);
        break;

        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;
    
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

#ifdef NRF52_DK
    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);
#endif

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_lesc_request_handler();
    APP_ERROR_CHECK(err_code);

    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


//static void eeg_data_avail_handler()
//{
//    m_eeg_data_avail = true;
//}
//
//static void emg_data_avail_handler()
//{
//    m_emg_data_avail = true;
//}


//static void read_sensor_data(max30003_dev_t *dev, ble_biosig_svc_dev_type dev_type)
//{
//    uint32_t ecgFIFO, ETAG, status;
//    int16_t ecgSample;
//    int16_t ecgSamples[32];
//    uint64_t last_sample_timestamp;
//    uint8_t num_samples = 0;
//    uint8_t i;
//    ret_code_t ret;
//    ret_code_t err_code;
//    bool retry_ble_send = false;
//  
//    /* Read back ECG samples from the FIFO */
//    ret = max30003_get_status(dev, &status);      // Read the STATUS register
//
//    // Check if EINT interrupt asserted
//    if ((status & EINT_STATUS) == EINT_STATUS) 
//    {     
//        last_sample_timestamp = rtc_timestamp_get();
//        
//        do 
//        {
//            max30003_get_fifo(dev, &ecgFIFO);                        // Read FIFO
//
//            ecgSample = (ecgFIFO >> 8) & 0xFFFF;   // Isolate voltage data
//            ETAG = (ecgFIFO >> 3) & ETAG_BITS;     // Isolate ETAG
//    
//            ecgSamples[num_samples] = ecgSample;
////            if (num_samples == 15) {SEGGER_RTT_printf(0, "Device : %d, No. %d, Signal: %d\n\r", dev_type, num_samples, ecgSample);}
//            num_samples++;
//        } 
//        while (ETAG == FIFO_VALID_SAMPLE || ETAG == FIFO_FAST_SAMPLE);   // Check that sample is not last sample in FIFO              
//
////        SEGGER_RTT_printf(0, "ETAG: %d\r\n", ETAG);
////        SEGGER_RTT_printf(0, "%d samples read from FIFO\r\n", num_samples);
//    
//        // Check if FIFO has overflowed
//        if(ETAG == FIFO_OVF)
//        {                  
//            max30003_reset_fifo(dev); // Reset FIFO
// //           SEGGER_RTT_printf(0, "FIFO overflow!\r\n");
//        }
//
//#ifndef SEND_BLE
//        // Send and display received samples
//
//            // send sample over BLE
//            err_code = ble_biosig_svc_measurement_send(&m_biosig_svc, last_sample_timestamp, 
//                                    num_samples, ecgSamples, dev_type);
//
//            while (err_code == NRF_ERROR_RESOURCES) 
//            {
//                nrf_delay_ms(100);
//                retry_ble_send = true;
////                SEGGER_RTT_printf(0, "SEND STATUS TRYING\r\n");
//                err_code = ble_biosig_svc_measurement_send(&m_biosig_svc, last_sample_timestamp, 
//                                    num_samples, ecgSamples, dev_type);
//            }
//            if ((err_code != NRF_SUCCESS) &&
//                (err_code != NRF_ERROR_INVALID_STATE) &&
//                (err_code != NRF_ERROR_RESOURCES) &&
//                (err_code != NRF_ERROR_BUSY) &&
//                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
//               )
//            {
//                APP_ERROR_HANDLER(err_code);
//            }
//
//
//#endif
//    }
//    else if ((status & EOVF_STATUS) == EOVF_STATUS)
//    {
//        max30003_reset_fifo(dev); // Reset FIFO
//    }
//
//    // Check if FIFO overflowed while we were sending BLE data, if so reset the FIFO
//    ret = max30003_get_status(dev, &status);      // Read the STATUS register
//    if ((status & EOVF_STATUS) == EOVF_STATUS)
//    {
//        max30003_reset_fifo(dev); // Reset FIFO
//    }
//}


int8_t spi_read (uint8_t dev_id, uint8_t reg_addr, uint8_t *temp_buf, uint16_t temp_len)
{

    int8_t rslt = BMI160_OK;
    
    uint8_t tx_buf[temp_len];
    tx_buf[0] = reg_addr;
    
    for (int16_t i = 1; i < temp_len; i++)
    {
      tx_buf[i] = 0xFF;
    }

    bool repeated_xfer = false;

    rslt = spi_transfer(dev_id, tx_buf, 
                temp_len, temp_buf, 
                temp_len, repeated_xfer);

    return rslt;
}

int8_t spi_write (uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    int8_t rslt = BMI160_OK;
    
    uint16_t temp_len = len + 1;
    uint8_t tx_buf[temp_len];
    tx_buf[0] = reg_addr;
    for (int16_t i = 1; i < temp_len; i++)
    {
      tx_buf[i] = data[i-1];
    }

    bool repeated_xfer = false;

    rslt = spi_transfer (dev_id, tx_buf, 
                        temp_len, NULL, 0, repeated_xfer);

    return rslt;
}

int8_t bmi160_init_and_config(struct bmi160_dev *dev)
{
    int8_t rslt = BMI160_OK;

    dev->id = 0;
    dev->interface = BMI160_SPI_INTF;
    dev->read = spi_read;
    dev->write = spi_write;
    dev->delay_ms = nrf_delay_ms;
    rslt = bmi160_init(dev);

    uint8_t data = 0;

    dev->accel_cfg.odr = BMI160_ACCEL_ODR_1600HZ;  // There is a problem setting ODR to 1600 Hz. If using 1600 Hz, do in set_accel_conf() in bmi160.c;
    dev->accel_cfg.range = BMI160_ACCEL_RANGE_4G; //Not sure why 2G does not work. ADC value range wrong.
    dev->accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;
    dev->accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;
    dev->gyro_cfg.power = BMI160_GYRO_SUSPEND_MODE;    
    rslt = bmi160_set_power_mode(dev);
    rslt = bmi160_set_sens_conf(dev);

    
    fifo_frame.data = fifo_buff;
    fifo_frame.length = WATERMARK_FRAMES * BYTES_PER_FRAME;
    dev->fifo = &fifo_frame; //Without instancing dev->fifo, setting values to dev->fifo->... fails.
    rslt = bmi160_set_fifo_config(BMI160_FIFO_ACCEL, BMI160_ENABLE, dev);

    rslt = bmi160_set_fifo_wm(WATERMARK_FRAMES * BYTES_PER_FRAME/4, dev); //watermark unit is 4 bytes

    struct bmi160_int_settg int_config;
    int_config.int_channel = BMI160_INT_CHANNEL_1;
    int_config.int_type = BMI160_ACC_GYRO_FIFO_WATERMARK_INT;
    int_config.int_pin_settg.output_en = BMI160_ENABLE;
    int_config.int_pin_settg.output_mode = BMI160_DISABLE;
    int_config.int_pin_settg.output_type = BMI160_DISABLE;
    int_config.int_pin_settg.edge_ctrl = BMI160_ENABLE;
    int_config.int_pin_settg.input_en = BMI160_DISABLE;
    int_config.int_pin_settg.latch_dur = BMI160_LATCH_DUR_NONE;    
    bmi160_set_int_config(&int_config, dev);
   
    return rslt;
}


uint64_t prev = 0;
uint32_t flush = 0;
uint32_t cnt = 0;  
int8_t read_sensor_accel_headerless(struct bmi160_dev *dev)
{
     int8_t rslt = BMI160_OK;
     ret_code_t err_code;
     bool retry_ble_send = false;

     uint64_t last_sample_timestamp;
     uint16_t index = 0;
     uint16_t fifo_level;


     struct bmi160_sensor_data accel_data[WATERMARK_FRAMES];
     uint8_t accel_frames_req = WATERMARK_FRAMES;
     uint8_t accel_index;
     
     last_sample_timestamp = rtc_timestamp_get();
     uint16_t delta = 38000000000/(last_sample_timestamp - prev);
     prev = last_sample_timestamp;
//     SEGGER_RTT_printf(0,"%d Hz\n", delta);

//     SEGGER_RTT_printf(0,"fifo before read: ");
     rslt = bmi160_get_fifo_data(dev);
     rslt = bmi160_extract_accel(accel_data, &accel_frames_req, dev);
 //    SEGGER_RTT_printf(0,"%d, %d, %d\n", accel_data[0].x, accel_data[0].y, accel_data[0].z);     
//     SEGGER_RTT_printf(0,"fifo after read: ");

//    for (int k = 0; k<WATERMARK_FRAMES; k++)
//    {
//      SEGGER_RTT_printf(0,"%d: %d, %d, %d\n", k, accel_data[k].x, accel_data[k].y, accel_data[k].z);
//      }
      
     err_code = ble_biosig_svc_measurement_send(&m_biosig_svc, last_sample_timestamp,
                                    WATERMARK_FRAMES, accel_data);
//     if (err_code == NRF_ERROR_RESOURCES)
//     {
//            while (err_code == NRF_ERROR_RESOURCES) 
//            {
//               // nrf_delay_ms(25);
//                retry_ble_send = true;
//                SEGGER_RTT_printf(0, "SEND STATUS TRYING\r\n");
//                err_code = ble_biosig_svc_measurement_send(&m_biosig_svc, last_sample_timestamp, 
//                                    WATERMARK_FRAMES, accel_data);
//            }
//            if ((err_code != NRF_SUCCESS) &&
//                (err_code != NRF_ERROR_INVALID_STATE) &&
//                (err_code != NRF_ERROR_RESOURCES) &&
//                (err_code != NRF_ERROR_BUSY) &&
//                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
//               )
//            {
//                APP_ERROR_HANDLER(err_code);
//            }
////            uint16_t data3 = 0;
////            SEGGER_RTT_printf(0,"after resource error\n");
////            get_fifo_byte_counter(&data3, dev);
//                        
//             bmi160_set_fifo_flush(dev);
//             SEGGER_RTT_printf(0,"interrupt %d, flushed %d\n", cnt, flush);
//             flush++;
//     }

     if (err_code == 1000) { // 1000 means packet resent due to ble resource error, there is fifo overrun above watermark, need to flush fifo to restart watermark interrupt.
       get_fifo_byte_counter(&fifo_level, dev);
      // SEGGER_RTT_printf(0, "fifo level: %d\n", fifo_level);
       if (fifo_level >= WATERMARK_FRAMES * BYTES_PER_FRAME - 2) {
         bmi160_set_fifo_flush(dev);
     //    SEGGER_RTT_printf(0, "interrupt %d, flushed %d\n", cnt, flush);
         flush++;
       }
     }
     return rslt;
}
 
  
static void bmi160_int_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{   
    watermark_triggered = true;
 //   SEGGER_RTT_printf(0,"int. %d\n", cnt);
    cnt++;
}

void bmi160_init_intb_gpio(struct bmi160_dev *dev)
{
    ret_code_t err_code;

    uint8_t pin_num =  BMI160_SPI_DEV0_INTB_PIN;

    if (!nrfx_gpiote_is_init())
    {
        err_code = nrfx_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrfx_gpiote_in_init(pin_num, &in_config, bmi160_int_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_gpiote_in_event_enable(pin_num, true);
}

/**@brief Function for application main entry.
 */

int main(void)
{
    bool erase_bonds;
//SEGGER_RTT_printf(0,"1\n");
    // Initialize timers
    log_init();
    timers_init();
//SEGGER_RTT_printf(0,"2\n");
    // Delay startup to allow power harvesting to ramp up
    lfclk_request();  //turn on here because we need it before softdevice is started
//    SEGGER_RTT_printf(0,"3\n");
    m_startup_delay = true;
    ret_code_t err_code = app_timer_start(m_startup_delay_timer_id, STARTUP_DELAY, NULL);
    APP_ERROR_CHECK(err_code);

    while (m_startup_delay) 
    { 
        idle_state_handle();
    }

    // Remainder of initialization
//    SEGGER_RTT_printf(0,"4\n");
    rtc_config();
    buttons_leds_init(&erase_bonds);
//    SEGGER_RTT_printf(0,"5\n");
    power_management_init();
//    SEGGER_RTT_printf(0,"6\n");
    ble_stack_init();
//    SEGGER_RTT_printf(0,"7\n");
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();
    peer_manager_init();
    // Start execution.
    NRF_LOG_INFO("EEG/EMG monitoring started.");
//    SEGGER_RTT_printf(0,"here\n");
    application_timers_start();
    advertising_start(erase_bonds);
    // wait for RTC to finish initializing
    while (!m_rtc_init) 
    { 
        idle_state_handle();
    }

    spi_init();
    bmi160_init_and_config(&sensor);
    bmi160_init_intb_gpio(&sensor);
    bmi160_set_fifo_flush(&sensor);
    
    watermark_triggered = false;
    
    // Enter main loop.
    while (1) {
      if (watermark_triggered) {
        read_sensor_accel_headerless(&sensor);
        watermark_triggered = false;
        
      }
//      if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
//      }

      idle_state_handle();
    }
}