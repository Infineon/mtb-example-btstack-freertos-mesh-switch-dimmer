/*******************************************************************************
* File Name: mesh_app.c
*
* Description: This file contains mesh app callback and supported API's.
*
* Related Document: See README.md
*
********************************************************************************
* Copyright 2021-2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Header file includes
 ******************************************************************************/
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_mesh_models.h"
#ifdef MESH_DFU_SUPPORTED
#include "wiced_bt_mesh_dfu.h"
#endif
#include "wiced_bt_mesh_app.h"
#if ( defined(DIRECTED_FORWARDING_SERVER_SUPPORTED) || defined(NETWORK_FILTER_SERVER_SUPPORTED))
#include "wiced_bt_mesh_mdf.h"
#endif
#ifdef PRIVATE_PROXY_SUPPORTED
#include "wiced_bt_mesh_private_proxy.h"
#endif

#ifdef HCI_CONTROL
#include "wiced_transport.h"
#include "hci_control_api.h"
#endif
#include "cy_retarget_io.h"
#include "cybsp_bt_config.h"
#include "wiced_bt_types.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_mesh_core.h"
#include "wiced_bt_mesh_models.h"
#include "wiced_bt_mesh_provision.h"
#include "wiced_bt_mesh_app.h"
#include "wiced_memory.h"

#include "wiced_bt_factory_app_config.h"
#include "wiced_bt_cfg.h"

#include "board.h"
#include "flash_utils.h"
#include "mesh_application.h"
#include "mesh_platform_utils.h"
#include "mesh_cfg.h"
#include "mesh_app.h"


/*******************************************************************************
* Macros
********************************************************************************/
// Time in power on state to clear resets counter
#define MESH_APP_FAST_POWER_OFF_TIMEOUT_IN_SECONDS  5u

// Number of fast power off to do factory reset
#define MESH_APP_FAST_POWER_OFF_NUM                 5u

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
static void mesh_app_init_callback(wiced_bool_t is_provisioned);
static void mesh_app_gatt_conn_status_cb(wiced_bt_gatt_connection_status_t *p_status);
static uint32_t mesh_app_proc_rx_cmd_cb(uint16_t opcode, uint8_t *p_data, uint32_t length);
static void mesh_app_fast_power_off_timer_cb(TimerHandle_t timer_handle);
static void mesh_app_fast_power_off_execute(void);
static void mesh_app_factory_reset_callback(void);

/*******************************************************************************
* Global Variables
*******************************************************************************/
uint8_t mesh_mfr_name[WICED_BT_MESH_PROPERTY_LEN_DEVICE_MANUFACTURER_NAME] = { 'I', 'n', 'f', 'i', 'n', 'e', 'o', 'n', 0 };
uint8_t mesh_model_num[WICED_BT_MESH_PROPERTY_LEN_DEVICE_MODEL_NUMBER]     = { '1', '2', '3', '4', 0, 0, 0, 0 };
uint8_t mesh_system_id[8]                                                  = { 0xbb, 0xb8, 0xa1, 0x80, 0x5f, 0x9f, 0x91, 0x71 };

// Fast power off timer
wiced_timer_t   mesh_app_fast_power_off_timer = { 0 };

TimerHandle_t power_off_timer;

static wiced_bool_t last_provision_state = WICED_TRUE;

/*
 * Mesh application library will call into application functions if provided
 * by the application.
 */
wiced_bt_mesh_app_func_table_t wiced_bt_mesh_app_func_table =
{
    mesh_app_init_callback,                  // application initialization
    NULL,                                   // hardware initialization
    mesh_app_gatt_conn_status_cb,           // GATT connection status
    NULL,                                   // attention processing
    NULL,                                   // notify period set
    mesh_app_proc_rx_cmd_cb,                   // WICED HCI command
    NULL,                                   // LPN sleep
    mesh_app_factory_reset_callback         // factory reset
};

/******************************************************************************
* Function Definitions
******************************************************************************/

