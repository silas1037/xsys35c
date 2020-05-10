/* Copyright (C) 2020 <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/
#include "common.h"
#include "s2utbl.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t hankaku81[] = {
	0x20, 0xa4, 0xa1, 0x00, 0x00, 0xa5, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xa2, 0xa3, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t hankaku82[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa7,
	0xb1, 0xa8, 0xb2, 0xa9, 0xb3, 0xaa, 0xb4, 0xab,
	0xb5, 0xb6, 0x00, 0xb7, 0x00, 0xb8, 0x00, 0xb9,
	0x00, 0xba, 0x00, 0xbb, 0x00, 0xbc, 0x00, 0xbd,
	0x00, 0xbe, 0x00, 0xbf, 0x00, 0xc0, 0x00, 0xc1,
	0x00, 0xaf, 0xc2, 0x00, 0xc3, 0x00, 0xc4, 0x00,
	0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0x00, 0x00,
	0xcb, 0x00, 0x00, 0xcc, 0x00, 0x00, 0xcd, 0x00,
	0x00, 0xce, 0x00, 0x00, 0xcf, 0xd0, 0xd1, 0xd2,
	0xd3, 0xac, 0xd4, 0xad, 0xd5, 0xae, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0x00, 0xdc, 0x00, 0x00,
	0xa6, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint16_t kanatbl[] = {
	0x8140, 0x8142, 0x8175, 0x8176, 0x8141, 0x8145, 0x82f0, 0x829f,
	0x82a1, 0x82a3, 0x82a5, 0x82a7, 0x82e1, 0x82e3, 0x82e5, 0x82c1,
	0x815b, 0x82a0, 0x82a2, 0x82a4, 0x82a6, 0x82a8, 0x82a9, 0x82ab,
	0x82ad, 0x82af, 0x82b1, 0x82b3, 0x82b5, 0x82b7, 0x82b9, 0x82bb,
	0x82bd, 0x82bf, 0x82c2, 0x82c4, 0x82c6, 0x82c8, 0x82c9, 0x82ca,
	0x82cb, 0x82cc, 0x82cd, 0x82d0, 0x82d3, 0x82d6, 0x82d9, 0x82dc,
	0x82dd, 0x82de, 0x82df, 0x82e0, 0x82e2, 0x82e4, 0x82e6, 0x82e7,
	0x82e8, 0x82e9, 0x82ea, 0x82eb, 0x82ed, 0x82f1, 0x814a, 0x814b
};

static int unicode_to_sjis(int u) {
	for (int b1 = 0x81; b1 <= 0xff; b1++) {
		if (b1 >= 0xa0 && b1 <= 0xdf)
			continue;
		for (int b2 = 0x40; b2 <= 0xff; b2++) {
			if (u == s2u[b1 - 0x80][b2 - 0x40])
				return b1 << 8 | b2;
		}
	}
	return 0;
}

bool is_valid_sjis(uint8_t c1, uint8_t c2) {
	return is_sjis_byte1(c1) && is_sjis_byte2(c2) && s2u[c1 - 0x80][c2 - 0x40];
}

char *sjis2utf_sub(const char *str, int substitution_char) {
#ifdef SJIS_NATIVE
	return strdup(str);
#else
	const uint8_t *src = (uint8_t *)str;
	uint8_t *dst = malloc(strlen(str) * 3 + 1);
	uint8_t *dstp = dst;

	while (*src) {
		if (*src <= 0x7f) {
			*dstp++ = *src++;
			continue;
		}

		int c;
		if (*src >= 0xa0 && *src <= 0xdf) {
			c = 0xff60 + *src - 0xa0;
			src++;
		} else if (is_valid_sjis(src[0], src[1])) {
			c = s2u[src[0] - 0x80][src[1] - 0x40];
			src += 2;
		} else {
			if (substitution_char < 0)
				error("Invalid SJIS byte sequence %02x %02x", src[0], src[1]);
			c = substitution_char;
			src++;
		}

		if (c <= 0x7f) {
			*dstp++ = c;
		} else if (c <= 0x7ff) {
			*dstp++ = 0xc0 | c >> 6;
			*dstp++ = 0x80 | (c & 0x3f);
		} else {
			*dstp++ = 0xe0 | c >> 12;
			*dstp++ = 0x80 | (c >> 6 & 0x3f);
			*dstp++ = 0x80 | (c & 0x3f);
		}
	}
	*dstp = '\0';
	return (char *)dst;
#endif
}

char *utf2sjis_sub(const char *str, int substitution_char) {
#ifdef SJIS_NATIVE
	return strdup(str);
#else
	const uint8_t *src = (uint8_t *)str;
	uint8_t *dst = malloc(strlen(str) + 1);
	uint8_t *dstp = dst;

	while (*src) {
		if (*src <= 0x7f) {
			*dstp++ = *src++;
			continue;
		}

		int u;
		if (*src <= 0xdf) {
			u = (src[0] & 0x1f) << 6 | (src[1] & 0x3f);
			src += 2;
		} else if (*src <= 0xef) {
			u = (src[0] & 0xf) << 12 | (src[1] & 0x3f) << 6 | (src[2] & 0x3f);
			src += 3;
		} else {
			if (substitution_char < 0)
				error("Unsupported UTF-8 sequence");
			*dstp++ = substitution_char;
			do src++; while ((*src & 0xc0) == 0x80);
			continue;
		}

		if (u > 0xff60 && u <= 0xff9f) {
			*dstp++ = u - 0xff60 + 0xa0;
		} else {
			int c = unicode_to_sjis(u);
			if (c) {
				*dstp++ = c >> 8;
				*dstp++ = c & 0xff;
			} else {
				if (substitution_char < 0)
					error("Codepoint U+%04X cannot be converted to Shift_JIS", u);
				*dstp++ = substitution_char;
			}
		}
	}
	*dstp = '\0';
	return (char*)dst;
#endif
}

uint8_t to_sjis_half_kana(uint8_t c1, uint8_t c2) {
	return c1 == 0x81 ? hankaku81[c2 - 0x40] :
		   c1 == 0x82 ? hankaku82[c2 - 0x40] : 0;
}

uint16_t from_sjis_half_kana(uint8_t c) {
	return kanatbl[c - 0xa0];
}
