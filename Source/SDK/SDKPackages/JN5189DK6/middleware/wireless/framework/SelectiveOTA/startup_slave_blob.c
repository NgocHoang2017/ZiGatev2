/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.

* \file
*
* This file is the source file for the startup file that will be used on each slave blob

* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#if defined(__cplusplus)
#ifdef __REDLIB__
#error Redlib does not support C++
#else
//*****************************************************************************
//
// The entry point for the C++ library startup
//
//*****************************************************************************
extern "C" {
extern void __libc_init_array(void);
}
#endif
#endif

#define WEAK __attribute__((weak))
#define ALIAS(f) __attribute__((weak, alias(#f)))

//*****************************************************************************
#if defined(__cplusplus)
extern "C" {
#endif

#include "blob_utils.h"

#define EMPTY_VALUE (void (*const)(void))0xffffffff

//*****************************************************************************
//
// Forward declaration of the default handlers. These are aliased.
// When the application defines a handler (with the same name), this will
// automatically take precedence over these weak definitions
//
//*****************************************************************************
void ResetISR(BlobResetMode_t eResetMode);
void DataUsage(BlobResetMode_t eResetMode, void (*prRegionDefine)(uint32_t, uint32_t));

WEAK void pvAPI_JumpTable(void);

//*****************************************************************************
//
// The entry point for all blob slave
//
//*****************************************************************************
extern void BLOBS_Init(BlobResetMode_t eResetMode);

//*****************************************************************************
#if defined(__cplusplus)
} // extern "C"
#endif
//*****************************************************************************
//
// The vector table.
// This relies on the linker script to place at correct location in memory.
//
//*****************************************************************************
extern void (*const g_pfnVectors[72])(void);
__attribute__((used, section(".isr_vector"))) void (*const g_pfnVectors[72])(void) = {
    // Core Level - CM4
    EMPTY_VALUE,        // The initial stack pointer
    (void (*)(void))ResetISR, // The reset handler
    (void (*)(void))DataUsage, // Returns RAM data usage, for power management
    EMPTY_VALUE,        // The hard fault handler
    EMPTY_VALUE,        // The MPU fault handler
    EMPTY_VALUE,        // The bus fault handler
    EMPTY_VALUE,        // The usage fault handler
    0,                  // Reserved: Post-build step (jn518x_image_tool.py)
    0,                  // Reserved: Post-build step (jn518x_image_tool.py)
    0,                  // Reserved: Post-build step (jn518x_image_tool.py)
    0,                  // Reserved: Post-build step (jn518x_image_tool.py)
    EMPTY_VALUE,        // SVCall handler
    EMPTY_VALUE,        // Debug monitor handler
    pvAPI_JumpTable,    // Reserved: Function look-up table
}; /* End of g_pfnVectors */

//*****************************************************************************
// Functions to carry out the initialization of RW and BSS data sections. These
// are written as separate functions rather than being inlined within the
// ResetISR() function in order to cope with MCUs with multiple banks of
// memory.
//*****************************************************************************
__attribute__((section(".after_vectors"))) void data_init(unsigned int romstart, unsigned int start, unsigned int len)
{
    unsigned int *pulDest = (unsigned int *)start;
    unsigned int *pulSrc = (unsigned int *)romstart;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = *pulSrc++;
}

__attribute__((section(".after_vectors"))) void bss_init(unsigned int start, unsigned int len)
{
    unsigned int *pulDest = (unsigned int *)start;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = 0;
}

//*****************************************************************************
// The following symbols are constructs generated by the linker, indicating
// the location of various points in the "Global Section Table". This table is
// created by the linker via the Code Red managed linker script mechanism. It
// contains the load address, execution address and length of each RW data
// section and the execution and length of each BSS (zero initialized) section.
//*****************************************************************************
extern unsigned int __data_section_table;
extern __attribute__((weak)) unsigned int __data_discard_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern __attribute__((weak)) unsigned int __bss_discard_section_table;
extern unsigned int __bss_section_table_end;

