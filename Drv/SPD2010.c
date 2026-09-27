#include "SPD2010.h"
#include <stdbool.h>
#include "../def.h"

#define DISPLAY_CS					GPIO_PE0
#define DISPLAY_RESET				GPIO_PE6

#define DISPLAY_WIDTH				412
#define DISPLAY_HEIGHT				412

#define LCD_OPCODE_WRITE_CMD        (0x02ULL)
#define LCD_OPCODE_READ_CMD         (0x0BULL)
#define LCD_OPCODE_WRITE_COLOR      (0x32ULL)

#define SPD2010_CMD_SET             (0xFF)
#define SPD2010_CMD_SET_BYTE0       (0x20)
#define SPD2010_CMD_SET_BYTE1       (0x10)
#define SPD2010_CMD_SET_USER        (0x00)






typedef struct {
    int cmd;                /*<! The specific LCD command */
    void *data;       /*<! Buffer that holds the command specific data */
    size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} spd2010_lcd_init_cmd_t;

static void _set_cs_active(bool active)
{
  if (BSP_DRIVER_SPI_IsSleep(BDSID_LSPI))
	  return;
  gpio_set_level(DISPLAY_CS, active ? 0 : 1);
}

static DRIVER_SPD2010_WRITE_CB _write_callback;
static volatile bool _async_write_active;

static void _spi_callback(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_MSG msg)
{
  if (ID != BDSID_LSPI || msg != BDSM_WRITEN)
    return;

  if (!_async_write_active)
    return;

  _async_write_active = false;
  _set_cs_active(false);

  if (_write_callback)
    _write_callback();
}

void DRIVER_SPD2010_SetWriteCallback(DRIVER_SPD2010_WRITE_CB cb)
{
  _write_callback = cb;
}

static void _display_reset(void)
{
  if (BSP_DRIVER_SPI_IsSleep(BDSID_LSPI))
	  return;
  delay_ms(10);
  gpio_set_low_level(DISPLAY_RESET);
  delay_ms(10);
  gpio_set_high_level(DISPLAY_RESET);
  delay_ms(30);
  tlk_printf("Dspl rst\n");
}

static void _tx_command(uint8_t command, void *cmd_data, uint8_t cmd_data_len)
{
	uint8_t cmd[4] = {2, 0, command, 0};
	BSP_DRIVER_SPI_SetMode(BDSID_LSPI, SPI_SINGLE_MODE);
	_set_cs_active(true);
	BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, cmd, 1);
	BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, &cmd[1], 3);
	if (cmd_data_len)
	{
		//if (cmd[0] != 0x02)
			//BSP_DRIVER_SPI_SetMode(BDSID_LSPI, SPI_QUAD_MODE);
		BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, cmd_data, cmd_data_len);
	}
	_set_cs_active(false);
}

static void _tx_color(bool start_write, TDisplayColor *cmd_data, uint32_t cmd_data_len, bool blocking)
{
  uint8_t cmd[4] = {0x32, 0, start_write ? 0x2C : 0x3C, 0};

  BSP_DRIVER_SPI_SetMode(BDSID_LSPI, SPI_SINGLE_MODE);
  _set_cs_active(true);

  if (BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, cmd, 1) < 0 ||
      BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, &cmd[1], 3) < 0)
  {
    _set_cs_active(false);
    if (_write_callback)
      _write_callback();
    return;
  }

  if (!cmd_data_len)
  {
    _set_cs_active(false);
    if (_write_callback)
      _write_callback();
    return;
  }

  if (blocking)
  {
    BSP_DRIVER_SPI_SetMode(BDSID_LSPI, SPI_QUAD_MODE);
    BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, cmd_data, cmd_data_len);
    _set_cs_active(false);
    return;
  }

  BSP_DRIVER_SPI_SetMode(BDSID_LSPI, SPI_QUAD_MODE);
  _async_write_active = true;

  if (BSP_DRIVER_SPI_Write(BDSID_LSPI, cmd_data, cmd_data_len) < 0)
  {
    _async_write_active = false;
    _set_cs_active(false);
    BSP_DRIVER_SPI_WriteBlocking(BDSID_LSPI, cmd_data, cmd_data_len);
    if (_write_callback)
      _write_callback();
  }
}

