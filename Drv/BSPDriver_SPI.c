#include "BSPDriver_SPI.h"
#include <stdbool.h>
#include "gpio.h"
#include "../def.h"

//static unsigned int spi_slave_address = 0xc0200400;

lspi_pin_config_t lspi_pin_config = {
	//.spi_csn_pin      = LSPI_CSN_PE0_PIN,
    .spi_csn_pin      = (lspi_pin_def_e)GPIO_NONE_PIN,
    .spi_clk_pin      = LSPI_CLK_PE1_PIN,
    .spi_mosi_io0_pin = LSPI_MOSI_IO0_PE2_PIN,
    .spi_miso_io1_pin = LSPI_MISO_IO1_PE3_PIN, //3line mode is required, otherwise it is NONE_PIN.
    .spi_io2_pin      = LSPI_IO2_PE4_PIN,      //quad  mode is required, otherwise it is NONE_PIN.
    .spi_io3_pin      = LSPI_IO3_PE5_PIN,      //quad  mode is required, otherwise it is NONE_PIN.
};


gspi_pin_config_t gspi_pin_config =
{
    .spi_clk_pin = GPIO_PB2,
    .spi_csn_pin = GPIO_NONE_PIN,
    .spi_mosi_io0_pin = GPIO_PB6,
    .spi_miso_io1_pin = GPIO_PB5,
    .spi_io2_pin = GPIO_PB4,
    .spi_io3_pin = GPIO_PB3
};

/*
gspi_pin_config_t gspi_pin_config =
{
    .spi_clk_pin = GPIO_PA0,
    .spi_csn_pin = GPIO_NONE_PIN,
    .spi_mosi_io0_pin = GPIO_PF4,
    .spi_miso_io1_pin = GPIO_PF5,
    .spi_io2_pin = GPIO_NONE_PIN,
    .spi_io3_pin = GPIO_NONE_PIN
};
*/
typedef struct
{
	spi_sel_e module;
	spi_mode_type_e mode;
	void *pin_config;
	spi_wr_rd_config_t *config;
	dma_chn_e transmitt_dma_channel;
	dma_chn_e receive_dma_channel;
	uint32_t speed;
	BSP_DRIVER_SPI_CB cb;
	bool transfer;
	bool transfer_ok;
	bool receive;
	bool receive_ok;
	bool busy;
	bool giving;
}BSP_DRIVER_SPI_Def;

static spi_wr_rd_config_t lspi_config = {
    .spi_io_mode     = SPI_QUAD_MODE, /*IO mode set to SPI_3_LINE_MODE when SPI_3LINE_SLAVE.*/
    .spi_dummy_cnt   = 0,              //B92 supports up to 32 clk cycle dummy, and TL751X,TL7518,TL721X,TL321X,tl322x supports up to 256 clk cycle dummy.
    .spi_cmd_en      = 0,
    .spi_addr_en     = 0,
    .spi_addr_len    = 0, //when spi_addr_en = 0,invalid set.
    .spi_cmd_fmt_en  = 0, //when spi_cmd_en = 0,invalid set.
    .spi_addr_fmt_en = 0, //when spi_addr_en = 0,invalid set.
};

static spi_wr_rd_config_t gspi_config = {
    .spi_io_mode     = SPI_3_LINE_MODE, /*IO mode set to SPI_3_LINE_MODE when SPI_3LINE_SLAVE.*/
    .spi_dummy_cnt   = 0,              //B92 supports up to 32 clk cycle dummy, and TL751X,TL7518,TL721X,TL321X,tl322x supports up to 256 clk cycle dummy.
    .spi_cmd_en      = 0,
    .spi_addr_en     = 0,
    .spi_addr_len    = 0, //when spi_addr_en = 0,invalid set.
    .spi_cmd_fmt_en  = 0, //when spi_cmd_en = 0,invalid set.
    .spi_addr_fmt_en = 0, //when spi_addr_en = 0,invalid set.
};

static BSP_DRIVER_SPI_Def spi[BDSID_COUNT] =
{
		{
				LSPI_MODULE,
				SPI_MODE3,
				&lspi_pin_config,
				&lspi_config,
				DMA0,
				DMA1,
				12000000,
				NULL,
				false,
				false,
				false,
				false,
				false,
				false
		},/*
		{
				GSPI_MODULE,
				SPI_MODE3,
				&gspi_pin_config,
				&gspi_config,
				DMA2,
				DMA3,
				1000000,
				NULL,
				false,
				false,
				false,
				false,
				false,
				false
		}*/
};

