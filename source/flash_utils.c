/*******************************************************************************
* File Name: flash_utils.c
*
* Description: This file contains flash memory access and helper functions.
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
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include "cycfg_qspi_memslot.h"
#include "wiced_memory.h"
#include "mtb_kvstore.h"
#include "stdlib.h"
#include "flash_utils.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define FLASH_KEY_SIZE                      (8u)
#define FLASH_KEY_BASE                      (16u) /* Hexadecimal */
#define FLASH_CONFIG_MAX_LEN                (1048)

#define QSPI_BUS_FREQ                       (50000000l)
#define QSPI_GET_ERASE_SIZE                 (0u)

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
uint32_t bd_read_size(void* context, uint32_t addr);
uint32_t bd_program_size(void* context, uint32_t addr);
uint32_t bd_erase_size(void* context, uint32_t addr);
cy_rslt_t bd_read(void* context, uint32_t addr, uint32_t length, uint8_t* buf);
cy_rslt_t bd_program(void* context, uint32_t addr, uint32_t length, const uint8_t* buf);
cy_rslt_t bd_erase(void* context, uint32_t addr, uint32_t length);
/*******************************************************************************
* Global Variables
*******************************************************************************/
mtb_kvstore_t kv_store_obj;

cy_stc_smif_context_t SMIFContext;

/*Kvstore block device*/

mtb_kvstore_bd_t block_device =
{
    .read         = bd_read,
    .program      = bd_program,
    .erase        = bd_erase,
    .read_size    = bd_read_size,
    .program_size = bd_program_size,
    .erase_size   = bd_erase_size,
    .context      = NULL
};

/*******************************************************************************
* Function Name: flash_memory_init
********************************************************************************
* Summary:
* This function Initialize the flash memory.
*
* Parameters:
*  None
*
* Return:
*  cy_rslt_t : returns the result status.
*
*******************************************************************************/
cy_rslt_t flash_memory_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    /*Define the space to be used Storage*/
    uint32_t length=0u;
    uint32_t sector_size = 0u;
    uint32_t start_addr = 0;

    /* Initialize the SMIF*/
    result = cybsp_smif_init();

    /*Check if the smif initialization was successful */
    if(CY_RSLT_SUCCESS != result)
    {
        printf("External flash initialization failed \r\n");
        CY_ASSERT(0);
    }

    /*Define the space to be used for Bond Data Storage*/
    sector_size = (size_t)smifBlockConfig.memConfig[0]->deviceCfg->eraseSize;
    length = sector_size * 4;

    /* If the device is not a hybrid memory, use last sector to erase since
      * first sector has some configuration data used during boot from
      * flash operation.
      */
     if (0u == smifMemConfigs[0]->deviceCfg->hybridRegionCount)
     {
         start_addr = (smifMemConfigs[0]->deviceCfg->memSize - sector_size* 4);
     }

    /*Initialize kv-store library*/
    result = mtb_kvstore_init(&kv_store_obj, start_addr, length, &block_device);

    /*Check if the kv-store initialization was successful*/
    if (CY_RSLT_SUCCESS !=  result)
    {
        printf("Kv-store initialization failed with error code = %x\r\n", (int)result);
        CY_ASSERT(0);
    }
    else
    {
        printf("Kv-store initialization success with starting address = 0x%x\r\n", (unsigned int)start_addr);
    }

    return result;
}


/*******************************************************************************
* Function Name: flash_memory_read
********************************************************************************
* Summary:
* This function reads data from flash memory.
*
* Parameters:
*  config_item_id : index of data
*  len :data length
*  buf : data buffer
*
* Return:
*  uint32_t : returns the length of data size.
*
*******************************************************************************/
uint16_t flash_memory_read(uint16_t config_item_id, uint32_t len, uint8_t* buf, wiced_result_t *rslt)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    char key[]="0000";

    itoa(config_item_id, key, FLASH_KEY_BASE);

    if(CY_RSLT_SUCCESS != mtb_kvstore_key_exists(&kv_store_obj, key))
    {
        *rslt = WICED_SUCCESS;
        return 0;
    }

    result = mtb_kvstore_read(&kv_store_obj, key, (uint8_t*)buf, &len);
    if(CY_RSLT_SUCCESS != result)
    {
        printf("Flash read failed with error code : 0x%x\r\n", (int)result);
        *rslt = WICED_SUCCESS;
        return 0;
    }
    *rslt = WICED_SUCCESS;
    return ((uint16_t)len);
}


/*******************************************************************************
* Function Name: flash_memory_write
********************************************************************************
* Summary:
* This function write data to flash memory.
*
* Parameters:
*  config_item_id : index of data
*  len :data length
*  buf : data buffer
*
* Return:
*  uint32_t : returns the length of data size.
*
*******************************************************************************/
uint16_t flash_memory_write(uint16_t config_item_id, uint32_t len, uint8_t* buf, wiced_result_t *rslt)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    char key[]="0000";

    itoa(config_item_id, key, FLASH_KEY_BASE);

    result = mtb_kvstore_write(&kv_store_obj, key, (uint8_t*)buf, len);
    if(CY_RSLT_SUCCESS != result)
    {
        printf("Flash write failed with error code: 0x%x\r\n", (int)result);
        *rslt = WICED_SUCCESS;
        return 0;
    }
    *rslt = WICED_SUCCESS;
    return (uint16_t) len;
}