static const spd2010_lcd_init_cmd_t vendor_specific_init_default[] = {
//  {cmd, { data }, data_size, delay_ms}
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x0C, (uint8_t []){0x11}, 1, 0},
    {0x10, (uint8_t []){0x02}, 1, 0},
    {0x11, (uint8_t []){0x11}, 1, 0},
    {0x15, (uint8_t []){0x42}, 1, 0},
    {0x16, (uint8_t []){0x11}, 1, 0},
    {0x1A, (uint8_t []){0x02}, 1, 0},
    {0x1B, (uint8_t []){0x11}, 1, 0},
    {0x61, (uint8_t []){0x80}, 1, 0},
    {0x62, (uint8_t []){0x80}, 1, 0},
    {0x54, (uint8_t []){0x44}, 1, 0},
    {0x58, (uint8_t []){0x88}, 1, 0},
    {0x5C, (uint8_t []){0xcc}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x20, (uint8_t []){0x80}, 1, 0},
    {0x21, (uint8_t []){0x81}, 1, 0},
    {0x22, (uint8_t []){0x31}, 1, 0},
    {0x23, (uint8_t []){0x20}, 1, 0},
    {0x24, (uint8_t []){0x11}, 1, 0},
    {0x25, (uint8_t []){0x11}, 1, 0},
    {0x26, (uint8_t []){0x12}, 1, 0},
    {0x27, (uint8_t []){0x12}, 1, 0},
    {0x30, (uint8_t []){0x80}, 1, 0},
    {0x31, (uint8_t []){0x81}, 1, 0},
    {0x32, (uint8_t []){0x31}, 1, 0},
    {0x33, (uint8_t []){0x20}, 1, 0},
    {0x34, (uint8_t []){0x11}, 1, 0},
    {0x35, (uint8_t []){0x11}, 1, 0},
    {0x36, (uint8_t []){0x12}, 1, 0},
    {0x37, (uint8_t []){0x12}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x41, (uint8_t []){0x11}, 1, 0},
    {0x42, (uint8_t []){0x22}, 1, 0},
    {0x43, (uint8_t []){0x33}, 1, 0},
    {0x49, (uint8_t []){0x11}, 1, 0},
    {0x4A, (uint8_t []){0x22}, 1, 0},
    {0x4B, (uint8_t []){0x33}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x15}, 3, 0},
    {0x00, (uint8_t []){0x00}, 1, 0},
    {0x01, (uint8_t []){0x00}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x00}, 1, 0},
    {0x04, (uint8_t []){0x10}, 1, 0},
    {0x05, (uint8_t []){0x0C}, 1, 0},
    {0x06, (uint8_t []){0x23}, 1, 0},
    {0x07, (uint8_t []){0x22}, 1, 0},
    {0x08, (uint8_t []){0x21}, 1, 0},
    {0x09, (uint8_t []){0x20}, 1, 0},
    {0x0A, (uint8_t []){0x33}, 1, 0},
    {0x0B, (uint8_t []){0x32}, 1, 0},
    {0x0C, (uint8_t []){0x34}, 1, 0},
    {0x0D, (uint8_t []){0x35}, 1, 0},
    {0x0E, (uint8_t []){0x01}, 1, 0},
    {0x0F, (uint8_t []){0x01}, 1, 0},
    {0x20, (uint8_t []){0x00}, 1, 0},
    {0x21, (uint8_t []){0x00}, 1, 0},
    {0x22, (uint8_t []){0x00}, 1, 0},
    {0x23, (uint8_t []){0x00}, 1, 0},
    {0x24, (uint8_t []){0x0C}, 1, 0},
    {0x25, (uint8_t []){0x10}, 1, 0},
    {0x26, (uint8_t []){0x20}, 1, 0},
    {0x27, (uint8_t []){0x21}, 1, 0},
    {0x28, (uint8_t []){0x22}, 1, 0},
    {0x29, (uint8_t []){0x23}, 1, 0},
    {0x2A, (uint8_t []){0x33}, 1, 0},
    {0x2B, (uint8_t []){0x32}, 1, 0},
    {0x2C, (uint8_t []){0x34}, 1, 0},
    {0x2D, (uint8_t []){0x35}, 1, 0},
    {0x2E, (uint8_t []){0x01}, 1, 0},
    {0x2F, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x16}, 3, 0},
    {0x00, (uint8_t []){0x00}, 1, 0},
    {0x01, (uint8_t []){0x00}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x00}, 1, 0},
    {0x04, (uint8_t []){0x08}, 1, 0},
    {0x05, (uint8_t []){0x04}, 1, 0},
    {0x06, (uint8_t []){0x19}, 1, 0},
    {0x07, (uint8_t []){0x18}, 1, 0},
    {0x08, (uint8_t []){0x17}, 1, 0},
    {0x09, (uint8_t []){0x16}, 1, 0},
    {0x0A, (uint8_t []){0x33}, 1, 0},
    {0x0B, (uint8_t []){0x32}, 1, 0},
    {0x0C, (uint8_t []){0x34}, 1, 0},
    {0x0D, (uint8_t []){0x35}, 1, 0},
    {0x0E, (uint8_t []){0x01}, 1, 0},
    {0x0F, (uint8_t []){0x01}, 1, 0},
    {0x20, (uint8_t []){0x00}, 1, 0},
    {0x21, (uint8_t []){0x00}, 1, 0},
    {0x22, (uint8_t []){0x00}, 1, 0},
    {0x23, (uint8_t []){0x00}, 1, 0},
    {0x24, (uint8_t []){0x04}, 1, 0},
    {0x25, (uint8_t []){0x08}, 1, 0},
    {0x26, (uint8_t []){0x16}, 1, 0},
    {0x27, (uint8_t []){0x17}, 1, 0},
    {0x28, (uint8_t []){0x18}, 1, 0},
    {0x29, (uint8_t []){0x19}, 1, 0},
    {0x2A, (uint8_t []){0x33}, 1, 0},
    {0x2B, (uint8_t []){0x32}, 1, 0},
    {0x2C, (uint8_t []){0x34}, 1, 0},
    {0x2D, (uint8_t []){0x35}, 1, 0},
    {0x2E, (uint8_t []){0x01}, 1, 0},
    {0x2F, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x00, (uint8_t []){0x99}, 1, 0},
    {0x2A, (uint8_t []){0x28}, 1, 0},
    {0x2B, (uint8_t []){0x0f}, 1, 0},
    {0x2C, (uint8_t []){0x16}, 1, 0},
    {0x2D, (uint8_t []){0x28}, 1, 0},
    {0x2E, (uint8_t []){0x0f}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0xA0}, 3, 0},
    {0x08, (uint8_t []){0xdc}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x45}, 3, 0},
    {0x01, (uint8_t []){0x9C}, 1, 0},
    {0x03, (uint8_t []){0x9C}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x42}, 3, 0},
    {0x05, (uint8_t []){0x2c}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x50, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0x2A, (uint8_t []){0x00, 0x00, DISPLAY_WIDTH >> 8, DISPLAY_WIDTH & 0xFF}, 4, 0},
    {0x2B, (uint8_t []){0x00, 0x00, DISPLAY_HEIGHT >> 8, DISPLAY_HEIGHT & 0xFF}, 4, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x40}, 3, 0},
    {0x86, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x0D, (uint8_t []){0x66}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x17}, 3, 0},
    {0x39, (uint8_t []){0x3c}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x31}, 3, 0},
    {0x38, (uint8_t []){0x03}, 1, 0},
    {0x39, (uint8_t []){0xf0}, 1, 0},
    {0x36, (uint8_t []){0x03}, 1, 0},
    {0x37, (uint8_t []){0xe8}, 1, 0},
    {0x34, (uint8_t []){0x03}, 1, 0},
    {0x35, (uint8_t []){0xCF}, 1, 0},
    {0x32, (uint8_t []){0x03}, 1, 0},
    {0x33, (uint8_t []){0xBA}, 1, 0},
    {0x30, (uint8_t []){0x03}, 1, 0},
    {0x31, (uint8_t []){0xA2}, 1, 0},
    {0x2e, (uint8_t []){0x03}, 1, 0},
    {0x2f, (uint8_t []){0x95}, 1, 0},
    {0x2c, (uint8_t []){0x03}, 1, 0},
    {0x2d, (uint8_t []){0x7e}, 1, 0},
    {0x2a, (uint8_t []){0x03}, 1, 0},
    {0x2b, (uint8_t []){0x62}, 1, 0},
    {0x28, (uint8_t []){0x03}, 1, 0},
    {0x29, (uint8_t []){0x44}, 1, 0},
    {0x26, (uint8_t []){0x02}, 1, 0},
    {0x27, (uint8_t []){0xfc}, 1, 0},
    {0x24, (uint8_t []){0x02}, 1, 0},
    {0x25, (uint8_t []){0xd0}, 1, 0},
    {0x22, (uint8_t []){0x02}, 1, 0},
    {0x23, (uint8_t []){0x98}, 1, 0},
    {0x20, (uint8_t []){0x02}, 1, 0},
    {0x21, (uint8_t []){0x6f}, 1, 0},
    {0x1e, (uint8_t []){0x02}, 1, 0},
    {0x1f, (uint8_t []){0x32}, 1, 0},
    {0x1c, (uint8_t []){0x01}, 1, 0},
    {0x1d, (uint8_t []){0xf6}, 1, 0},
    {0x1a, (uint8_t []){0x01}, 1, 0},
    {0x1b, (uint8_t []){0xb8}, 1, 0},
    {0x18, (uint8_t []){0x01}, 1, 0},
    {0x19, (uint8_t []){0x6E}, 1, 0},
    {0x16, (uint8_t []){0x01}, 1, 0},
    {0x17, (uint8_t []){0x41}, 1, 0},
    {0x14, (uint8_t []){0x00}, 1, 0},
    {0x15, (uint8_t []){0xfd}, 1, 0},
    {0x12, (uint8_t []){0x00}, 1, 0},
    {0x13, (uint8_t []){0xCf}, 1, 0},
    {0x10, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x98}, 1, 0},
    {0x0e, (uint8_t []){0x00}, 1, 0},
    {0x0f, (uint8_t []){0x89}, 1, 0},
    {0x0c, (uint8_t []){0x00}, 1, 0},
    {0x0d, (uint8_t []){0x79}, 1, 0},
    {0x0a, (uint8_t []){0x00}, 1, 0},
    {0x0b, (uint8_t []){0x67}, 1, 0},
    {0x08, (uint8_t []){0x00}, 1, 0},
    {0x09, (uint8_t []){0x55}, 1, 0},
    {0x06, (uint8_t []){0x00}, 1, 0},
    {0x07, (uint8_t []){0x3F}, 1, 0},
    {0x04, (uint8_t []){0x00}, 1, 0},
    {0x05, (uint8_t []){0x28}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x0E}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x32}, 3, 0},
    {0x38, (uint8_t []){0x03}, 1, 0},
    {0x39, (uint8_t []){0xf0}, 1, 0},
    {0x36, (uint8_t []){0x03}, 1, 0},
    {0x37, (uint8_t []){0xe8}, 1, 0},
    {0x34, (uint8_t []){0x03}, 1, 0},
    {0x35, (uint8_t []){0xCF}, 1, 0},
    {0x32, (uint8_t []){0x03}, 1, 0},
    {0x33, (uint8_t []){0xBA}, 1, 0},
    {0x30, (uint8_t []){0x03}, 1, 0},
    {0x31, (uint8_t []){0xA2}, 1, 0},
    {0x2e, (uint8_t []){0x03}, 1, 0},
    {0x2f, (uint8_t []){0x95}, 1, 0},
    {0x2c, (uint8_t []){0x03}, 1, 0},
    {0x2d, (uint8_t []){0x7e}, 1, 0},
    {0x2a, (uint8_t []){0x03}, 1, 0},
    {0x2b, (uint8_t []){0x62}, 1, 0},
    {0x28, (uint8_t []){0x03}, 1, 0},
    {0x29, (uint8_t []){0x44}, 1, 0},
    {0x26, (uint8_t []){0x02}, 1, 0},
    {0x27, (uint8_t []){0xfc}, 1, 0},
    {0x24, (uint8_t []){0x02}, 1, 0},
    {0x25, (uint8_t []){0xd0}, 1, 0},
    {0x22, (uint8_t []){0x02}, 1, 0},
    {0x23, (uint8_t []){0x98}, 1, 0},
    {0x20, (uint8_t []){0x02}, 1, 0},
    {0x21, (uint8_t []){0x6f}, 1, 0},
    {0x1e, (uint8_t []){0x02}, 1, 0},
    {0x1f, (uint8_t []){0x32}, 1, 0},
    {0x1c, (uint8_t []){0x01}, 1, 0},
    {0x1d, (uint8_t []){0xf6}, 1, 0},
    {0x1a, (uint8_t []){0x01}, 1, 0},
    {0x1b, (uint8_t []){0xb8}, 1, 0},
    {0x18, (uint8_t []){0x01}, 1, 0},
    {0x19, (uint8_t []){0x6E}, 1, 0},
    {0x16, (uint8_t []){0x01}, 1, 0},
    {0x17, (uint8_t []){0x41}, 1, 0},
    {0x14, (uint8_t []){0x00}, 1, 0},
    {0x15, (uint8_t []){0xfd}, 1, 0},
    {0x12, (uint8_t []){0x00}, 1, 0},
    {0x13, (uint8_t []){0xCf}, 1, 0},
    {0x10, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x98}, 1, 0},
    {0x0e, (uint8_t []){0x00}, 1, 0},
    {0x0f, (uint8_t []){0x89}, 1, 0},
    {0x0c, (uint8_t []){0x00}, 1, 0},
    {0x0d, (uint8_t []){0x79}, 1, 0},
    {0x0a, (uint8_t []){0x00}, 1, 0},
    {0x0b, (uint8_t []){0x67}, 1, 0},
    {0x08, (uint8_t []){0x00}, 1, 0},
    {0x09, (uint8_t []){0x55}, 1, 0},
    {0x06, (uint8_t []){0x00}, 1, 0},
    {0x07, (uint8_t []){0x3F}, 1, 0},
    {0x04, (uint8_t []){0x00}, 1, 0},
    {0x05, (uint8_t []){0x28}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x0E}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x60, (uint8_t []){0x01}, 1, 0},
    {0x65, (uint8_t []){0x03}, 1, 0},
    {0x66, (uint8_t []){0x38}, 1, 0},
    {0x67, (uint8_t []){0x04}, 1, 0},
    {0x68, (uint8_t []){0x34}, 1, 0},
    {0x69, (uint8_t []){0x03}, 1, 0},
    {0x61, (uint8_t []){0x03}, 1, 0},
    {0x62, (uint8_t []){0x38}, 1, 0},
    {0x63, (uint8_t []){0x04}, 1, 0},
    {0x64, (uint8_t []){0x34}, 1, 0},
    {0x0A, (uint8_t []){0x11}, 1, 0},
    {0x0B, (uint8_t []){0x20}, 1, 0},
    {0x0c, (uint8_t []){0x20}, 1, 0},
    {0x55, (uint8_t []){0x06}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x42}, 3, 0},
    {0x05, (uint8_t []){0x3D}, 1, 0},
    {0x06, (uint8_t []){0x03}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x1F, (uint8_t []){0xDC}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x17}, 3, 0},
    {0x11, (uint8_t []){0xAA}, 1, 0},
    {0x16, (uint8_t []){0x12}, 1, 0},
    {0x0B, (uint8_t []){0xC3}, 1, 0},
    {0x10, (uint8_t []){0x0E}, 1, 0},
    {0x14, (uint8_t []){0xAA}, 1, 0},
    {0x18, (uint8_t []){0xA0}, 1, 0},
    {0x1A, (uint8_t []){0x80}, 1, 0},
    {0x1F, (uint8_t []){0x80}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x30, (uint8_t []){0xEE}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x15, (uint8_t []){0x0F}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x2D}, 3, 0},
    {0x01, (uint8_t []){0x3E}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x40}, 3, 0},
    {0x83, (uint8_t []){0xC4}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x00, (uint8_t []){0xCC}, 1, 0},
    {0x36, (uint8_t []){0xA0}, 1, 0},
    {0x2A, (uint8_t []){0x2D}, 1, 0},
    {0x2B, (uint8_t []){0x1e}, 1, 0},
    {0x2C, (uint8_t []){0x26}, 1, 0},
    {0x2D, (uint8_t []){0x2D}, 1, 0},
    {0x2E, (uint8_t []){0x1e}, 1, 0},
    {0x1F, (uint8_t []){0xE6}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0xA0}, 3, 0},
    {0x08, (uint8_t []){0xE6}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x10, (uint8_t []){0x0F}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x18}, 3, 0},
    {0x01, (uint8_t []){0x01}, 1, 0},
    {0x00, (uint8_t []){0x1E}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x43}, 3, 0},
    {0x03, (uint8_t []){0x04}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x18}, 3, 0},
    {0x3A, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x50}, 3, 0},
    {0x05, (uint8_t []){0x08}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x50}, 3, 0},
    {0x00, (uint8_t []){0xA6}, 1, 0},
    {0x01, (uint8_t []){0xA6}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x50}, 3, 0},
    {0x08, (uint8_t []){0x55}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x0B, (uint8_t []){0x43}, 1, 0},
    {0x0C, (uint8_t []){0x12}, 1, 0},
    {0x10, (uint8_t []){0x01}, 1, 0},
    {0x11, (uint8_t []){0x12}, 1, 0},
    {0x15, (uint8_t []){0x00}, 1, 0},
    {0x16, (uint8_t []){0x00}, 1, 0},
    {0x1A, (uint8_t []){0x00}, 1, 0},
    {0x1B, (uint8_t []){0x00}, 1, 0},
    {0x61, (uint8_t []){0x00}, 1, 0},
    {0x62, (uint8_t []){0x00}, 1, 0},
    {0x51, (uint8_t []){0x11}, 1, 0},
    {0x55, (uint8_t []){0x55}, 1, 0},
    {0x58, (uint8_t []){0x00}, 1, 0},
    {0x5C, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x20, (uint8_t []){0x81}, 1, 0},
    {0x21, (uint8_t []){0x82}, 1, 0},
    {0x22, (uint8_t []){0x72}, 1, 0},
    {0x30, (uint8_t []){0x00}, 1, 0},
    {0x31, (uint8_t []){0x00}, 1, 0},
    {0x32, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x44, (uint8_t []){0x44}, 1, 0},
    {0x45, (uint8_t []){0x55}, 1, 0},
    {0x46, (uint8_t []){0x66}, 1, 0},
    {0x47, (uint8_t []){0x77}, 1, 0},
    {0x49, (uint8_t []){0x00}, 1, 0},
    {0x4A, (uint8_t []){0x00}, 1, 0},
    {0x4B, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x17}, 3, 0},
    {0x37, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x15}, 3, 0},
    {0x04, (uint8_t []){0x08}, 1, 0},
    {0x05, (uint8_t []){0x04}, 1, 0},
    {0x06, (uint8_t []){0x1C}, 1, 0},
    {0x07, (uint8_t []){0x1A}, 1, 0},
    {0x08, (uint8_t []){0x18}, 1, 0},
    {0x09, (uint8_t []){0x16}, 1, 0},
    {0x24, (uint8_t []){0x05}, 1, 0},
    {0x25, (uint8_t []){0x09}, 1, 0},
    {0x26, (uint8_t []){0x17}, 1, 0},
    {0x27, (uint8_t []){0x19}, 1, 0},
    {0x28, (uint8_t []){0x1B}, 1, 0},
    {0x29, (uint8_t []){0x1D}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x16}, 3, 0},
    {0x04, (uint8_t []){0x09}, 1, 0},
    {0x05, (uint8_t []){0x05}, 1, 0},
    {0x06, (uint8_t []){0x1D}, 1, 0},
    {0x07, (uint8_t []){0x1B}, 1, 0},
    {0x08, (uint8_t []){0x19}, 1, 0},
    {0x09, (uint8_t []){0x17}, 1, 0},
    {0x24, (uint8_t []){0x04}, 1, 0},
    {0x25, (uint8_t []){0x08}, 1, 0},
    {0x26, (uint8_t []){0x16}, 1, 0},
    {0x27, (uint8_t []){0x18}, 1, 0},
    {0x28, (uint8_t []){0x1A}, 1, 0},
    {0x29, (uint8_t []){0x1C}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x18}, 3, 0},
    {0x1F, (uint8_t []){0x02}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x15, (uint8_t []){0x99}, 1, 0},
    {0x16, (uint8_t []){0x99}, 1, 0},
    {0x1C, (uint8_t []){0x88}, 1, 0},
    {0x1D, (uint8_t []){0x88}, 1, 0},
    {0x1E, (uint8_t []){0x88}, 1, 0},
    {0x13, (uint8_t []){0xf0}, 1, 0},
    {0x14, (uint8_t []){0x34}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x12, (uint8_t []){0x89}, 1, 0},
    {0x06, (uint8_t []){0x06}, 1, 0},
    {0x18, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x0A, (uint8_t []){0x00}, 1, 0},
    {0x0B, (uint8_t []){0xF0}, 1, 0},
    {0x0c, (uint8_t []){0xF0}, 1, 0},
    {0x6A, (uint8_t []){0x10}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x08, (uint8_t []){0x70}, 1, 0},
    {0x09, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x3A, (uint8_t []){0x05}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x21, (uint8_t []){0x70}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x2D}, 3, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
};