/*******************************************************************************
* Function Name: mesh_app_fast_power_off_timer_cb
********************************************************************************
* Summary: It is called if 5 seconds passed since start - then delete reset
*            NVRAM ID to count power ons(resets) frmom 0 Does factory reset on
*            defined fast power off(reset).
*
* Parameters:
*  TimerHandle_t timer_handle: Unused
*
* Return:
*  None
*
*******************************************************************************/
void mesh_app_fast_power_off_timer_cb(TimerHandle_t timer_handle)
{
    uint16_t        id = mesh_application_get_nvram_id_app_start();
    printf("mesh power reset: timeout\n");
    flash_memory_delete(id);
}


/*******************************************************************************
* Function Name: mesh_app_init_callback
********************************************************************************
* Summary: Does factory reset on fast power off(reset).
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void mesh_app_fast_power_off_execute(void)
{
    uint16_t        id;
    uint8_t         cnt;
    wiced_result_t rslt;

    // Get the first usable by application NVRAM Identifier
    id = mesh_application_get_nvram_id_app_start();
    /* read counter from the NVRAM and increment it.
     * If it doesn't exist then reset counter     */

    if (sizeof(cnt) !=  flash_memory_read(id, sizeof(cnt), &cnt, &rslt))
    {
        printf("mesh power reset: read flash failed.\r\n");
        cnt = 1;
    }
    else
        cnt++;
    /* If counter has reached configured limit then delete counter NVRAM ID and
     * do factory reset */
    if (cnt >= MESH_APP_FAST_POWER_OFF_NUM)
    {
        printf("mesh power reset: user requested factory reset\n");
        flash_memory_delete(id);
        mesh_application_factory_reset(); /* Factory reset the mesh application */
    }
    /* Write counter to NVRAM and start 5 sec timer to reset it if it expires */
    else if (sizeof(cnt) != flash_memory_write(id, sizeof(cnt), &cnt, &rslt))
    {
        printf("mesh power reset: write flash failed. \r\n");
    }
    else
    {
        /* Create a periodic timer for LED timeout */
        power_off_timer = xTimerCreate("power_off_timer",
                                              pdMS_TO_TICKS(5000u),
                                              pdFALSE,
                                              NULL,
                                              mesh_app_fast_power_off_timer_cb);

        /* Starting the timer */
        xTimerStart(power_off_timer, MESH_APP_FAST_POWER_OFF_TIMEOUT_IN_SECONDS);

    }

}