/*******************************************************************************
* Function Name: flash_memory_delete
********************************************************************************
* Summary:
* This function delete data from the flash memory.
*
* Parameters:
*  config_item_id : index of data
*
* Return:
*  uint32_t : returns the status.
*
*******************************************************************************/
cy_rslt_t flash_memory_delete(uint16_t config_item_id)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    char key[FLASH_KEY_SIZE]={'\0'};

    itoa(config_item_id, key, FLASH_KEY_BASE);

    result = mtb_kvstore_delete(&kv_store_obj, key);
    if(CY_RSLT_SUCCESS != result)
    {
        printf("Flash delete failed with error code: 0x%x\r\n", (int)result);
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}


/*******************************************************************************
* Function Name: flash_memory_reset
********************************************************************************
* Summary:
* This function reset data from the flash memory.
*
* Parameters:
*  None
*
* Return:
*  uint32_t : returns the status.
*
*******************************************************************************/
cy_rslt_t flash_memory_reset(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result = mtb_kvstore_reset(&kv_store_obj);
    if(CY_RSLT_SUCCESS != result)
    {
        printf("Flash reset failed with error code: 0x%x\r\n", (int)result);
        return WICED_ERROR;
    }
    return (result);
}

/*******************************************************************************
* Function Name: bd_read_size
********************************************************************************
* Summary:
* This function read block data size.
*
* Parameters:
*  context : block data context
*  addr : block data address
*
* Return:
*  uint32_t : TRUE.
*
*******************************************************************************/
uint32_t bd_read_size(void* context, uint32_t addr)
{
    (void)context;
    (void)addr;
    return 1u;
}


/*******************************************************************************
* Function Name: bd_program_size
********************************************************************************
* Summary:
* This function program the block.
*
* Parameters:
*  context : block data context
*  addr : block data address
*
* Return:
*  uint32_t : block page size.
*
*******************************************************************************/
uint32_t bd_program_size(void* context, uint32_t addr)
{
    (void)context;

    CY_UNUSED_PARAMETER(addr);
    return (size_t)smifBlockConfig.memConfig[0]->deviceCfg->programSize;
}


/*******************************************************************************
* Function Name: bd_erase_size
********************************************************************************
* Summary:
* This function erase the block data.
*
* Parameters:
*  context : block data context
*  addr : block data address
*
* Return:
*  uint32_t : block sector size.
*
*******************************************************************************/
uint32_t bd_erase_size(void* context, uint32_t addr)
{
    (void)context;

    size_t                            erase_sector_size;
    cy_stc_smif_hybrid_region_info_t* hybrid_info = NULL;

    cy_en_smif_status_t smif_status =
        Cy_SMIF_MemLocateHybridRegion(smifBlockConfig.memConfig[0], &hybrid_info, addr);

    if (CY_SMIF_SUCCESS != smif_status)
    {
        erase_sector_size = (size_t)smifBlockConfig.memConfig[0]->deviceCfg->eraseSize;
    }
    else
    {
        erase_sector_size = (size_t)hybrid_info->eraseSize;
    }

    return erase_sector_size;
}


/*******************************************************************************
* Function Name: bd_read
********************************************************************************
* Summary:
* This function read block data.
*
* Parameters:
*  context : block data context
*  addr : block data address
*
* Return:
*  uint32_t : TRUE.
*
*******************************************************************************/
cy_rslt_t bd_read(void* context, uint32_t addr, uint32_t length, uint8_t* buf)
{
    (void)context;

    cy_rslt_t result = 0;
    // Cy_SMIF_MemRead() returns error if (addr + length) > total flash size.
    result = (cy_rslt_t)Cy_SMIF_MemRead(SMIF0, smifBlockConfig.memConfig[0],
            addr,
            buf, length, &cybsp_smif_context);

    return result;
}


/*******************************************************************************
* Function Name: bd_program
********************************************************************************
* Summary:
* This function program the block data.
*
* Parameters:
*  context : block data context
*  addr : block data address
*  length : block data length
*  buf : block data buffer
*
* Return:
*  cy_rslt_t : status.
*
*******************************************************************************/
cy_rslt_t bd_program(void* context, uint32_t addr, uint32_t length, const uint8_t* buf)
{
    (void)context;
    
    cy_rslt_t result = 0;
    // Cy_SMIF_MemWrite() returns error if (addr + length) > total flash size.
    result = (cy_rslt_t)Cy_SMIF_MemWrite(SMIF0, smifBlockConfig.memConfig[0],
            addr,
            (uint8_t*)buf, length, &cybsp_smif_context);

    return result;
}


/*******************************************************************************
* Function Name: bd_erase
********************************************************************************
* Summary:
* This function eraze the block data.
*
* Parameters:
*  context : block data context
*  addr : block data address
*  length : block data length
*
* Return:
*  cy_rslt_t : status.
*
*******************************************************************************/
cy_rslt_t bd_erase(void* context, uint32_t addr, uint32_t length)
{
    (void)context;
    
    cy_rslt_t result = 0;
    // If the erase is for the entire chip, use chip erase command
    if ((addr == 0u) && (length == (size_t)smifBlockConfig.memConfig[0]->deviceCfg->memSize))
    {
        result =
                (cy_rslt_t)Cy_SMIF_MemEraseChip(SMIF0,
                        smifBlockConfig.memConfig[0],
                        &cybsp_smif_context);
    }
    else
    {
        // Cy_SMIF_MemEraseSector() returns error if (addr + length) > total flash size or if
        // addr is not aligned to erase sector size or if (addr + length) is not aligned to
        // erase sector size.
        result =
                (cy_rslt_t)Cy_SMIF_MemEraseSector(SMIF0,
                        smifBlockConfig.memConfig[0],
                        addr, length, &cybsp_smif_context);
    }

    return result;
}

/* [] END OF FILE */