/*
static void _nop(void)
{
	_tx_command(0x00, NULL, 0);
}

static void  _sw_reset(void)
{
	delay_ms(1);
	_tx_command(0x01, NULL, 0);
	delay_ms(5);
}
*/

/* команды чтения пропускаем, ну их в зад */

/*
static void _sleep_in(void)
{
	_tx_command(0x10, NULL, 0);
}

static void _sleep_out(void)
{
	_tx_command(0x11, NULL, 0);
	delay_ms(5);
}

static void _normal_mode_on(void)
{
	_tx_command(0x13, NULL, 0);
}

static void _invert_mode_off(void)
{
	_tx_command(0x20, NULL, 0);
}

static void _invert_mode_on(void)
{
	_tx_command(0x21, NULL, 0);
}

static void _display_off(void)
{
	_tx_command(0x28, NULL, 0);
}
*/
static void _display_on(void)
{
	_tx_command(0x29, NULL, 0);
}
/*
static void _set_column(uint16_t start, uint16_t end)
{
	uint8_t data[4] = {start >> 8, start & 0xFC, end >> 8, (end & 0xFF) | 0x03};
	_tx_command(0x2A, data, 4);
}

static void _set_row(uint16_t start, uint16_t end)
{
	uint8_t data[4] = {start >> 8, start & 0xFF, end >> 8, end & 0xFF};
	_tx_command(0x2B, data, 4);
}

static void _write_memory_start(void)
{
	_tx_command(0x2C, NULL, 0);
}

static void _tearing_off(void)
{
	_tx_command(0x34, NULL, 0);
}

static void _tearing_on(void)
{
	_tx_command(0x35, NULL, 0);
}
*/
/*
static void _data_access_control(bool BGR_color_order, bool horisontal_flip, bool vertical_flip)
{
	uint8_t param = (BGR_color_order ? 8 : 0) | (horisontal_flip ? 2 : 0) | (vertical_flip ? 1 : 0);
	_tx_command(0x36, &param, 1);
}
*/
/*
static void _idle_mode_off(void)
{
	_tx_command(0x38, NULL, 0);
}

static void _idle_mode_on(void)
{
	_tx_command(0x39, NULL, 0);
}
*/
/*
static void _set_color_format(uint8_t format)
{
	if ((format != COLOR_FORMAT_16700K) && (format != COLOR_FORMAT_262K))
		format = COLOR_FORMAT_65K;
	_tx_command(0x3A, &format, 1);
}
*/
/*
static void _write_memory_continue(void)
{
	_tx_command(0x3C, NULL, 0);
}


static void _set_tear_scanline(uint16_t line)
{
	line = (line >> 8) | (line << 8);
	_tx_command(0x44, &line, 2);
}

static void _write_display_brightness(uint16_t bright)
{
	bright = ((bright >> 5) & 0xFF) | ((bright << 8) & 0x3F00);
	_tx_command(0x51, &bright, 2);
}

static void _set_display_brightness_mode(bool BCTRL, bool DD, bool BL)
{
	uint8_t data = (BCTRL ? 0x20 : 0) | (DD ? 0x08 : 0) | (BL ? 0x04 : 0);
	_tx_command(0x53, &data, 1);
}

static void _set_CABC_control(uint8_t power_saving_level)
{
	power_saving_level &= 3;
	_tx_command(0x55, &power_saving_level, 1);
}

static void _set_CABC_minimum_brightness(void)
{
    _tx_command(0x5E, NULL, 0);
}
*/
static void panel_spd2010_init(void)
{
	unsigned int i;
	delay_ms(50);



	_tx_command(SPD2010_CMD_SET, (uint8_t[]) {SPD2010_CMD_SET_BYTE0, SPD2010_CMD_SET_BYTE1, SPD2010_CMD_SET_USER}, 3);
	_tx_command(LCD_CMD_MADCTL, (uint8_t[]){0}, 9);
	_tx_command(LCD_CMD_COLMOD, (uint8_t[]){0x05}, 1);

	for (i = 0; i < (sizeof(vendor_specific_init_default) / sizeof(vendor_specific_init_default[0])); i++)
	{
			_tx_command(vendor_specific_init_default[i].cmd, vendor_specific_init_default[i].data, vendor_specific_init_default[i].data_bytes);
			if (vendor_specific_init_default[i].delay_ms)
				delay_ms(vendor_specific_init_default[i].delay_ms);
	}

    _display_on();
    delay_ms(75);
    DRIVER_SPD2010_FullDisplaySet();
    DRIVER_SPD2010_WriteSingleColor(0, DISPLAY_WIDTH * DISPLAY_HEIGHT, false);
}