_attribute_ram_code_sec_noinline_ void bsp_driver_spi_lspi_irq_handler(void)
{
	int i;
	for (i = 0; i < BDSID_COUNT; i++)
	{
		if (spi_get_irq_status(spi[i].module, SPI_END_INT))
		{
		    spi_clr_irq_status(spi[i].module, SPI_END_INT); //clr
		    if (spi[i].transfer)
		    {
		    	spi[i].transfer_ok = true;
		    }
		}
	}
}

PLIC_ISR_REGISTER(bsp_driver_spi_lspi_irq_handler, IRQ_LSPI)

_attribute_ram_code_sec_noinline_ void bsp_driver_spi_dma_irq_handler(void)
{
	int i;
	for (i = 0; i < BDSID_COUNT; i++)
	{
		if (dma_get_tc_irq_status(BIT(spi[i].receive_dma_channel)))
		{
			dma_clr_tc_irq_status(BIT(spi[i].receive_dma_channel));
			if (spi[i].receive)
			{
				spi[i].receive_ok = true;
			}
		}
	}
}

static void _spi_clear(BSP_DRIVER_SPI_ID ID)
{
	spi[ID].cb = NULL;
	spi[ID].transfer = false;
	spi[ID].transfer_ok = false;
	spi[ID].receive = false;
	spi[ID].receive_ok = false;
	spi[ID].busy = false;
	spi[ID].giving = false;
}

void BSP_DRIVER_SPI_Init(void)
{
  int i;
  for (i = 0; i < BDSID_COUNT; i++)
  {
	switch (spi[i].module)
	{
      case BDSID_LSPI:
      //case BDSID_GSPI:
      {
    	  break;
      }
	  default:
	  {
		  continue;
	  }
	}
  	spi_master_init(spi[i].module, sys_clk.pll_clk * 1000000 / spi[i].speed, spi[i].mode);
    //spi_clr_irq_status(spi[i].module, SPI_END_INT);  //clr
    //spi_set_irq_mask(spi[i].module, SPI_END_INT_EN); //endint_en
    //spi_set_tx_dma_config(spi[i].module, spi[i].transmitt_dma_channel);
    //spi_set_master_rx_dma_config(spi[i].module, spi[i].receive_dma_channel);
    switch ((int)spi[i].module)
    {
      case LSPI_MODULE:
      {
   	      lspi_set_pin(&lspi_pin_config);
   	      spi_master_config(spi[i].module, spi[i].config->spi_io_mode == SPI_3_LINE_MODE ? SPI_3LINE : SPI_NORMAL);
   		  BSP_DRIVER_SPI_SetMode(i, spi[i].config->spi_io_mode);
   	      //spi_master_config(spi[i].module, SPI_NORMAL);
    	  //plic_interrupt_enable(IRQ_LSPI);
    	  break;
      }
     /* case GSPI_MODULE:
      {
    	  GPIO_Config(gspi_pin_config.spi_clk_pin, GPIO_OUTPUT, GPIO_PIN_OUT_HIGH);
    	  GPIO_Config(gspi_pin_config.spi_miso_io1_pin, GPIO_INPUT, GPIO_NO_PULL);
    	  GPIO_Config(gspi_pin_config.spi_mosi_io0_pin, GPIO_OUTPUT, GPIO_PIN_OUT_HIGH);
   	      //gspi_set_pin(&gspi_pin_config);
   	      //spi_master_config(spi[i].module, SPI_3LINE);
    	  //plic_interrupt_enable(IRQ_LSPI);
    	  break;
      }*/
    }
    //spi_master_config_plus(spi[i].module, spi[i].config);
    //dma_set_irq_mask(spi[i].receive_dma_channel, TC_MASK);
    _spi_clear(i);
  }
  //plic_interrupt_enable(IRQ_DMA);
}

void BSP_DRIVER_SPI_SetMode(BSP_DRIVER_SPI_ID ID, spi_io_mode_e mode)
{
	spi_set_io_mode(spi[ID].module, mode);
}

