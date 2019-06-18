/*
 * Copyright (c) 2014 Jonas Eriksson
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>

static inline uint16_t crc16_ccitt_init(void) {
	return 0xFFFF;
}

static inline uint16_t crc16_ccitt_update(uint8_t byte, uint16_t crc) {
	int i;
	int xor_flag;

	/* For each bit in the data byte, starting from the leftmost bit */
	for (i = 7; i >= 0; i--) {
		/* If leftmost bit of the CRC is 1, we will XOR with
		 * the polynomial later */
		xor_flag = crc & 0x8000;

		/* Shift the CRC, and append the next bit of the
		 * message to the rightmost side of the CRC */
		crc <<= 1;
		crc |= (byte & (1 << i)) ? 1 : 0;

		/* Perform the XOR with the polynomial */
		if (xor_flag)
			crc ^= 0x1021;
	}

	return crc;
}

static inline uint16_t crc16_ccitt_finalize(uint16_t crc) {
	int i;

	/* Augment 16 zero-bits */
	for (i = 0; i < 2; i++) {
		crc = crc16_ccitt_update(0, crc);
	}

	return crc;
}