static void _pin_out_config(gpio_pin_e pin, uint8_t default_state)
{
  gpio_output_en(pin);
  gpio_input_dis(pin);
  gpio_set_level(pin, default_state == 0 ? 0 : 1);
  gpio_function_en(pin);
}

void DRIVER_SPD2010_Init(BSP_DRIVER_SPI_ID SPI_ID)
{
	(void)SPI_ID;

    /*
     * Register the SPI completion callback before any display transaction.
     * Without this registration the DMA transfer can complete normally,
     * but _spi_callback() is never invoked and LVGL remains in the
     * "flushing" state forever after the first asynchronous frame.
     */
    BSP_DRIVER_SPI_SetCallback(BDSID_LSPI, _spi_callback);

	GPIO_Config(DISPLAY_CS, GPIO_OUTPUT, GPIO_PIN_OUT_HIGH);
	GPIO_Config(DISPLAY_RESET, GPIO_OUTPUT, GPIO_PIN_OUT_LOW);
    _display_reset();
	panel_spd2010_init();
}

int DRIVER_SPD2010_GetWidth(void)
{
  return DISPLAY_WIDTH;
}

int DRIVER_SPD2010_GetHeight(void)
{
  return DISPLAY_HEIGHT;
}

static inline void _place_coord(uint8_t *arr, uint16_t start, uint16_t size)
{
	arr[0] = start >> 8;
	arr[1] = start & 0xFF;
	arr[2] = (start + size - 1) >> 8;
	arr[3] = (start + size - 1) & 0xFF;
}

