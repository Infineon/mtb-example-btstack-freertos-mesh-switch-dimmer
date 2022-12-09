/*******************************************************************************
* File Name: mesh_app.h
*
* Description: This file is the public interface of mesh_app.c
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
 * Include guard
 ******************************************************************************/
#ifndef MESH_APP_H_
#define MESH_APP_H_

#include "cy_retarget_io.h"
#include "cybsp_bt_config.h"
#include "wiced_bt_mesh_app.h"
#include "wiced_bt_mesh_models.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/*******************************************************************************
* Function Prototypes
*******************************************************************************/
typedef void (*mesh_app_free_t)(uint8_t *p_buf);

void mesh_application_init(void);
void mesh_level_client_model_init(wiced_bool_t is_provisioned);
void mesh_dimmer_set_level(bool is_instant, bool is_final);
wiced_bool_t mesh_app_adv_config(uint8_t *device_name, uint16_t appearance);
wiced_result_t mesh_management_callback(wiced_bt_management_evt_t event,
                                wiced_bt_management_evt_data_t *p_event_data);
                                
#endif /* MESH_APP_H_ */