/*******************************************************************************
* Function Name: mesh_app_init_callback
********************************************************************************
* Summary: MESH application initialization callback.
*
* Parameters:
*  is_provisioned : Provision status
*
* Return:
*  None
*
*******************************************************************************/
void mesh_app_init_callback(wiced_bool_t is_provisioned)
{

    /* Enable or disable Mesh traces from makefile*/
#if(WICED_BT_MESH_TRACE_ENABLE)
    wiced_bt_mesh_models_set_trace_level(WICED_BT_MESH_CORE_TRACE_DEBUG);
    wiced_bt_mesh_core_set_trace_level(WICED_BT_MESH_CORE_TRACE_FID_ALL, WICED_BT_MESH_CORE_TRACE_DEBUG);
#endif

    printf("Mesh provision status:%d\r\n" , is_provisioned);

    if(!is_provisioned)
        last_provision_state = is_provisioned;

    if(is_provisioned && (1u == last_provision_state))
    {
        mesh_app_fast_power_off_execute();
    }
    last_provision_state = is_provisioned;

    /* Adv Data is fixed. Spec allows to put URI, Name, Appearance and Tx Power
     in the Scan Response Data. */
    if (!is_provisioned)
    {
        (void) mesh_app_adv_config((uint8_t*)MESH_DEVICE_NAME, MESH_DEVICE_APPERANCE);
    }

    /* Set the PWM output frequency and duty cycle */
    if(!is_provisioned){

        board_led_set_blink(USER_LED1, BLINK_SLOW);
    }
    else
    {
        board_led_set_state(USER_LED1, LED_OFF);
    }

#ifdef DIRECTED_FORWARDING_SERVER_SUPPORTED
    wiced_bt_mesh_directed_forwarding_init(
        MESH_DIRECTED_FORWARDING_DIRECTED_PROXY_SUPPORTED,
        MESH_DIRECTED_FORWARDING_DIRECTED_FRIEND_SUPPORTED,
        MESH_DIRECTED_FORWARDING_DEFAULT_RSSI_THRESHOLD,
        MESH_DIRECTED_FORWARDING_MAX_DT_ENTRIES_CNT,
        MESH_DIRECTED_FORWARDING_NODE_PATHS,
        MESH_DIRECTED_FORWARDING_RELAY_PATHS,
        MESH_DIRECTED_FORWARDING_PROXY_PATHS,
        MESH_DIRECTED_FORWARDING_FRIEND_PATHS);

#endif

#ifdef NETWORK_FILTER_SERVER_SUPPORTED
    if (is_provisioned)
        wiced_bt_mesh_network_filter_init();
#endif

#if REMOTE_PROVISION_SERVER_SUPPORTED
    wiced_bt_mesh_remote_provisioning_server_init();
#endif

#ifdef MESH_DFU_SUPPORTED
    wiced_bt_mesh_model_fw_distribution_server_init();
#endif
    mesh_level_client_model_init(is_provisioned);
    printf("Mesh module initialization Done!\r\n");
}


#if defined(ENABLE_BT_SPY_LOG) && defined(ENABLE_HCI_TRACES)
void hci_trace_cback(wiced_bt_hci_trace_type_t type, uint16_t length, uint8_t* p_data)
{
#ifdef ENABLE_ONLY_EXT_ADV_SPY_LOG
    if (type == HCI_TRACE_COMMAND)
    {
        if (p_data[0] < 0x35 || p_data[0] > 0x39 || p_data[1] != 0x20)
            return;
    }
    else if (type == HCI_TRACE_EVENT)
    {
        if (p_data[0] != 0xe || p_data[3] < 0x35 || p_data[3] > 0x39 || p_data[4] != 0x20)
            if (p_data[0] != 0x3e || p_data[1] != 0x6 || p_data[2] != 0x12)
                return;
    }
    else
        return;
#endif
    cybt_debug_uart_send_hci_trace(type, length, p_data);
}
#endif


/*******************************************************************************
* Function Name: mesh_management_callback
********************************************************************************
* Summary: Bluetooth MESH management callback.
*
* Parameters:
*  event : Event type
*  *p_event_data : Event data
*
* Return:
*  wiced_result_t : result
*
*******************************************************************************/
wiced_result_t mesh_management_callback(wiced_bt_management_evt_t event,
                                        wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_bt_ble_advert_mode_t          *p_mode;
    wiced_result_t                      result = WICED_BT_SUCCESS;

    switch (event)
    {
        /* Bluetooth stack enabled */
    case BTM_ENABLED_EVT:

#if defined(ENABLE_BT_SPY_LOG) && defined(ENABLE_HCI_TRACES)
    wiced_bt_dev_register_hci_trace(hci_trace_cback);
#endif

#ifdef _DEB_DELAY_START_SEC   /* Mesh Application to start with defined delay */
        mesh_delay_start_init();
#else
#ifdef MESH_APPLICATION_MCU_MEMORY
        mesh_application_send_hci_event(HCI_CONTROL_EVENT_DEVICE_STARTED, NULL, 0);
#else
        mesh_initialize_random_seed(); /* Initialize the mesh core */   
#endif
#endif
        break;

    case BTM_DISABLED_EVT:
        break;

    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        p_mode = &p_event_data->ble_advert_state_changed;
        printf("Advertisement State Changed:%d\n", *p_mode);
        if (*p_mode == BTM_BLE_ADVERT_OFF)
        {
            printf("BT adv stopped\r\n");
            // On failed attempt to connect FW stops all connectable adverts.
            // If we disconnected then notify core to restart them
            if (!mesh_app_gatt_is_connected())
            {
                wiced_bt_mesh_core_connection_status(0u, WICED_FALSE, 0, 20);
            }
        }
        break;

    case BTM_BLE_SCAN_STATE_CHANGED_EVT:
        printf("BT scan state change:%d\r\n", p_event_data->ble_scan_state_changed);
        break;

    case  BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
        result = WICED_BT_ERROR;
        break;

    default:
        break;
    }

    return result;
}

