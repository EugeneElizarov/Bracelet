/********************************************************************************************************
 * @file    app_att.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#include <time.h>

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "app_att.h"
#include "BBStream.h"
#include "app_config.h"

#include "UUIDdef.h"

////////////////////////////////////////// peripheral-role ATT service concerned ///////////////////////////////////////////////
typedef struct
{
    /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
    u16 intervalMin;
    /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
    u16 intervalMax;
    /** Number of LL latency connection events (0x0000 - 0x03e8) */
    u16 latency;
    /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
    u16 timeout;
} gap_periConnectParams_t;

typedef struct __attribute__((packed))
{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
    u8  value[1];
} ble_rf_packet_att_write_t;

static const u16 clientCharacterCfgUUID = GATT_UUID_CLIENT_CHAR_CFG;

static const u16 extReportRefUUID = GATT_UUID_EXT_REPORT_REF;

static const u16 reportRefUUID = GATT_UUID_REPORT_REF;

static const u16 userdesc_UUID = GATT_UUID_CHAR_USER_DESC;

static const u16 serviceChangeUUID = GATT_UUID_SERVICE_CHANGE;

static const u16 my_primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;

static const u16 my_characterUUID = GATT_UUID_CHARACTER;

static const u16 my_devServiceUUID = SERVICE_UUID_DEVICE_INFORMATION;

static const u16 my_modelNumberUUID = CHARACTERISTIC_UUID_MODEL_NUMBER_STRING;

static const u16 my_firmwareRevisionUUID = CHARACTERISTIC_UUID_FIRMWARE_REVISION_STRING;

static const u16 my_softwareRevisionUUID = CHARACTERISTIC_UUID_SOFTWARE_REVISION_STRING;

static const u16 my_manufacturerNameUUID = CHARACTERISTIC_UUID_MANUFACTURER_NAME_STRING;

static const u16 my_PnPUUID = CHARACTERISTIC_UUID_PNP_ID;

static const u16 my_devNameUUID = GATT_UUID_DEVICE_NAME;

static const u16 my_gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;

static const u16 my_appearanceUUID = GATT_UUID_APPEARANCE;

static const u16 my_periConnParamUUID = GATT_UUID_PERI_CONN_PARAM;

static const u16 my_appearance = GAP_APPEARANCE_UNKNOWN;

static const u16 my_gattServiceUUID = SERVICE_UUID_GENERIC_ATTRIBUTE;

static const gap_periConnectParams_t my_periConnParameters = {20, 40, 0, 1000};

_attribute_ble_data_retention_ static u16 serviceChangeVal[2] = {0};

_attribute_ble_data_retention_ static u8 serviceChangeCCC[2] = {0, 0};

#if SERVICE_TSKBM != 0
static const u8 my_devName[]       = {'T', 'S', 'K', 'B', 'M', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '5'};
#else
static const u8 my_devName[]       = {'N', 'e', 'u', 'r', 'o', ' ', 'W', 'r', 'i', 's', 't', 'b', 'a', 'n', 'd', ' ', '3',};
#endif

static const u8 modelNumber[]      = {'N', 'e', 'u', 'r', 'o', ' ', 'W', 'r', 'i', 's', 't', 'b', 'a', 'n', 'd', ' ', 'm', 'o', 'd', 'e', 'l', ' ', 'v', '3'};
static const u8 firmwareRevision[] = {'v', '0', '.', '1'};
static const u8 softwareRevision[] = {'v', '0', '.', '1'};
static const u8 manufacturerName[] = {'N', 'e', 'u', 'r', 'o', 'c', 'o', 'm', ' ', 'J', 'S', 'C'};


#define NEUROCOM_DATA_STREAM_CHAR			0xFFF3
#define NEUROCOM_CONFIG_CHAR				0xFFF4
#define NEUROCOM_BB_FLASH_DATA_CHAR			0xFFF5
#define NEUROCOM_BB_FLASH_CMD_CHAR			0xFFF6
#define NEUROCOM_INFO_CHAR					0xFFF7
#define NEUROCOM_INFO_FLASH_CHAR			0xFFF8
#define NEUROCOM_CMD_CHAR					0xFFF9
#define NEUROCOM_DIAGNOSTIC_CHAR			0xFFFA