//*****************************************************************************
// Reset entry point for your code.
// Sets up a simple runtime environment and initializes the C/C++
// library.
//*****************************************************************************

__attribute__((used, section(".after_vectors.reset"))) void ResetISR(BlobResetMode_t eResetMode)
{
    unsigned int LoadAddr, ExeAddr, SectionLen;
    unsigned int *SectionTableAddr;

    if (E_RESET_WARM == eResetMode)
    {
        if (&__data_discard_section_table)
        {
            /* Load discard sections base address of Global Section Table A */
            SectionTableAddr = &__data_discard_section_table;
        }
        else
        {
            /* Nothing to copy, so set to end of table */
            SectionTableAddr = &__data_section_table_end;
        }
    }
    else
    {
        /* Load base address of Global Section Table A */
        SectionTableAddr = &__data_section_table;
    }

    /* Copy the data sections from flash to SRAM */
    while (SectionTableAddr < &__data_section_table_end)
    {
        LoadAddr = *SectionTableAddr++;
        ExeAddr = *SectionTableAddr++;
        SectionLen = *SectionTableAddr++;
        data_init(LoadAddr, ExeAddr, SectionLen);
    }

    if (E_RESET_WARM == eResetMode)
    {
        if (&__bss_discard_section_table)
        {
            /* Load discard sections base address of Global Section Table B */
            SectionTableAddr = &__bss_discard_section_table;
        }
        else
        {
            /* Nothing to copy */
            SectionTableAddr = &__bss_section_table_end;
        }
    }
    else
    {
        /* Load base address of Global Section Table B */
        SectionTableAddr = &__bss_section_table;
    }

    /* Zero fill the bss sections */
    while (SectionTableAddr < &__bss_section_table_end)
    {
        ExeAddr = *SectionTableAddr++;
        SectionLen = *SectionTableAddr++;
        bss_init(ExeAddr, SectionLen);
    }

    BLOBS_Init(eResetMode);
}

//*****************************************************************************
// Utility to return the location and size of each RAM region that should be
// saved through sleep, based on the reset mode. Traverses through each region
// definition in the section tables, calling out to the provided function
// callback each time with the start and length parameters
//*****************************************************************************
void DataUsage(BlobResetMode_t eResetMode, void (*prRegionDefine)(uint32_t, uint32_t))
{
    unsigned int u32StartAddr, u32SectionLen;
    unsigned int *psSectionTableAddr;
    unsigned int *pu32DataEnd;
    unsigned int *pu32BssEnd;

    /* Only retain data for warm sleep */
    if (E_RESET_WARM == eResetMode)
    {
        /* Only need to retain non-discarded data, so set end to start of
         * discarded data, if present */
        if (&__data_discard_section_table)
        {
            pu32DataEnd = &__data_discard_section_table;
        }
        else
        {
            pu32DataEnd = &__data_section_table_end;
        }

        if (&__bss_discard_section_table)
        {
            pu32BssEnd = &__bss_discard_section_table;
        }
        else
        {
            pu32BssEnd = &__bss_section_table_end;
        }

        // Load base address of Global Section Table
        psSectionTableAddr = &__data_section_table;

        // Note each region's start and length
        while (psSectionTableAddr < pu32DataEnd)
        {
            psSectionTableAddr++; /* Pass over load address */
            u32StartAddr = *psSectionTableAddr++;
            u32SectionLen = *psSectionTableAddr++;
            prRegionDefine(u32StartAddr, u32SectionLen);
        }

        // Load base address of Global Section Table B
        psSectionTableAddr = &__bss_section_table;

        // Note each region's start and length
        while (psSectionTableAddr < pu32BssEnd)
        {
            u32StartAddr = *psSectionTableAddr++;
            u32SectionLen = *psSectionTableAddr++;
            prRegionDefine(u32StartAddr, u32SectionLen);
        }
    }
}