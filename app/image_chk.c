/*
 * image_chk.c
 *
 *  Created on: Oct 10, 2020
 *      Author: zjm09
 */
#include "main.h"
#if 0
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"

extern int _image_start;
extern int _image_end;

//mbedtls_rsa_context ctx;
mbedtls_pk_context ctx_pk;
mbedtls_sha256_context sha256;

int image_chk(void)
{
	int image_size = _image_start - _image_end;
	uint8_t sha256_hash[32];

	mbedtls_sha256_init(&sha256);
	mbedtls_sha256_update(&sha256, (void *)&_image_start, image_size);
	mbedtls_sha256_finish(&sha256, (void *)sha256_hash);

	mbedtls_pk_init(&ctx_pk);

	mbedtls_pk_parse_public_key(&ctx_pk, NULL, 0);

//	mbedtls_rsa_init(&ctx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

	mbedtls_rsa_pkcs1_verify(NULL, NULL, NULL, 0, 0, 0, NULL, NULL);

	return 0;
}
#endif