int BSP_DRIVER_SPI_Take(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_CB cb)
{
  if (ID < BDSID_COUNT)
  {
	do
	{
      BSP_DRIVER_SPI_Poll();
	}
	while (spi[ID].giving && spi[ID].busy);
	if (!spi[ID].busy)
	{
	  spi[ID].busy = true;
	  spi[ID].cb = cb;
	  return BDSM_OK;
	}
  }

  return BDSM_ERROR;
}

void BSP_DRIVER_SPI_Give(BSP_DRIVER_SPI_ID ID)
{
  if (ID < BDSID_COUNT)
  {
	if (spi[ID].busy)
	{
      spi[ID].giving = true;
	  BSP_DRIVER_SPI_Poll();
	}
  }
}

static bool LSPI_sleep = false;
static bool GSPI_sleep = false;

static void _disable_pin(gpio_pin_e pin)
{
    gpio_set_up_down_res(pin, GPIO_PIN_UP_DOWN_FLOAT);

    // Вернуть pin из LSPI в GPIO
    gpio_set_mux_function(pin, AS_GPIO);

    // Включить управление pin со стороны GPIO
    gpio_function_en(pin);

    // Задать 0 ДО включения выхода
    gpio_set_low_level(pin);

    // GPIO output enable
    gpio_output_en(pin);

    // Input disable
    gpio_input_dis(pin);
}

void BSP_DRIVER_SPI_Sleep(BSP_DRIVER_SPI_ID ID)
{
	switch (ID)
	{
		case BDSID_LSPI:
		{
			spi_hw_fsm_reset(spi[ID].module);
			reg_clk_en0 &= ~FLD_CLK0_LSPI_EN;
			/*
			_disable_pin(((lspi_pin_config_t *)(spi[ID].pin_config))->spi_clk_pin);
			_disable_pin(((lspi_pin_config_t *)(spi[ID].pin_config))->spi_csn_pin);
			_disable_pin(((lspi_pin_config_t *)(spi[ID].pin_config))->spi_io2_pin);
			_disable_pin(((lspi_pin_config_t *)(spi[ID].pin_config))->spi_io3_pin);
			_disable_pin(((lspi_pin_config_t *)(spi[ID].pin_config))->spi_miso_io1_pin);
			_disable_pin(((lspi_pin_config_t *)(spi[ID].pin_config))->spi_mosi_io0_pin);
			*/
			_disable_pin(lspi_pin_config.spi_clk_pin);
			_disable_pin(lspi_pin_config.spi_csn_pin);
			_disable_pin(lspi_pin_config.spi_io2_pin);
			_disable_pin(lspi_pin_config.spi_io3_pin);
			_disable_pin(lspi_pin_config.spi_miso_io1_pin);
			_disable_pin(lspi_pin_config.spi_mosi_io0_pin);
			LSPI_sleep = true;
			break;
		}
		default:
		{
			break;
		}
	}
}

void BSP_DRIVER_SPI_Wakeup(BSP_DRIVER_SPI_ID ID)
{
	switch (ID)
	{
		case BDSID_LSPI:
		{
			BSP_DRIVER_SPI_Init();
			LSPI_sleep = false;
			break;
		}
		default:
		{
			break;
		}
	}
}

