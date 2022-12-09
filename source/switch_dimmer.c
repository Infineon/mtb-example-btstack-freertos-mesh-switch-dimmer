/*******************************************************************************
* File Name: switch_dimmer.c
*
* Description: This file contains mesh client level implementation.
*
* Related Document: See README.md
*
********************************************************************************
* Copyright 2022, Cypress Semiconductor Corporation (an Infineon company) or
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
#include "cy_retarget_io.h"
#include "cybsp_bt_config.h"
#include "wiced_bt_types.h"
#include "wiced_bt_stack.h"

#include "wiced_bt_types.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_mesh_app.h"
#include "wiced_bt_mesh_core.h"
#include "wiced_bt_mesh_models.h"
#include "mesh_cfg.h"
#include "mesh_app.h"
#include "board.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define SWITCH_NUM_LEVELS                (9u)
/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void mesh_level_client_message_handler(uint16_t event, wiced_bt_mesh_event_t *p_event,
                                            wiced_bt_mesh_level_status_data_t *p_data);
void mesh_dimmer_set_level(bool is_instant, bool is_final);
/*******************************************************************************
 * Variables Definitions
 ******************************************************************************/
/* Variable to keep the level step */
extern uint8_t button_step_count;

uint16_t client_level_step[SWITCH_NUM_LEVELS] =
{
    0x8000, 0xa000, 0xC000, 0xE000, 0x0000, 0x2000, 0x4000, 0x6000, 0x7FFF,
};

/* Structure to keep the light state of lightness server. */
typedef struct
{
    uint8_t level_step;
    uint32_t remaining_time;
} mesh_level_state_t;

/* Application state */
mesh_level_state_t app_state;

/*******************************************************************************
 * Function Name: mesh_level_client_model_init
 *******************************************************************************
 * Summary:
 *  mesh level client model initialization.
 *
 * Parameters:
 *  wiced_bool_t is_provisioned : provisioned status
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mesh_level_client_model_init(wiced_bool_t is_provisioned)
{
    wiced_bt_mesh_model_level_client_init(MESH_LEVEL_CLIENT_ELEMENT_INDEX,
            mesh_level_client_message_handler, is_provisioned);
}

/*******************************************************************************
 * Function Name: mesh_level_client_message_handler
 *******************************************************************************
 * Summary:
 *  level client message handler.
 *
 * Parameters:
 *  uint16_t event : event type
 *  wiced_bt_mesh_event_t *p_even : event pointer
 *  wiced_bt_mesh_level_status_data_t *p_data : event data
 * 
 * Return:
 *  void
 *
 ******************************************************************************/
void mesh_level_client_message_handler(uint16_t event, wiced_bt_mesh_event_t *p_event,
                                            wiced_bt_mesh_level_status_data_t *p_data)
{

    switch (event)
    {
    case WICED_BT_MESH_TX_COMPLETE:
        printf("Mesh client level tx complete status:%d\n", p_event->status.tx_flag);
        break;
    case WICED_BT_MESH_LEVEL_STATUS:
        break;

    default:
        printf("Mesh lightness server unknown event:%d\r\n", event);
        break;
    }

}

/*******************************************************************************
 * Function Name: mesh_dimmer_set_level
 *******************************************************************************
 * Summary:
 * This function set the client level on button event is received.
 *
 * Parameters:
 *  bool is_instant : instant flag
 *  bool is_final : final flag
 * 
 * Return:
 *  void
 *
 ******************************************************************************/
void mesh_dimmer_set_level(bool is_instant, bool is_final)
{

    wiced_bt_mesh_level_set_level_t set_data;

    set_data.level = client_level_step[button_step_count];
    set_data.transition_time = is_instant ? 100 : 500;
    set_data.delay = 0;

    app_state.level_step = set_data.level;
    app_state.remaining_time = set_data.transition_time;

    printf("Mesh client set level:%d transition time:%ld final:%d\n", set_data.level, set_data.transition_time, is_final);
    wiced_bt_mesh_model_level_client_set(0, is_final, &set_data);
}


/* [] END OF FILE */