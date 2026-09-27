#ifndef VENDOR_SPI_DEMO_DRIVER_BSPDRIVER_SPI_H_
#define VENDOR_SPI_DEMO_DRIVER_BSPDRIVER_SPI_H_

#include "tl_common.h"
#include "spi.h"

typedef enum
{
    BDSID_LSPI,
    BDSID_GSPI,
    BDSID_COUNT
} BSP_DRIVER_SPI_ID;

typedef enum
{
    BDSM_ERROR = -1,
    BDSM_OK = 0,
    BDSM_NONE = BDSM_OK,
    BDSM_TAKEN,
    BDSM_READEN,
    BDSM_WRITEN,
    BDSM_COUNT
} BSP_DRIVER_SPI_MSG;

typedef void (*BSP_DRIVER_SPI_CB)(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_MSG msg);

void BSP_DRIVER_SPI_Init(void);
void BSP_DRIVER_SPI_SetCallback(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_CB cb);

/*
 * Asynchronous DMA transfers.
 * The buffer address must be 4-byte aligned. The byte count may be arbitrary.
 * LSPI is TX-only in this project, therefore BSP_DRIVER_SPI_Read(LSPI, ...)
 * returns an error.
 */
int BSP_DRIVER_SPI_Read(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count);
int BSP_DRIVER_SPI_Write(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count);

/*
 * Blocking DMA transmit for short commands. The exact byte count is sent;
 * the function never adds padding bytes to the SPI transaction.
 */
int BSP_DRIVER_SPI_WriteBlocking(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count);

void BSP_DRIVER_SPI_SetMode(BSP_DRIVER_SPI_ID ID, spi_io_mode_e mode);

/* Temporary compatibility API. No polling is required anymore. */
void BSP_DRIVER_SPI_Poll(void);

void BSP_DRIVER_SPI_Sleep(BSP_DRIVER_SPI_ID ID);
void BSP_DRIVER_SPI_Wakeup(BSP_DRIVER_SPI_ID ID);
bool BSP_DRIVER_SPI_IsSleep(BSP_DRIVER_SPI_ID ID);

#endif