/*******************************************************************************
* Function Name: mesh_app_proc_rx_cmd_cb
********************************************************************************
* Summary: In 2 chip solutions MCU can send the HCI command that light state has
*          changed.
*
* Parameters:
*  opcode : HCI Command Opcode
*  *pdata : HCI Command parameters
*  length : Length of the HCI Command parameter
*
* Return:
*  wiced_boot_t : WICED_TRUE
*
*******************************************************************************/
uint32_t mesh_app_proc_rx_cmd_cb(uint16_t opcode, uint8_t *p_data, uint32_t length)
{

    printf("mesh app proc rx cmd opcode 0x%02x\n", opcode);

    return WICED_TRUE;
}


/*******************************************************************************
* Function Name: mesh_app_factory_reset_callback
********************************************************************************
* Summary: Callback function for mesh factory reset
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void mesh_app_factory_reset_callback(void)
{
    /*TODO on factory reset*/
}

/*******************************************************************************
* Function Name: mesh_app_gatt_conn_status_cb
********************************************************************************
* Summary: This callback function notifies the GATT connection status.
*
* Parameters:
*  *pstatus : GATT connection status
*
* Return:
*  None
*
*******************************************************************************/
void mesh_app_gatt_conn_status_cb(wiced_bt_gatt_connection_status_t *pstatus)
{
    printf( "mesh app GATT connected status %d, id:%d \n", pstatus->connected,pstatus->conn_id);
}

/*******************************************************************************
* Function Name: mesh_app_adv_config
********************************************************************************
* Summary: Application provided function to read/write information from/into
*            flash memory.
*
* Parameters:
*  *device_name : Name of the mesh node
*  appearance : Appearance of the mesh node
*
* Return:
*  wiced_boot_t : result
*
*******************************************************************************/
wiced_bool_t mesh_app_adv_config(uint8_t *device_name, uint16_t appearance)
{
    cy_rslt_t result;
    wiced_bt_ble_advert_elem_t  adv_elem[3];
    uint8_t                     buf[2];
    uint8_t                     num_elem = 0;

    if(NULL == device_name)
        return WICED_FALSE;

    /* Adv Data is fixed. Spec allows to put URI, Name, Appearance and Tx Power
    in the Scan Response Data. */

    wiced_bt_cfg_settings.device_name = (uint8_t *)device_name;
    wiced_bt_cfg_ble.appearance = (wiced_bt_gatt_appearance_t)appearance;

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len = (uint16_t)strlen((const char*)wiced_bt_cfg_settings.device_name);
    adv_elem[num_elem].p_data = wiced_bt_cfg_settings.device_name;
    num_elem++;

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE;
    adv_elem[num_elem].len = 2;
    buf[0] = (uint8_t)wiced_bt_cfg_settings.p_ble_cfg->appearance;
    buf[1] = (uint8_t)(wiced_bt_cfg_settings.p_ble_cfg->appearance >> 8);
    adv_elem[num_elem].p_data = buf;
    num_elem++;

    result = wiced_bt_mesh_set_raw_scan_response_data(num_elem, adv_elem);

    if(WICED_TRUE == result)
    {
        printf("Advertising in the name \"%s\"\n",wiced_bt_cfg_settings.device_name);
    }
    else
    {
        printf("Failed to set scan response data \n");
    }

    return result;
}


/* [] END OF FILE */