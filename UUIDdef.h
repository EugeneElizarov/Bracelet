/*
 * UUIDdef.h
 *
 *  Created on: 4 апр. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_ACL_CONNECTION_DEMO_UUIDDEF_H_
#define VENDOR_ACL_CONNECTION_DEMO_UUIDDEF_H_

#include "def.h"

// Приведение 16-ричных символов к верхнему регистру
#define UPHEX(c)						(((c >= 'a') && (c <= 'f')) ? (c - 0x20) : (c))
// Проверка принадлежности символа к 16-ричному
#define VALID_HEX_CHAR(c)				(((UPHEX(c) >= '0') && (UPHEX(c) <= '9')) || ((UPHEX(c) >= 'A') && (UPHEX(c) <= 'F')))
// Получение кода из 16-ричного символа
#define CHAR2D(c)						(VALID_HEX_CHAR(c) ? (c > '9') ? (UPHEX(c) - 'A' + 10) : (c - '0') : 0)

#define D2CHAR(d)						((((d) & 0x0F) > 9) ? ((d) & 0x0F) + 'A' - 10 : ((d) & 0x0F) + '0')
// Извлечение байта из 16-ричной строки с заданного индекса
#define GET_BYTE_FROM_HEX(str, index)	((CHAR2D(str[index]) << 4) + CHAR2D(str[index + 1]))
// Определение валидности записи UUID в формате "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX", где X любой 16-ричный символ
#define VALID_UUID(uuid)										\
(																\
	(uuid[ 8] == '-')        && (uuid[13] == '-')        &&		\
    (uuid[18] == '-')        && (uuid[23] == '-')        &&		\
    VALID_HEX_CHAR(uuid[ 0]) && VALID_HEX_CHAR(uuid[ 1]) &&		\
	VALID_HEX_CHAR(uuid[ 2]) && VALID_HEX_CHAR(uuid[ 3]) && 	\
	VALID_HEX_CHAR(uuid[ 4]) &&	VALID_HEX_CHAR(uuid[ 5]) &&		\
	VALID_HEX_CHAR(uuid[ 6]) && VALID_HEX_CHAR(uuid[ 7]) &&		\
	VALID_HEX_CHAR(uuid[ 9]) &&	VALID_HEX_CHAR(uuid[10]) &&		\
	VALID_HEX_CHAR(uuid[11]) &&	VALID_HEX_CHAR(uuid[12]) &&		\
	VALID_HEX_CHAR(uuid[14]) &&	VALID_HEX_CHAR(uuid[15]) &&		\
	VALID_HEX_CHAR(uuid[16]) &&	VALID_HEX_CHAR(uuid[17]) &&		\
	VALID_HEX_CHAR(uuid[19]) &&	VALID_HEX_CHAR(uuid[20]) &&		\
	VALID_HEX_CHAR(uuid[21]) &&	VALID_HEX_CHAR(uuid[22]) &&		\
	VALID_HEX_CHAR(uuid[24]) &&	VALID_HEX_CHAR(uuid[25]) &&		\
	VALID_HEX_CHAR(uuid[26]) &&	VALID_HEX_CHAR(uuid[27]) &&		\
	VALID_HEX_CHAR(uuid[28]) &&	VALID_HEX_CHAR(uuid[29]) &&		\
	VALID_HEX_CHAR(uuid[30]) &&	VALID_HEX_CHAR(uuid[31]) &&		\
	VALID_HEX_CHAR(uuid[32]) &&	VALID_HEX_CHAR(uuid[33]) &&		\
	VALID_HEX_CHAR(uuid[34]) &&	VALID_HEX_CHAR(uuid[35])		\
)

// Формирование массива сервиса
#define USER_UUID_ARR(uuid)										\
	GET_BYTE_FROM_HEX(uuid, 34), GET_BYTE_FROM_HEX(uuid, 32),	\
	GET_BYTE_FROM_HEX(uuid, 30), GET_BYTE_FROM_HEX(uuid, 28),	\
	GET_BYTE_FROM_HEX(uuid, 26), GET_BYTE_FROM_HEX(uuid, 24),	\
	GET_BYTE_FROM_HEX(uuid, 21), GET_BYTE_FROM_HEX(uuid, 19),	\
	GET_BYTE_FROM_HEX(uuid, 16), GET_BYTE_FROM_HEX(uuid, 14),	\
	GET_BYTE_FROM_HEX(uuid, 11), GET_BYTE_FROM_HEX(uuid, 9),	\
	GET_BYTE_FROM_HEX(uuid, 6),	 GET_BYTE_FROM_HEX(uuid, 4),	\
	GET_BYTE_FROM_HEX(uuid, 2),	 GET_BYTE_FROM_HEX(uuid, 0)

// Формирование массива сервиса
#define USER_UUID(uuid)			{USER_UUID_ARR(uuid)}

// Формирование массива характеристики сервиса
#define USER_CHRC_ARR(uuid, chrc)								\
	GET_BYTE_FROM_HEX(uuid, 34), GET_BYTE_FROM_HEX(uuid, 32),	\
	GET_BYTE_FROM_HEX(uuid, 30), GET_BYTE_FROM_HEX(uuid, 28),	\
	GET_BYTE_FROM_HEX(uuid, 26), GET_BYTE_FROM_HEX(uuid, 24),	\
	GET_BYTE_FROM_HEX(uuid, 21), GET_BYTE_FROM_HEX(uuid, 19),	\
	GET_BYTE_FROM_HEX(uuid, 16), GET_BYTE_FROM_HEX(uuid, 14),	\
	GET_BYTE_FROM_HEX(uuid, 11), GET_BYTE_FROM_HEX(uuid,  9),	\
	(chrc) & 0xFF              , ((chrc) >> 8) & 0xFF       ,	\
	GET_BYTE_FROM_HEX(uuid,  2), GET_BYTE_FROM_HEX(uuid,  0)

// Формирование массива характеристики сервиса
#define USER_CHRC(uuid, chrc)	{USER_CHRC_ARR(uuid, chrc)}

#endif /* VENDOR_ACL_CONNECTION_DEMO_UUIDDEF_H_ */