static const u8 neurocomMainService[] 		= USER_UUID(NEUROCOM_MAIN_SERVICE);
static const u8 neurocomWristbandService[]	= USER_UUID(NEUROCOM_MAIN_SERVICE);
static const u8 neurocomDataStreamChar[]	= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_DATA_STREAM_CHAR);
static const u8 neurocomConfigChar[]		= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_CONFIG_CHAR);
static const u8 neurocomBBFlashDataChar[]	= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_BB_FLASH_DATA_CHAR);
static const u8 neurocomBBFlashCmdChar[]	= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_BB_FLASH_CMD_CHAR);
static const u8 neurocomInfoChar[]			= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_INFO_CHAR);
static const u8 neurocomInfoFlashChar[]		= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_INFO_FLASH_CHAR);
static const u8 neurocomCmdChar[]			= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_CMD_CHAR);
static const u8 neurocomDiagChar[]			= USER_CHRC(NEUROCOM_MAIN_SERVICE, NEUROCOM_DIAGNOSTIC_CHAR);

void neurocomDbg(u32 Char, ble_rf_packet_att_write_t *p, u8 len) // len == 0 -> write operation
{
	char str[30];
	if (Char >= 0x10000)
		snprintf(str, sizeof(str), "TSKBM [%04X] %c%c ", Char & 0xFFFF, (len == 0) ? '<' : '=', (len == 0) ? '=' : '>');
	else
		snprintf(str, sizeof(str), "SPRV [%04X] %c%c ", Char & 0xFFFF, (len == 0) ? '<' : '=', (len == 0) ? '=' : '>');
	if (len == 0)
		tlkapi_send_str_data(str, p->value, p->l2capLen - 3);
	else
		tlkapi_send_str_data(str, p, len);
}

#if SERVICE_TSKBM != 0
static const u16 TSKBMMainService			= 0xFFF0;
static const u16 TSKBMInfoReq				= 0xFFF1;
static const u16 TSKBMSecurityReq			= 0xFFF2;
static const u16 TSKBMSecurityResp			= 0xFFF3;
static const u16 TSKBMDataEDR    			= 0xFFF4;
static const u16 TSKBMInformationResp		= 0xFFF5;

static const u8 TSKBMInfoReqVal[5] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE,
    U16_LO(TSKBM_Data_InfoReq_DP_H),
    U16_HI(TSKBM_Data_InfoReq_DP_H),
    U16_LO(TSKBMInfoReq),
    U16_HI(TSKBMInfoReq)};

static const u8 TSKBMSecurityReqVal[5] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE,
    U16_LO(TSKBM_Data_SecurityReq_DP_H),
    U16_HI(TSKBM_Data_SecurityReq_DP_H),
    U16_LO(TSKBMSecurityReq),
    U16_HI(TSKBMSecurityReq)};

static const u8 TSKBMSecurityRespVal[5] = {
	CHAR_PROP_READ,
    U16_LO(TSKBM_Data_SecurityResp_DP_H),
    U16_HI(TSKBM_Data_SecurityResp_DP_H),
    U16_LO(TSKBMSecurityResp),
    U16_HI(TSKBMSecurityResp)};

static const u8 TSKBMDataEDRVal[5] = {
	CHAR_PROP_READ | CHAR_PROP_NOTIFY,
    U16_LO(TSKBM_Data_DataEDR_DP_H),
    U16_HI(TSKBM_Data_DataEDR_DP_H),
    U16_LO(TSKBMDataEDR),
    U16_HI(TSKBMDataEDR)};

static const u8 TSKBMInformationRespVal[5] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE,
    U16_LO(TSKBM_DataInformationResp_DP_H),
    U16_HI(TSKBM_DataInformationResp_DP_H),
    U16_LO(TSKBMInformationResp),
    U16_HI(TSKBMInformationResp)};

static u8 TSKBMInformationReadCmd[2]  = {0, 0};
static u8 TSKBMRandomValue[3]		  = {0, 0, 0};
static u8 TSKBMEncryptedValue[3]	  = {0, 0, 0};
static u8 TSKBMDataEDRValue[11]		  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static u8 TSKBMDataEDRCCCValue[2]     = {0, 0};
static u8 TSKBMInformationWriteCmd[6] = {0, 0, 0, 0, 0, 0};
static u8 TSKBMDataBuffer[6] = {0, 0, 0, 0, 0, 0};
static u8 TSKBMCycleNumber = 0;
static u8 TSKBM_ESK[6] = {0, 0, 0, 0, 0, 0};

static u16 ble_cadr_counter = 0;
static struct
{
  u8 measure_number;
  u8 esk_code;
  u8 esk_code_prev;
  float logarithm;
  u32 logarithm_prev;
  u32 logarithm_convert_prev;
  u32 data;
  u32 data_last_valid;
  float data_convert_kom;
  u32 logarithm_convert;
  u32 data_convert_last_valid;
  u32 data_convert_kom_prev;
  u32 integrator;
}eda_data;

