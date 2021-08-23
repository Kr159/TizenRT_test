/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
#pragma once

#include <stdint.h>
/*
 * Desc: Get ED25519
 * This key pair is verified in http://ed25519.herokuapp.com
 */

#if 0
/* g_ed25519_privakey contains public key. This key is provided
 * for verify the ed25519 feature on other platforms.
 * however it caused build error. So comment it */
static uint8_t g_ed25519_privkey[] = {
	0x37, 0x31, 0x64, 0x65, 0x48, 0x53, 0x4d, 0x42,
	0x4e, 0x4e, 0x56, 0x49, 0x50, 0x39, 0x71, 0x68,
	0x55, 0x51, 0x75, 0x74, 0x36, 0x36, 0x6d, 0x6f,
	0x30, 0x4a, 0x65, 0x41, 0x65, 0x79, 0x42, 0x62,
	0x58, 0x74, 0x61, 0x55, 0x70, 0x62, 0x73, 0x35,
	0x7a, 0x75, 0x47, 0x52, 0x37, 0x53, 0x53, 0x49,
	0x55, 0x74, 0x48, 0x54, 0x76, 0x6d, 0x50, 0x35,
	0x4f, 0x51, 0x73, 0x32, 0x51, 0x48, 0x34, 0x58,
	0x39, 0x65, 0x38, 0x45, 0x57, 0x6a, 0x43, 0x70,
	0x37, 0x35, 0x74, 0x4b, 0x43, 0x63, 0x43, 0x4d,
	0x62, 0x4b, 0x63, 0x63, 0x37, 0x51, 0x3d, 0x3d};
#endif
static uint8_t g_ed25519_privkey_only[] = {
	0xef, 0x57, 0x5e, 0x1d, 0x23, 0x01, 0x34, 0xd5,
	0x48, 0x3f, 0xda, 0xa1, 0x51, 0x0b, 0xad, 0xeb,
	0xa9, 0xa8, 0xd0, 0x97, 0x80, 0x7b, 0x20, 0x5b,
	0x5e, 0xd6, 0x94, 0xa5, 0xbb, 0x39, 0xce, 0xe1};


static uint8_t g_ed25519_pubkey[] = {
	0x91, 0xed, 0x24, 0x88, 0x52, 0xd1, 0xd3, 0xbe,
	0x63, 0xf9, 0x39, 0x0b, 0x36, 0x40, 0x7e, 0x17,
	0xf5, 0xef, 0x04, 0x5a, 0x30, 0xa9, 0xef, 0x9b,
	0x4a, 0x09, 0xc0, 0x8c, 0x6c, 0xa7, 0x1c, 0xed};

static const char g_test_crt[] =
		"-----BEGIN CERTIFICATE-----\r\n"
		"MIICaDCCAgygAwIBAgIBAjAMBggqhkjOPQQDAgUAMHAxLTArBgNVBAMTJFNhbXN1\r\n"
		"bmcgRWxlY3Ryb25pY3MgT0NGIFJvb3QgQ0EgVEVTVDEUMBIGA1UECxMLT0NGIFJv\r\n"
		"b3QgQ0ExHDAaBgNVBAoTE1NhbXN1bmcgRWxlY3Ryb25pY3MxCzAJBgNVBAYTAktS\r\n"
		"MCAXDTE2MTEyNDAyNDcyN1oYDzIwNjkxMjMxMTQ1OTU5WjBwMS0wKwYDVQQDEyRT\r\n"
		"YW1zdW5nIEVsZWN0cm9uaWNzIE9DRiBSb290IENBIFRFU1QxFDASBgNVBAsTC09D\r\n"
		"RiBSb290IENBMRwwGgYDVQQKExNTYW1zdW5nIEVsZWN0cm9uaWNzMQswCQYDVQQG\r\n"
		"EwJLUjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABBzzury7p8HANVn+v4CIa2h/\r\n"
		"R/SAt3VVst+vTv4/kR+lgU1OEiT3t9+mOWE7J+oddpRofFW2DdeJkpfQUVOn4NOj\r\n"
		"gZIwgY8wDgYDVR0PAQH/BAQDAgHGMC4GA1UdHwQnMCUwI6AhoB+GHWh0dHA6Ly9j\r\n"
		"YS5zYW1zdW5naW90cy5jb20vY3JsMA8GA1UdEwEB/wQFMAMBAf8wPAYIKwYBBQUH\r\n"
		"AQEEMDAuMCwGCCsGAQUFBzABhiBodHRwOi8vb2NzcC10ZXN0LnNhbXN1bmdpb3Rz\r\n"
		"LmNvbTAMBggqhkjOPQQDAgUAA0gAMEUCIQCIsi3BcOQMXO/pCiUA+S75bYFWS27E\r\n"
		"GAq9e2E3+hQ2TAIgWrTieFAZ5xRH3BnSHG+XEF2HPD99y/SYSa6T59YW+jE=\r\n"
		"-----END CERTIFICATE-----\r\n\0";