/*
[0001]Flash read MID device num
[0002][FLASH][INI] flash initialization: 85 60 15 00 85 60 00 02 15 00 00 00
[0003][FLASH][INI] 2M Flash, MAC on 1ff000
[0004]DRead addr 001FE000: ff
[0005][FLASH][INI] user_calib_freq_offset value error
[0006]DRead addr 001FF000: ff ff ff ff ff ff ff ff
[0007]Public MAC: : ad 4c 00 28 22 38
[0008]Static MAC: : ad 4c 00 ff ff c0
[0009]DRead addr 001EDFF0: 3c
[000a]DRead addr 001EFFF0: ff
[000b]DRead addr 001EE000: ff ff ff ff
[000c]DRead addr 001EC000: ff ff
[000d][APP][INI] acl peripheral demo init:





[0001]Flash read MID device num
[0002][FLASH][INI] flash initialization: 85 60 15 00 85 60 00 02 15 00 00 00
[0003][FLASH][INI] 2M Flash, MAC on 1ff000
[0004]DRead addr 001FE000: ff
[0005][FLASH][INI] user_calib_freq_offset value error
[0006]DRead addr 001FF000: ff ff ff ff ff ff ff ff
[0007]Public MAC: : c9 a8 01 28 22 38
[0008]Static MAC: : c9 a8 01 ff ff c0
[0009]DRead addr 001EDFF0: 3c
[000a]DRead addr 001EFFF0: ff
[000b]DRead addr 001EE000: ff ff ff ff
[000c]DRead addr 001EC000: 9a 00
[000d]DRead addr 001EC060: 00 00
[000e]DRead addr 001EC0C0: 00 00
[000f]DRead addr 001EC120: 00 00
[0010]DRead addr 001EC180: 00 00
[0011]DRead addr 001EC1E0: 9a 00
[0012]DRead addr 001EC240: 9a 00
[0013]DRead addr 001EC2A0: ff ff
[0014][APP][INI] acl peripheral demo init:
[0015]Stream Timer Active
[0016]Main Cycle 200000 counts completed in 6244 ms



[0001]Flash read MID device num
[0002][FLASH][INI] flash initialization: 85 60 15 00 85 60 00 02 15 00 00 00
[0003][FLASH][INI] 2M Flash, MAC on 1ff000
[0004]RF_Read addr 001FE000: ff
[0005][FLASH][INI] user_calib_freq_offset value error
[0006]RF_Read addr 001FF000: ff ff ff ff ff ff ff ff
[0007]Public MAC: : c9 a8 01 28 22 38
[0008]Static MAC: : c9 a8 01 ff ff c0
[0009]RF_Read addr 001EDFF0: ff
[000a]RF_Read addr 001EFFF0: ff
[000b]RF_Write addr 001EDFF0: 3c
[000c]RF_Read addr 001EDFF0: 3c
[000d]RF_Read addr 001EC000: ff ff
[000e][APP][INI] acl peripheral demo init:
[000f]Stream Timer Active
[0010]Main Cycle 200000 counts completed in 6209 ms

*/