static u8 dataedr_testdata[4] = {0x02, 0xB6, 0xBE, 0xFE};
static u8 counter_measure = 0;
static u8 fast_mode_counter = 0;
static u32 raw_eda[2];
static u8 DataEdrPackCounter = 0;
static u8 DataEdrPack[11];

void TSKBMSetData(bool onHand, int32_t GSR_R, u32 ad7791_read_data)
{

    ble_cadr_counter++;
    eda_data.measure_number = ble_cadr_counter;
    eda_data.esk_code_prev = eda_data.esk_code;
    eda_data.logarithm_prev = eda_data.logarithm;
    eda_data.logarithm_convert_prev = eda_data.logarithm_convert;
    switch (ble_cadr_counter)
    {
      case 256: // последний кадр данных, запускаем проверку на переменном токе
      	eda_data.data_last_valid = eda_data.data;
      	eda_data.data_convert_last_valid = eda_data.data_convert_kom_prev;
        break;
      case 257:
        eda_data.esk_code = dataedr_testdata[0];
        eda_data.data = eda_data.data_last_valid;
        eda_data.data_convert_kom = eda_data.data_convert_last_valid;	// передаем последнее актуальное значение
        break;
      case 258:
        eda_data.esk_code = dataedr_testdata[1];
        eda_data.data = eda_data.data_last_valid;
        eda_data.data_convert_kom = eda_data.data_convert_last_valid; // передаем последнее актуальное значение
        break;
      case 259:
        eda_data.esk_code = dataedr_testdata[2];
        eda_data.data = eda_data.data_last_valid;
        eda_data.data_convert_kom = eda_data.data_convert_last_valid; // передаем последнее актуальное значение
        break;
      case 260:
        eda_data.esk_code = dataedr_testdata[3];
        eda_data.data = eda_data.data_last_valid;
        eda_data.data_convert_kom = eda_data.data_convert_last_valid; // передаем последнее актуальное значение
        ble_cadr_counter = 0;
        break;
      default:
        raw_eda[counter_measure++] = ad7791_read_data & 0xFFFFFFF0;
        if (counter_measure > 1)
        	counter_measure = 0;
        else
        	return;

        if (GSR_R <= 0)
        {
        	eda_data.data = (raw_eda[0] + raw_eda[1]) / 2; // данные от АЦП для вычисления приращений
        	eda_data.data_convert_kom = (100000 * (float)eda_data.data / 0x00800000) ;
        	eda_data.logarithm = roundf(250*logf(eda_data.data_convert_kom/248.0)); // логарифмирование для вычисления приращений (data_convert_kom изменилось на 0,2% = logarithm изменился на 1)
        }
        else
        	eda_data.logarithm = GSR_R;
        eda_data.logarithm_convert = (u32)(8*eda_data.logarithm + 16384);

        if(fast_mode_counter <= 4)
        {
          if ((eda_data.logarithm - eda_data.integrator) > 16) // резкое изменение сопротивления
          {
            eda_data.esk_code = 32;
            fast_mode_counter++;
            eda_data.integrator += 16;
          }
          else if((eda_data.integrator - eda_data.logarithm) > 16)
          {
            eda_data.esk_code = 0;
            fast_mode_counter++;
            eda_data.integrator -= 16;
          }
          else
          {
            eda_data.esk_code = (eda_data.logarithm - eda_data.integrator + 16);
            eda_data.integrator = eda_data.logarithm;
            fast_mode_counter = 0;
          }
        }
        else  // режим догонки
        {
          if ((eda_data.logarithm - eda_data.integrator) > 160)
          {
            eda_data.esk_code = 32; // 32 << 2
            fast_mode_counter++;
            eda_data.integrator += 160;
          }
          else if (((eda_data.logarithm - eda_data.integrator) > 16) && ((eda_data.logarithm - eda_data.integrator) <= 160))
          {
            eda_data.esk_code = 32; // 32 << 2
            fast_mode_counter = 0;
            eda_data.integrator = eda_data.logarithm;
          }
          else if((eda_data.integrator - eda_data.logarithm) > 160)
          {
            eda_data.esk_code = 0;
            fast_mode_counter++;
            eda_data.integrator -= 160;
          }
          else if(((eda_data.integrator - eda_data.logarithm) > 16) && ((eda_data.integrator - eda_data.logarithm) <= 160))
          {
            eda_data.esk_code = 0;
            fast_mode_counter = 0;
            eda_data.integrator = eda_data.logarithm;
          }
          else
          {
            eda_data.esk_code = eda_data.logarithm - eda_data.integrator + 16;
            eda_data.integrator = eda_data.logarithm;
            fast_mode_counter = 0;
          }
        }

#if 0
	// Дополнительное искажение, чтобы проходить проверку на минимальное изменение сигнала за период 65 секунд
        if ((ble_cadr_counter < 9) && (device_status & SYSTEM_CHANGE_POLARITY_FLAG_MASK))
        {
        	eda_data.esk_code = (!(device_status & DEVICE_ON_HAND_ERROR_MASK)) ? 128 : 129;
        	eda_data.data = eda_data.data_last_valid;
        	eda_data.data_convert_kom = eda_data.data_convert_last_valid; // передаем последнее актуальное значение
        	break;
        }
        else if ((ble_cadr_counter < 18) && (device_status & SYSTEM_CHANGE_POLARITY_FLAG_MASK))
        {
        	eda_data.esk_code = (!(device_status & DEVICE_ON_HAND_ERROR_MASK)) ? 0 : 1;
        	eda_data.data = eda_data.data_last_valid;
        	eda_data.data_convert_kom = eda_data.data_convert_last_valid; // передаем последнее актуальное значение
        	break;
        }
        else
        {
            eda_data.esk_code = (!(device_status & DEVICE_ON_HAND_ERROR_MASK)) ? (eda_data.esk_code << 2) & 0xFC : ((eda_data.esk_code << 2) & 0xFC) | 0x01;
            break;
        }
#else

        eda_data.esk_code = (onHand) ? (eda_data.esk_code << 2) & 0xFC : ((eda_data.esk_code << 2) & 0xFC) | 0x01;
        break;

#endif
    }
/*
    if (device_status & SYSTEM_MENU_BLE_TEST_DATA_MASK)
    {
      eda_data.esk_code = (ble_test_data << 2) & 0xFC;
      ble_test_data++;
    }
*/
	// формирование пакета в соответствии с характеристикой DataEDR.
    if(!(eda_data.measure_number & 0x0001)) // "0" в младшем бите, выдаем данные каждый четный такт, measure_number считается с 1
    {

      DataEdrPack[0] = 0x20 | (DataEdrPackCounter & 0x0F);
      DataEdrPack[1] = DataEdrPack[3];
      DataEdrPack[2] = DataEdrPack[4];
      DataEdrPack[3] = DataEdrPack[5];
      DataEdrPack[4] = DataEdrPack[6];
      DataEdrPack[5] = eda_data.esk_code_prev;
      DataEdrPack[6] = eda_data.esk_code;
      DataEdrPack[7] = (uint16_t)eda_data.logarithm_convert_prev;
      DataEdrPack[8] = (uint16_t)eda_data.logarithm_convert_prev >> 8;
      DataEdrPack[9] = (uint16_t)eda_data.logarithm_convert;
      DataEdrPack[10] = (uint16_t)eda_data.logarithm_convert >> 8;
      DataEdrPackCounter++;

      BBStream_TSKBMSend(DataEdrPack);
    }


}

int TSKBMInfoReadCmdWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_TSKBM);
    u8 len = p->l2capLen - 3;
    if (len >= 2)
    {
    	memcpy(&TSKBMInformationReadCmd, p->value, 2);
    }

    neurocomDbg(0x1FFF1, p, 0);

    return 0;
}

int TSKBMInfoReadCmdRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
	BBStream_SetProtocol(connHandle, CP_TSKBM);
	neurocomDbg(0x1FFF1, TSKBMInformationReadCmd, 2);
}

int TSKBMRandomWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_TSKBM);
    u8 len = p->l2capLen - 3;
    if (len > 2)
    {
    	memcpy(&TSKBMRandomValue, p->value, 3);
    	Crypto_Encrypt(TSKBMRandomValue, TSKBMEncryptedValue);
    }

    neurocomDbg(0x1FFF2, p, 0);
    tlkapi_send_str_data("Chipher text -> ", TSKBMRandomValue, sizeof(TSKBMRandomValue));
    tlkapi_send_str_data("Encrypt data -> ", TSKBMEncryptedValue, sizeof(TSKBMEncryptedValue));

    return 0;
}

int TSKBMRandomRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
	BBStream_SetProtocol(connHandle, CP_TSKBM);
	neurocomDbg(0x1FFF2, TSKBMRandomValue, 3);
}

int TSKBMEncryptedRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
	BBStream_SetProtocol(connHandle, CP_TSKBM);
	neurocomDbg(0x1FFF3, TSKBMEncryptedValue, 3);
}

int TSKBMInfoWriteCmdWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_TSKBM);
    u8 len = p->l2capLen - 3;
    if (len >= 6)
    {
    	memcpy(&TSKBMInformationWriteCmd, p->value, 6);
    }

    neurocomDbg(0x1FFF5, p, 0);

    return 0;
}

int TSKBMInfoWriteCmdRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
	BBStream_SetProtocol(connHandle, CP_TSKBM);
	TSKBMInformationWriteCmd[1] = 0xFF;
	TSKBMInformationWriteCmd[2] = 0;
	TSKBMInformationWriteCmd[3] = 0;
	TSKBMInformationWriteCmd[4] = 0;
	TSKBMInformationWriteCmd[5] = 0;
	neurocomDbg(0x1FFF5, TSKBMInformationWriteCmd, 6);
}
#endif

//// GAP attribute values (1800)
static const u8 my_devNameCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(GenericAccess_DeviceName_DP_H),
    U16_HI(GenericAccess_DeviceName_DP_H),
    U16_LO(GATT_UUID_DEVICE_NAME),
    U16_HI(GATT_UUID_DEVICE_NAME)};
static const u8 my_appearanceCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(GenericAccess_Appearance_DP_H),
    U16_HI(GenericAccess_Appearance_DP_H),
    U16_LO(GATT_UUID_APPEARANCE),
    U16_HI(GATT_UUID_APPEARANCE)};
static const u8 my_periConnParamCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(CONN_PARAM_DP_H),
    U16_HI(CONN_PARAM_DP_H),
    U16_LO(GATT_UUID_PERI_CONN_PARAM),
    U16_HI(GATT_UUID_PERI_CONN_PARAM)};


//// GATT attribute values (1801)
static const u8 my_serviceChangeCharVal[5] = {
    CHAR_PROP_INDICATE,
    U16_LO(GenericAttribute_ServiceChanged_DP_H),
    U16_HI(GenericAttribute_ServiceChanged_DP_H),
    U16_LO(GATT_UUID_SERVICE_CHANGE),
    U16_HI(GATT_UUID_SERVICE_CHANGE)};


//// device Information  attribute values (180A)

static const u8 my_ModelNumberVal[5] = {
	CHAR_PROP_READ,
    U16_LO(Characteristic_UUID_model_number_DP_H),
    U16_HI(Characteristic_UUID_model_number_DP_H),
    U16_LO(CHARACTERISTIC_UUID_MODEL_NUMBER_STRING),
    U16_HI(CHARACTERISTIC_UUID_MODEL_NUMBER_STRING)};
static const u8 my_FirmwareRevisionVal[5] = {
	CHAR_PROP_READ,
    U16_LO(Characteristic_UUID_firmware_revision_DP_H),
    U16_HI(Characteristic_UUID_firmware_revision_DP_H),
    U16_LO(CHARACTERISTIC_UUID_FIRMWARE_REVISION_STRING),
    U16_HI(CHARACTERISTIC_UUID_FIRMWARE_REVISION_STRING)};
static const u8 my_SoftwareRevisionVal[5] = {
	CHAR_PROP_READ,
    U16_LO(Characteristic_UUID_software_revision_DP_H),
    U16_HI(Characteristic_UUID_software_revision_DP_H),
    U16_LO(CHARACTERISTIC_UUID_SOFTWARE_REVISION_STRING),
    U16_HI(CHARACTERISTIC_UUID_SOFTWARE_REVISION_STRING)};
static const u8 my_ManufacturerNameVal[5] = {
	CHAR_PROP_READ,
    U16_LO(Characteristic_UUID_manufacturer_name_DP_H),
    U16_HI(Characteristic_UUID_manufacturer_name_DP_H),
    U16_LO(CHARACTERISTIC_UUID_MANUFACTURER_NAME_STRING),
    U16_HI(CHARACTERISTIC_UUID_MANUFACTURER_NAME_STRING)};

#if SERVICE_SPRV != 0
static const u8 neurocomDataStreamVal[19] = {
	CHAR_PROP_NOTIFY,
    U16_LO(Neurocom_Data_Stream_DP_H),
    U16_HI(Neurocom_Data_Stream_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_DATA_STREAM_CHAR)};

