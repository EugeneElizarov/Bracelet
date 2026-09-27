#ifndef VENDOR_SPI_DEMO_DRIVER_BSPDRIVER_SPI_H_
#define VENDOR_SPI_DEMO_DRIVER_BSPDRIVER_SPI_H_

#include "tl_common.h"
#include "spi.h"

typedef enum
{
  BDSID_LSPI,
  //BDSID_GSPI,
  BDSID_COUNT
}BSP_DRIVER_SPI_ID;

typedef enum
{
  BDSM_ERROR = -1,
  BDSM_OK = 0,
  BDSM_NONE = BDSM_OK,
  BDSM_TAKEN,
  BDSM_READEN,
  BDSM_WRITEN,
  BDSM_COUNT
}BSP_DRIVER_SPI_MSG;

#ifndef BSP_DRIVER_NOWAIT
#define BSP_DRIVER_NOWAIT	0
#endif

#ifndef BSP_DRIVER_WAIT
#define BSP_DRIVER_WAIT		1
#endif

typedef void (* BSP_DRIVER_SPI_CB)(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_MSG msg);

void BSP_DRIVER_SPI_Init(void);
int BSP_DRIVER_SPI_Take(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_CB cb);
void BSP_DRIVER_SPI_Give(BSP_DRIVER_SPI_ID ID);
int BSP_DRIVER_SPI_Read(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count);
int BSP_DRIVER_SPI_Write(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count);
void BSP_DRIVER_SPI_SetMode(BSP_DRIVER_SPI_ID ID, spi_io_mode_e mode);
void BSP_DRIVER_SPI_Poll(void);

void BSP_DRIVER_SPI_Sleep(BSP_DRIVER_SPI_ID ID);
void BSP_DRIVER_SPI_Wakeup(BSP_DRIVER_SPI_ID ID);
bool BSP_DRIVER_SPI_IsSleep(BSP_DRIVER_SPI_ID ID);

#endif
