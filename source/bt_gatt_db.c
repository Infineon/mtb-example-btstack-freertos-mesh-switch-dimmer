/*******************************************************************************
* File Name: bt_gatt_db.c
*
* Description: This file contains GATT configurations.
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

#include "wiced_bt_types.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_cfg.h"
#include "wiced_memory.h"
#include "wiced_bt_uuid.h"
#include "bt_gatt_db.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_mesh_core.h"
#include "wiced_bt_ota_firmware_upgrade.h"

/*
 * This is the GATT database for the WICED Mesh applications.
 * The database defines services, characteristics and
 * descriptors supported by the application.  Each attribute in the database
 * has a handle, (characteristic has two, one for characteristic itself,
 * another for the value).  The handles are used by the peer to access
 * attributes, and can be used locally by application, for example to retrieve
 * data written by the peer.  Definition of characteristics and descriptors
 * has GATT Properties (read, write, notify...) but also has permissions which
 * identify if peer application is allowed to read or write into it.
 * Handles do not need to be sequential, but need to be in order.
 *
 * Mesh applications have 2 GATT databases. One is shown while device is not
 * provisioned, this one contains Provisioning GATT service.  After device is
 * provisioned, it has GATT Proxy service.
 */
uint8_t gatt_db_unprovisioned[]=
{
    // Declare mandatory GATT service
    PRIMARY_SERVICE_UUID16(MESH_HANDLE_GATT_SERVICE, UUID_SERVICE_GATT),


    // Handle MESH_HANDLE_GAP_SERVICE (0x14): GAP service
    // Device Name and Appearance are mandatory characteristics.
    PRIMARY_SERVICE_UUID16(MESH_HANDLE_GAP_SERVICE, UUID_SERVICE_GAP),

        // Declare mandatory GAP service characteristic: Dev Name
        CHARACTERISTIC_UUID16(MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_DEV_NAME,
                              MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_DEV_NAME_VAL,
                              UUID_CHARACTERISTIC_DEVICE_NAME,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

        // Declare mandatory GAP service characteristic: Appearance
        CHARACTERISTIC_UUID16(MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_APPEARANCE,
                              MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_APPEARANCE_VAL,
                              UUID_CHARACTERISTIC_APPEARANCE,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

    // Mesh Provisionning Service.
    // This is the mesh application proprietary service. It has
    // characteristics which allows a device to provision a node.
    PRIMARY_SERVICE_UUID16(HANDLE_MESH_SERVICE_PROVISIONING, WICED_BT_MESH_CORE_UUID_SERVICE_PROVISIONING),

        // Handle HANDLE_CHAR_MESH_PROVISIONING_DATA_IN: characteristic Mesh Provisioning Data In
        // Handle HANDLE_CHAR_MESH_PROVISIONING_DATA_IN_VAL: characteristic Mesh Provisioning Data In Value
        // Characteristic is _WRITABLE and it allows writes.
        CHARACTERISTIC_UUID16_WRITABLE(HANDLE_CHAR_MESH_PROVISIONING_DATA_IN,
                                        HANDLE_CHAR_MESH_PROVISIONING_DATA_IN_VALUE,
                                        WICED_BT_MESH_CORE_UUID_CHARACTERISTIC_PROVISIONING_DATA_IN,
                                        GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                        GATTDB_PERM_WRITE_CMD | GATTDB_PERM_VARIABLE_LENGTH),

        // Handle HANDLE_CHAR_MESH_PROVISIONING_DATA_OUT: characteristic Mesh Provisioning Data Out
        // Handle HANDLE_CHAR_MESH_PROVISIONING_DATA_OUT_VAL: characteristic Mesh Provisioning Data Out Value
        // Characteristic can be notified to send provisioning PDU.
        CHARACTERISTIC_UUID16(HANDLE_CHAR_MESH_PROVISIONING_DATA_OUT,
                                        HANDLE_CHAR_MESH_PROVISIONING_DATA_OUT_VALUE,
                                        WICED_BT_MESH_CORE_UUID_CHARACTERISTIC_PROVISIONING_DATA_OUT,
                                        GATTDB_CHAR_PROP_NOTIFY,
                                        GATTDB_PERM_NONE),

            // Handle HANDLE_DESCR_MESH_PROVISIONING_DATA_CLIENT_CONFIG: Characteristic Client Configuration Descriptor.
            // This is standard GATT characteristic descriptor.  2 byte value 0 means that
            // message to the client is disabled.  Peer can write value 1 to enable
            // notifications.  Not _WRITABLE in the macro.  This
            // means that attribute can be written by the peer.
            CHAR_DESCRIPTOR_UUID16_WRITABLE(HANDLE_DESCR_MESH_PROVISIONING_DATA_CLIENT_CONFIG,
                                             UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                             GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ),

    // Handle MESH_HANDLE_DEV_INFO_SERVICE (0x4D): Device Info service
    // Device Information service helps peer to identify manufacture or vendor of the
    // device.  It is required for some types of the devices, for example HID, medical,
    // and optional for others.  There are a bunch of characteristics available.
    PRIMARY_SERVICE_UUID16(MESH_HANDLE_DEV_INFO_SERVICE, UUID_SERVICE_DEVICE_INFORMATION),

        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME:
        //     characteristic Manufacturer Name
        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME_VAL:
        //     characteristic value
        CHARACTERISTIC_UUID16(MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME,
                              MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME_VAL,
                              UUID_CHARACTERISTIC_MANUFACTURER_NAME_STRING,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM:
        //     characteristic Model Number
        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM_VAL:
        //     characteristic value
        CHARACTERISTIC_UUID16(MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM,
                              MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM_VAL,
                              UUID_CHARACTERISTIC_MODEL_NUMBER_STRING,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID: characteristic System ID
        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID_VAL: characteristic value
        CHARACTERISTIC_UUID16(MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID,
                              MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID_VAL,
                              UUID_CHARACTERISTIC_SYSTEM_ID,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

#ifndef MESH_HOMEKIT_COMBO_APP
#ifdef MESH_FW_UPGRADE_UNPROVISIONED
    // Handle 0xff00: Broadcom vendor specific WICED Upgrade Service.
    PRIMARY_SERVICE_UUID128(HANDLE_OTA_FW_UPGRADE_SERVICE, UUID_OTA_FW_UPGRADE_SERVICE),

        // Handles 0xff03: characteristic WS Control Point, handle 0xff04 characteristic value.
        CHARACTERISTIC_UUID128_WRITABLE(HANDLE_OTA_FW_UPGRADE_CHARACTERISTIC_CONTROL_POINT, HANDLE_OTA_FW_UPGRADE_CONTROL_POINT,
            UUID_OTA_FW_UPGRADE_CHARACTERISTIC_CONTROL_POINT, GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY | GATTDB_CHAR_PROP_INDICATE,
            GATTDB_PERM_VARIABLE_LENGTH | GATTDB_PERM_WRITE_REQ /*| GATTDB_PERM_AUTH_WRITABLE*/),

            // Declare client characteristic configuration descriptor
            // Value of the descriptor can be modified by the client
            // Value modified shall be retained during connection and across connection
            // for bonded devices.  Setting value to 1 tells this application to send notification
            // when value of the characteristic changes.  Value 2 is to allow indications.
            CHAR_DESCRIPTOR_UUID16_WRITABLE(HANDLE_OTA_FW_UPGRADE_CLIENT_CONFIGURATION_DESCRIPTOR, UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ /*| GATTDB_PERM_AUTH_WRITABLE */),

        // Handle 0xff07: characteristic WS Data, handle 0xff08 characteristic value. This
        // characteristic is used to send next portion of the FW Similar to the control point
        CHARACTERISTIC_UUID128_WRITABLE(HANDLE_OTA_FW_UPGRADE_CHARACTERISTIC_DATA, HANDLE_OTA_FW_UPGRADE_DATA,
            UUID_OTA_FW_UPGRADE_CHARACTERISTIC_DATA, GATTDB_CHAR_PROP_WRITE,
            GATTDB_PERM_VARIABLE_LENGTH | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_RELIABLE_WRITE /*| GATTDB_PERM_AUTH_WRITABLE */),
#endif
#endif // MESH_HOMEKIT_COMBO_APP
};
const uint32_t gatt_db_unprovisioned_size = sizeof(gatt_db_unprovisioned);

uint8_t gatt_db_provisioned[]=
{
    // Declare mandatory GATT service
    PRIMARY_SERVICE_UUID16(MESH_HANDLE_GATT_SERVICE, UUID_SERVICE_GATT),


    // Handle MESH_HANDLE_GAP_SERVICE (0x14): GAP service
    // Device Name and Appearance are mandatory characteristics.
    PRIMARY_SERVICE_UUID16(MESH_HANDLE_GAP_SERVICE, UUID_SERVICE_GAP),

        // Declare mandatory GAP service characteristic: Dev Name
        CHARACTERISTIC_UUID16(MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_DEV_NAME,
                              MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_DEV_NAME_VAL,
                              UUID_CHARACTERISTIC_DEVICE_NAME,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

        // Declare mandatory GAP service characteristic: Appearance
        CHARACTERISTIC_UUID16(MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_APPEARANCE,
                              MESH_HANDLE_GAP_SERVICE_CHARACTERISTIC_APPEARANCE_VAL,
                              UUID_CHARACTERISTIC_APPEARANCE,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

    PRIMARY_SERVICE_UUID16(HANDLE_MESH_SERVICE_PROXY, WICED_BT_MESH_CORE_UUID_SERVICE_PROXY),

        // Handle HANDLE_CHAR_MESH_PROXY_DATA_IN: characteristic Mesh Proxy In
        // Handle HANDLE_CHAR_MESH_PROXY_DATA_IN_VALUE: characteristic Mesh Proxy In Value
        CHARACTERISTIC_UUID16_WRITABLE(HANDLE_CHAR_MESH_PROXY_DATA_IN,
                                        HANDLE_CHAR_MESH_PROXY_DATA_IN_VALUE,
                                        WICED_BT_MESH_CORE_UUID_CHARACTERISTIC_PROXY_DATA_IN,
                                        GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                        GATTDB_PERM_WRITE_CMD | GATTDB_PERM_VARIABLE_LENGTH),

        // Handle HANDLE_CHAR_MESH_PROXY_DATA_OUT: characteristic Mesh Proxy Out
        // Handle HANDLE_CHAR_MESH_PROXY_DATA_OUT_VALUE: characteristic Mesh Proxy Out Value
        CHARACTERISTIC_UUID16_WRITABLE(HANDLE_CHAR_MESH_PROXY_DATA_OUT,
                                        HANDLE_CHAR_MESH_PROXY_DATA_OUT_VALUE,
                                        WICED_BT_MESH_CORE_UUID_CHARACTERISTIC_PROXY_DATA_OUT,
                                        GATTDB_CHAR_PROP_NOTIFY,
                                        GATTDB_PERM_WRITE_CMD | GATTDB_PERM_VARIABLE_LENGTH),

            // Handle HANDLE_DESCR_MESH_PROXY_DATA_CLIENT_CONFIG: Characteristic Client Configuration Descriptor.
            // This is standard GATT characteristic descriptor.  2 byte value 0 means that
            // message to the client is disabled.  Peer can write value 1 to enable
            // notifications.  Not _WRITABLE in the macro.  This
            // means that attribute can be written by the peer.
            CHAR_DESCRIPTOR_UUID16_WRITABLE (HANDLE_DESCR_MESH_PROXY_DATA_CLIENT_CONFIG,
                                             UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                             GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ),


    // Handle MESH_HANDLE_DEV_INFO_SERVICE (0x4D): Device Info service
    // Device Information service helps peer to identify manufacture or vendor of the
    // device.  It is required for some types of the devices, for example HID, medical,
    // and optional for others.  There are a bunch of characteristics available.
    PRIMARY_SERVICE_UUID16(MESH_HANDLE_DEV_INFO_SERVICE, UUID_SERVICE_DEVICE_INFORMATION),

        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME:
        //     characteristic Manufacturer Name
        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME_VAL:
        //     characteristic value
        CHARACTERISTIC_UUID16(MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME,
                              MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MFR_NAME_VAL,
                              UUID_CHARACTERISTIC_MANUFACTURER_NAME_STRING,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM:
        //     characteristic Model Number
        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM_VAL:
        //     characteristic value
        CHARACTERISTIC_UUID16(MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM,
                              MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_MODEL_NUM_VAL,
                              UUID_CHARACTERISTIC_MODEL_NUMBER_STRING,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID: characteristic System ID
        // Handle MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID_VAL: characteristic value
        CHARACTERISTIC_UUID16(MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID,
                              MESH_HANDLE_DEV_INFO_SERVICE_CHARACTERISTIC_SYSTEM_ID_VAL,
                              UUID_CHARACTERISTIC_SYSTEM_ID,
                              GATTDB_CHAR_PROP_READ, GATTDB_PERM_READABLE),

#ifdef _DEB_COMMAND_SERVICE
    // Handle HANDLE_MESH_SERVICE_COMMAND: Mesh temporary Command Service.
    // This is the mesh application proprietary service. It has
    // characteristics which allows a device to send commands to a node.
    PRIMARY_SERVICE_UUID16(HANDLE_MESH_SERVICE_COMMAND, WICED_BT_MESH_CORE_UUID_SERVICE_COMMAND),

        // Handle HANDLE_CHAR_MESH_COMMAND_DATA: temporary characteristic Mesh Command Data
        // Handle HANDLE_CHAR_MESH_COMMAND_DATA_VAL: temporary characteristic Mesh Command Data Value
        // Characteristic is _WRITABLE and it allows writes.
        CHARACTERISTIC_UUID16_WRITABLE(HANDLE_CHAR_MESH_COMMAND_DATA,
                                        HANDLE_CHAR_MESH_COMMAND_DATA_VALUE,
                                        WICED_BT_MESH_CORE_UUID_CHARACTERISTIC_COMMAND_DATA,
                                        GATTDB_CHAR_PROP_WRITE_NO_RESPONSE | GATTDB_CHAR_PROP_NOTIFY,
                                        GATTDB_PERM_WRITE_CMD | GATTDB_PERM_VARIABLE_LENGTH),

            // Handle HANDLE_DESCR_MESH_COMMAND_DATA_CLIENT_CONFIG: Characteristic Client Configuration Descriptor.
            // This is standard GATT characteristic descriptor.  2 byte value 0 means that
            // message to the client is disabled.  Peer can write value 1 or 2 to enable
            // notifications or indications respectively.  Not _WRITABLE in the macro.  This
            // means that attribute can be written by the peer.
            CHAR_DESCRIPTOR_UUID16_WRITABLE(HANDLE_DESCR_MESH_COMMAND_DATA_CLIENT_CONFIG,
                                             UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                             GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ),
#endif

    // Handle 0xff00: Broadcom vendor specific WICED Upgrade Service.
    PRIMARY_SERVICE_UUID128(HANDLE_OTA_FW_UPGRADE_SERVICE, UUID_OTA_FW_UPGRADE_SERVICE),

        // Handles 0xff03: characteristic WS Control Point, handle 0xff04 characteristic value.
        CHARACTERISTIC_UUID128_WRITABLE(HANDLE_OTA_FW_UPGRADE_CHARACTERISTIC_CONTROL_POINT, HANDLE_OTA_FW_UPGRADE_CONTROL_POINT,
            UUID_OTA_FW_UPGRADE_CHARACTERISTIC_CONTROL_POINT, GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY | GATTDB_CHAR_PROP_INDICATE,
            GATTDB_PERM_VARIABLE_LENGTH | GATTDB_PERM_WRITE_REQ /*| GATTDB_PERM_AUTH_WRITABLE*/),

            // Declare client characteristic configuration descriptor
            // Value of the descriptor can be modified by the client
            // Value modified shall be retained during connection and across connection
            // for bonded devices.  Setting value to 1 tells this application to send notification
            // when value of the characteristic changes.  Value 2 is to allow indications.
            CHAR_DESCRIPTOR_UUID16_WRITABLE(HANDLE_OTA_FW_UPGRADE_CLIENT_CONFIGURATION_DESCRIPTOR, UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ /*| GATTDB_PERM_AUTH_WRITABLE */),

        // Handle 0xff07: characteristic WS Data, handle 0xff08 characteristic value. This
        // characteristic is used to send next portion of the FW Similar to the control point
        CHARACTERISTIC_UUID128_WRITABLE(HANDLE_OTA_FW_UPGRADE_CHARACTERISTIC_DATA, HANDLE_OTA_FW_UPGRADE_DATA,
            UUID_OTA_FW_UPGRADE_CHARACTERISTIC_DATA, GATTDB_CHAR_PROP_WRITE,
            GATTDB_PERM_VARIABLE_LENGTH | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_RELIABLE_WRITE /*| GATTDB_PERM_AUTH_WRITABLE */),

};

const uint32_t gatt_db_provisioned_size = sizeof(gatt_db_provisioned);

/* [] END OF FILE */