static const u8 neurocomConfigVal[19] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE,
    U16_LO(Neurocom_Config_DP_H),
    U16_HI(Neurocom_Config_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_CONFIG_CHAR)};

static const u8 neurocomBBFlashDataVal[19] = {
	CHAR_PROP_NOTIFY,
    U16_LO(Neurocom_BB_Flash_Data_DP_H),
    U16_HI(Neurocom_BB_Flash_Data_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_BB_FLASH_DATA_CHAR)};

static const u8 neurocomBBFlashCmdVal[19] = {
	CHAR_PROP_WRITE,
    U16_LO(Neurocom_BB_Flash_Cmd_DP_H),
    U16_HI(Neurocom_BB_Flash_Cmd_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_BB_FLASH_CMD_CHAR)};

static const u8 neurocomInfoVal[19] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE,
    U16_LO(Neurocom_Info_DP_H),
    U16_HI(Neurocom_Info_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_INFO_CHAR)};

static const u8 neurocomInfoFlashVal[19] = {
		CHAR_PROP_READ | CHAR_PROP_WRITE,
    U16_LO(Neurocom_Info_Flash_DP_H),
    U16_HI(Neurocom_Info_Flash_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_INFO_FLASH_CHAR)};

static const u8 neurocomCmdVal[19] = {
	CHAR_PROP_WRITE,
    U16_LO(Neurocom_Cmd_DP_H),
    U16_HI(Neurocom_Cmd_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_CMD_CHAR)};

static const u8 neurocomDiagnosticVal[19] = {
	CHAR_PROP_READ,
    U16_LO(Neurocom_Diagnostic_DP_H),
    U16_HI(Neurocom_Diagnostic_DP_H),
	USER_CHRC_ARR(NEUROCOM_MAIN_SERVICE, NEUROCOM_DIAGNOSTIC_CHAR)};


static u8 neurocomStreamData[] = {0, 0};
static u8 neurocomStreamDataCCC[] = {0, 0};
static u8 neurocomConfig[12] = {'0', '1', 32, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static u8 neurocomFlashInfo[14] = {0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static u8 neurocomFlashCmd[2];
static u8 neurocomInfo[12] = {0, 1, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static u8 neurocomCmd[3];
static u8 neurocomDiagnostic[13] = {0, 0, 0xC8, 0x97, 0x04, 0, 0, 0, 0xA8, 0x61, 120, 0xF2, 0x30};

uint32_t Neurocom_TimeTick(void)
{
	uint32_t btime;

	memcpy(&btime, &neurocomInfo[4], 4);
	btime++;
	memcpy(&neurocomInfo[4], &btime, 4);

	memcpy(&btime, &neurocomInfo[8], 4);
	if (btime)
	{
	  btime++;
	  memcpy(&neurocomInfo[8], &btime, 4);
	}
	return btime;
}

int neurocomInfoWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    u8 len = p->l2capLen - 3;
    if (len > 3)
    {
    	static const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEC"};
    	struct tm UTC2time(const time_t timer);
    	struct tm rttime;
    	time_t rtime;
    	memcpy(&neurocomInfo[8], p->value, 4);
    	rtime = Neurocom_TimeTick();
    	rttime = UTC2time(rtime + (3600 * 3));
        tlk_printf("Set time: %02d:%02d %02d.%s.%d\n", rttime.tm_hour,
        		                                       rttime.tm_min,
										               rttime.tm_mday,
										               months[rttime.tm_mon - 1],
										               rttime.tm_year);
    }

    neurocomDbg(0xFFF7, p, 0);

    return 0;
}

int neurocomInfoRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    neurocomDbg(0xFFF7, neurocomInfo, sizeof(neurocomInfo));

    return 0;
}

int neurocomConfigWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    u8 len = p->l2capLen - 3;
    if (len <= 12)
    {
    	memcpy(&neurocomConfig, p->value, len);
    }

    neurocomDbg(0xFFF4, p, 0);

    return 0;
}

int neurocomConfigRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    neurocomDbg(0xFFF4, neurocomConfig, sizeof(neurocomConfig));

    return 0;
}


int neurocomFlashCmdWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    u8 len = p->l2capLen - 3;
    if (len <= 2)
    {
    	memcpy(&neurocomFlashCmd, p->value, len);
    }

    neurocomDbg(0xFFF6, p, 0);

    return 0;
}

int neurocomFlashInfoWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    u8 len = p->l2capLen - 3;
    if (len <= 14)
    {
    	memcpy(&neurocomFlashInfo, p->value, len);
    }

    neurocomDbg(0xFFF8, p, 0);

    return 0;
}

int neurocomFlashInfoRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    neurocomDbg(0xFFF8, neurocomFlashInfo, sizeof(neurocomFlashInfo));

    return 0;
}

int neurocomCmdWrite(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    u8 len = p->l2capLen - 3;
    if (len <= 3)
    {
    	memcpy(&neurocomCmd, p->value, len);
    	if (neurocomCmd[0] == 1)
    	{
    		void Sensors_ResetAlarm(void);
    		Sensors_ResetAlarm();
    	}
    }

    neurocomDbg(0xFFF9, p, 0);

    return 0;
}

int neurocomDiagnosticRead(u16 connHandle, ble_rf_packet_att_write_t *p)
{
    (void)connHandle;
    BBStream_SetProtocol(connHandle, CP_SPRV);
    neurocomDbg(0xFFFA, neurocomDiagnostic, sizeof(neurocomDiagnostic));

    return 0;
}
#else

uint32_t UNIX_time = 0;

uint32_t Neurocom_TimeTick(void)
{
	UNIX_time++;
	return UNIX_time;
}

#endif

// TM : to modify
static const attribute_t my_Attributes[] = {

    {ATT_END_H - 1,                         0,                     0,  0,                                         NULL,                                            NULL,                                              NULL,                                         NULL}, // total num of attribute


    // 0001 - 0007  gap
    {7,                                     ATT_PERMISSIONS_READ,  2,  2,                                         (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&my_gapServiceUUID),                0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_devNameCharVal),                 (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_devNameCharVal),                 0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_devName),                        (u8 *)(size_t)(&my_devNameUUID),                 (u8 *)(size_t)(my_devName),                        0,                                            0   },
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_appearanceCharVal),              (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_appearanceCharVal),              0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_appearance),                     (u8 *)(size_t)(&my_appearanceUUID),              (u8 *)(size_t)(&my_appearance),                    0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_periConnParamCharVal),           (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_periConnParamCharVal),           0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_periConnParameters),             (u8 *)(size_t)(&my_periConnParamUUID),           (u8 *)(size_t)(&my_periConnParameters),            0,                                            0   },


    // 0008 - 000b gatt
    {4,                                     ATT_PERMISSIONS_READ,  2,  2,                                         (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&my_gattServiceUUID),               0,                                            0   },
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_serviceChangeCharVal),           (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_serviceChangeCharVal),           0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(serviceChangeVal),                  (u8 *)(size_t)(&serviceChangeUUID),              (u8 *)(&serviceChangeVal),                         0,                                            0   },
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(serviceChangeCCC),                  (u8 *)(size_t)(&clientCharacterCfgUUID),         (u8 *)(serviceChangeCCC),                          0,                                            0   },


    // 000c - 000e  device Information Service
    {9,                                     ATT_PERMISSIONS_READ,  2,  2,                                         (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&my_devServiceUUID),                0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_ModelNumberVal),                 (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_ModelNumberVal),                 0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(modelNumber),                       (u8 *)(size_t)(&my_modelNumberUUID),             (u8 *)(size_t)(modelNumber),                       0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_FirmwareRevisionVal),            (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_FirmwareRevisionVal),            0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(firmwareRevision),                  (u8 *)(size_t)(&my_firmwareRevisionUUID),        (u8 *)(size_t)(firmwareRevision),                  0,                                            0   },
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_SoftwareRevisionVal),            (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_SoftwareRevisionVal),            0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(softwareRevision),                  (u8 *)(size_t)(&my_softwareRevisionUUID),        (u8 *)(size_t)(softwareRevision),                  0,                                            0   },
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_ManufacturerNameVal),            (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_ManufacturerNameVal),            0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(manufacturerName),                  (u8 *)(size_t)(&my_manufacturerNameUUID),        (u8 *)(size_t)(manufacturerName),                  0,                                            0   },
#if SERVICE_SPRV != 0
    // SPRV service
    {18,                                    ATT_PERMISSIONS_READ,  2,  16,                                        (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&neurocomWristbandService),         0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomDataStreamVal),             (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomDataStreamVal),             0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_READ,  16, sizeof(neurocomStreamData),                (u8 *)(size_t)(&neurocomDataStreamChar),         (&neurocomStreamData),                             0,                                            0   }, //value
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(neurocomStreamDataCCC),             (u8 *)(size_t)(&clientCharacterCfgUUID),         (u8 *)(neurocomStreamDataCCC),                     0,                                            0   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomConfigVal),                 (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomConfigVal),                 0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_RDWR,  16, sizeof(neurocomConfig),                    (u8 *)(size_t)(&neurocomConfigChar),             (u8 *)(neurocomConfig),                            neurocomConfigWrite,                          neurocomConfigRead   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomBBFlashDataVal),            (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomBBFlashDataVal),            0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_READ,  16, sizeof(neurocomStreamData),                (u8 *)(size_t)(&neurocomBBFlashDataChar),        (u8 *)(neurocomStreamData),                        0,                                            0   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomBBFlashCmdVal),             (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomBBFlashCmdVal),             0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_WRITE, 16, sizeof(neurocomFlashCmd),                  (u8 *)(size_t)(&neurocomBBFlashCmdChar),         (u8 *)(neurocomFlashCmd),                          neurocomFlashCmdWrite,                        0   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomInfoVal),                   (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomInfoVal),                   0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_RDWR,  16, sizeof(neurocomInfo),                      (u8 *)(size_t)(&neurocomInfoChar),               (u8 *)(neurocomInfo),                              neurocomInfoWrite,                            neurocomInfoRead   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomInfoFlashVal),              (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomInfoFlashVal),              0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_RDWR,  16, sizeof(neurocomFlashInfo),                 (u8 *)(size_t)(&neurocomInfoFlashChar),          (u8 *)(neurocomFlashInfo),                         neurocomFlashInfoWrite,                       neurocomFlashInfoRead   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomCmdVal),                    (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomCmdVal),                    0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_WRITE, 16, sizeof(neurocomCmd),                       (u8 *)(size_t)(&neurocomCmdChar),                (u8 *)(neurocomCmd),                               neurocomCmdWrite,                             0   }, //value
	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(neurocomDiagnosticVal),             (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(neurocomDiagnosticVal),             0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_READ,  16, sizeof(neurocomDiagnostic),                (u8 *)(size_t)(&neurocomDiagChar),               (u8 *)(neurocomDiagnostic),                        0,                                            neurocomDiagnosticRead   }, //value
