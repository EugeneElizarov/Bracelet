/*
 * crypto.c
 *
 *  Created on: 16 апр. 2026 г.
 *      Author: eugen
 */

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "crypto.h"
#include "errno.h"

#include "tl_common.h"

static const uint8_t key[] = {0x6E, 0xC4, 0x30, 0xD8, 0xCB, 0xC2, 0x10, 0x62,
		                      0xD7, 0x6B, 0x97, 0xC2, 0x60, 0x81, 0x2B, 0xCB};

void Crypto_Encrypt(void *indata, void *outdata)
{
	  uint8_t intext[16], outtext[16];
	  uint8_t *data = indata;
	  memset(intext, 0, sizeof(intext));
	  intext[15] = data[0];
	  intext[14] = data[1];
	  intext[13] = data[2];
	  ske_lp_crypto(SKE_ALG_AES_128, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT,
			        key, 0, NULL, intext, outtext, 16);
	  data = outdata;
	  data[0] = outtext[15];
	  data[1] = outtext[14];
	  data[2] = outtext[13];

}