void DRIVER_SPD2010_SetWindow(uint16_t xleft, uint16_t ytop, uint16_t width, uint16_t height)
{
	uint8_t coord[4];
	if (xleft >= DISPLAY_WIDTH)
	{
		xleft = DISPLAY_WIDTH - 1;
		width = 1;
	}
	if (ytop >= DISPLAY_HEIGHT)
	{
		ytop = DISPLAY_HEIGHT - 1;
		height = 1;
	}
	if (width == 0)
		width = 1;
	if (height == 0)
		height = 1;
	if ((xleft + width) > DISPLAY_WIDTH)
		width = DISPLAY_WIDTH - xleft;
	if ((ytop + height) > DISPLAY_HEIGHT)
		height = DISPLAY_HEIGHT - ytop;
	_place_coord(coord, xleft, width);
	_tx_command(LCD_CMD_CASET, coord, 4);
	_place_coord(coord, ytop, height);
	_tx_command(LCD_CMD_RASET, coord, 4);
}

void DRIVER_SPD2010_Write(TDisplayColor *data, uint32_t count, bool cont)
{
/*
  int cnt = (count > DISPLAY_WIDTH) ? DISPLAY_WIDTH : count, i = cnt;
  _tx_colordata(true, data, cnt);
  count -= cnt;
  while (count)
  {
	  cnt = (count > DISPLAY_WIDTH) ? DISPLAY_WIDTH : count;
	  _tx_colordata(false, &data[i], cnt);
	  i += cnt;
	  count -= cnt;
  }
*/
  _tx_color(!cont, data, count * sizeof(TDisplayColor), false);
}