bool BSP_DRIVER_SPI_IsSleep(BSP_DRIVER_SPI_ID ID)
{
	switch (ID)
	{
		case BDSID_LSPI:
		{
			return LSPI_sleep;
		}
		default:
		{
			break;
		}
	}
	return false;
}
/*
uint8_t _gspi_byte_read(void)
{
	uint8_t i, data = 0;

	for (i = 0; i < 16; i++)
	{
		bool level = gpio_get_level(gspi_pin_config.spi_miso_io1_pin);
		if (i & 1)
		{
			gpio_set_level(gspi_pin_config.spi_mosi_io0_pin, level ? 1 : 0);
			data <<= 1;
			if (level)
				data |= 1;
			gpio_set_high_level(gspi_pin_config.spi_clk_pin);
		}
		else
		{
			gpio_set_low_level(gspi_pin_config.spi_clk_pin);
		}
	}
	return data;
}

uint8_t _gspi_byte_writeread(uint8_t out_byte)
{
	uint8_t i, data = 0;

	for (i = 0; i < 16; i++)
	{
		bool level = gpio_get_level(gspi_pin_config.spi_miso_io1_pin);
		if (i & 1)
		{
			gpio_set_level(gspi_pin_config.spi_mosi_io0_pin, (out_byte & 0x80) != 0 ? 1 : 0);
			data <<= 1;
			if (level)
				data |= 1;
			gpio_set_high_level(gspi_pin_config.spi_clk_pin);
			out_byte <<= 1;
		}
		else
		{
			gpio_set_low_level(gspi_pin_config.spi_clk_pin);
		}
	}
	return data;
}

void _gspi_read(void *buffer, int count)
{
	uint8_t *buf = buffer;
	while (count)
	{
		*buf++ = _gspi_byte_writeread(0xFF);
		count--;
	}
}

void _gspi_write(void *buffer, int count)
{
	uint8_t *buf = buffer;
	while (count)
	{
		_gspi_byte_writeread(*buf++);
		count--;
	}
}

void _gspi_writeread(void *buffer, int count)
{
	uint8_t *buf = buffer;
	while (count)
	{
		*buf = _gspi_byte_writeread(*buf);
		buf++;
		count--;
	}
}
*/
int BSP_DRIVER_SPI_Read(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
	if ((ID < BDSID_COUNT) && (buffer != NULL))
	{
		if (count)
		{
			//_spi_read(spi[ID].module, buffer, count);
			//if (spi[ID].module == BDSID_GSPI)
			//	_gspi_read(buffer, count);
			//else
				spi_master_read(spi[ID].module, buffer, count);
		}
		return BDSM_OK;
	}
	return BDSM_ERROR;
}

int BSP_DRIVER_SPI_Write(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
	if ((ID < BDSID_COUNT) && (buffer != NULL))
	{
		if (count)
		{
			//while (spi[ID].transfer)
				//BSP_DRIVER_SPI_Poll();
			//spi[ID].transfer = true;
			//if (spi[ID].module == BDSID_GSPI)
			//	_gspi_write(buffer, count);
			//else
			spi_master_write(spi[ID].module, (unsigned char *)buffer, count);
			//spi_master_write_dma(spi[ID].module, (unsigned char *)buffer, count);
			/*spi_master_write_dma_plus(spi[ID].module,
					                  SPI_WRITE_DATA_QUAD_CMD,
									  spi_slave_address,
									  (unsigned char *)buffer,
									  count,
									  SPI_MODE_WR_DUMMY_WRITE);*/
		}
		return BDSM_OK;
	}
	return BDSM_ERROR;
}

void BSP_DRIVER_SPI_Poll(void)
{
  int i;
  for (i = 0; i < BDSID_COUNT; i++)
  {
	  if (spi[i].busy)
	  {
        if (spi[i].transfer_ok)
        {
        	if (spi[i].transfer)
        	{
        		spi[i].transfer = false;
        		if (spi[i].cb)
        			spi[i].cb(i, BDSM_WRITEN);
        	}
        	spi[i].transfer_ok = false;
        }
        if (spi[i].receive_ok)
        {
        	if (spi[i].receive)
        	{
        		spi[i].receive = false;
        		if (spi[i].cb)
        			spi[i].cb(i, BDSM_READEN);
        	}
        	spi[i].receive_ok = false;
        }
        if (!(spi[i].receive || spi[i].transfer))
        	if (spi[i].giving)
        		spi[i].busy = false;
	  }
	  if (!spi[i].busy)
		  _spi_clear(i);
  }
}

/*

gspi_pin_config_t gspi_pin_config =
{
    .spi_clk_pin = GPIO_PA2,
    .spi_csn_pin = GPIO_NONE_PIN,
    .spi_mosi_io0_pin = GPIO_PA3,
    .spi_miso_io1_pin = GPIO_PA4,
    .spi_io2_pin = GPIO_NONE_PIN,
    .spi_io3_pin = GPIO_NONE_PIN
};

...

spi_master_init(GSPI_MODULE, sys_clk.pll_clk * 1000000 / 1000000, SPI_MODE3);
gspi_set_pin(&gspi_pin_config);
spi_master_config(GSPI_MODULE, SPI_3LINE);
spi_set_io_mode(GSPI_MODULE, SPI_3_LINE_MODE);

 */