#endif

#if SERVICE_TSKBM != 0
	// TSKBM service
    {12,                                    ATT_PERMISSIONS_READ,  2,  2,                                         (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&TSKBMMainService),                 0,                                            0   },

	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMInfoReqVal),                   (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(TSKBMInfoReqVal),                   0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(TSKBMInformationReadCmd),           (u8 *)(size_t)(&TSKBMInfoReq),                   (u8 *)(size_t)(TSKBMInformationReadCmd),           TSKBMInfoReadCmdWrite,                        TSKBMInfoReadCmdRead   }, //value

	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMSecurityReqVal),               (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(TSKBMSecurityReqVal),               0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(TSKBMRandomValue),                  (u8 *)(size_t)(&TSKBMSecurityReq),               (u8 *)(size_t)(TSKBMRandomValue),                  TSKBMRandomWrite,                             TSKBMRandomRead   }, //value

	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMSecurityRespVal),              (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(TSKBMSecurityRespVal),              0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMEncryptedValue),               (u8 *)(size_t)(&TSKBMSecurityResp),              (u8 *)(size_t)(TSKBMEncryptedValue),               0,                                            TSKBMEncryptedRead   }, //value

    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMDataEDRVal),                   (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(TSKBMDataEDRVal),                   0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMDataEDRValue),                 (u8 *)(size_t)(&TSKBMDataEDR),                   (u8 *)(size_t)(&TSKBMDataEDRValue),                0,                                            0   }, //value
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(TSKBMDataEDRCCCValue),              (u8 *)(size_t)(&clientCharacterCfgUUID),         (u8 *)(size_t)(TSKBMDataEDRCCCValue),              0,                                            0   }, //value

	{0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(TSKBMInformationRespVal),           (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(TSKBMInformationRespVal),           0,                                            0   }, //prop
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(TSKBMInformationWriteCmd),          (u8 *)(size_t)(&TSKBMInformationResp),           (u8 *)(size_t)(TSKBMInformationWriteCmd),          TSKBMInfoWriteCmdWrite,                       TSKBMInfoWriteCmdRead   }, //value
#endif
};

/**
 * @brief   GATT initialization.
 *          !!!Note: this function is used to register ATT table to BLE Stack.
 * @param   none.
 * @return  none.
 */
void my_gatt_init(void)
{
    bls_att_setAttributeTable((u8 *)(size_t)my_Attributes);
}