static TDisplayColor dspl_str[DISPLAY_WIDTH];

void DRIVER_SPD2010_WriteSingleColor(TDisplayColor color, uint32_t count, bool cont)
{
  uint32_t i;
  bool start = !cont;
  for (i = 0; (i < count) && (i < DISPLAY_WIDTH); i++)
  {
	  dspl_str[i] = color;
  }
  while (count)
  {
	  int cnt = (count > DISPLAY_WIDTH) ? DISPLAY_WIDTH : count;
	  _tx_color(start, dspl_str, cnt * sizeof(TDisplayColor), true);
	  start = false;
	  count -= cnt;
  }
}

void DRIVER_SPD2010_FullDisplaySet(void)
{
	DRIVER_SPD2010_SetWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void DRIVER_SPD2010_Mirror(bool mirror_x, bool mirror_y)
{
	uint8_t madctl = 0;
    if (mirror_x)
    	madctl |= 2;
    if (mirror_y)
    	madctl |= 1;
    _tx_command(LCD_CMD_MADCTL, &madctl, 1);
}

void DRIVER_SPD2010_Sleep(void)
{
	gpio_set_low_level(DISPLAY_RESET);
	gpio_set_low_level(DISPLAY_CS);
	BSP_DRIVER_SPI_Sleep(BDSID_LSPI);
}

void DRIVER_SPD2010_Wakeup(void)
{
	BSP_DRIVER_SPI_Wakeup(BDSID_LSPI);
	DRIVER_SPD2010_Init(BDSID_LSPI);
}
