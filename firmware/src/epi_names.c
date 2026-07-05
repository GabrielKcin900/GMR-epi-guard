/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "epi_names.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "zbus_channels.h"

struct epi_alias {
	const char *canonical;
	const char *ascii;
};

static const struct epi_alias aliases[] = {
	{ "Oculos", "Oculos" },
};

static size_t utf8_fold_char(const char *in, char *out)
{
	unsigned char b = (unsigned char)in[0];

	if (b < 0x80) {
		*out = in[0];
		return 1;
	}

	if ((b & 0xE0) == 0xC0 && in[1] != '\0') {
		unsigned char b2 = (unsigned char)in[1];

		if (b == 0xC3) {
			switch (b2) {
			case 0xA1:
			case 0xA0:
			case 0xA2:
			case 0xA3:
				*out = 'a';
				break;
			case 0xA7:
				*out = 'c';
				break;
			case 0xA9:
			case 0xA8:
				*out = 'e';
				break;
			case 0xAD:
			case 0xAC:
				*out = 'i';
				break;
			case 0xB3:
			case 0xB2:
			case 0xB5:
				*out = 'o';
				break;
			case 0xBA:
			case 0xB9:
				*out = 'u';
				break;
			case 0x81:
				*out = 'A';
				break;
			case 0x87:
				*out = 'C';
				break;
			case 0x89:
				*out = 'E';
				break;
			case 0x8D:
				*out = 'I';
				break;
			case 0x93:
				*out = 'O';
				break;
			case 0x9A:
				*out = 'U';
				break;
			default:
				*out = '?';
				break;
			}
			return 2;
		}
	}

	*out = '?';
	return 1;
}

void epi_name_to_ascii(const char *in, char *out, size_t out_len)
{
	size_t oi = 0;
	size_t consumed;

	if (in == NULL || out == NULL || out_len == 0) {
		return;
	}

	for (size_t i = 0; in[i] != '\0' && oi < out_len - 1; i += consumed) {
		char c;

		consumed = utf8_fold_char(&in[i], &c);
		if (c == '_') {
			c = ' ';
		}
		out[oi++] = c;
	}

	out[oi] = '\0';
}

static void normalize_epi_name(const char *in, char *out, size_t out_len)
{
	epi_name_to_ascii(in, out, out_len);
}

static bool normalized_equal(const char *a, const char *b)
{
	char na[EPI_NAME_LEN];
	char nb[EPI_NAME_LEN];

	normalize_epi_name(a, na, sizeof(na));
	normalize_epi_name(b, nb, sizeof(nb));

	return strcmp(na, nb) == 0;
}

bool epi_name_equal(const char *a, const char *b)
{
	if (a == NULL || b == NULL) {
		return false;
	}

	if (strcmp(a, b) == 0 || normalized_equal(a, b)) {
		return true;
	}

	for (size_t i = 0; i < ARRAY_SIZE(aliases); i++) {
		if ((normalized_equal(a, aliases[i].canonical) &&
		     normalized_equal(b, aliases[i].ascii)) ||
		    (normalized_equal(a, aliases[i].ascii) &&
		     normalized_equal(b, aliases[i].canonical))) {
			return true;
		}
	}

	return false;
}

bool epi_item_in_list(const char *item, const char items[][EPI_NAME_LEN], uint8_t count)
{
	for (uint8_t i = 0; i < count; i++) {
		if (epi_name_equal(item, items[i])) {
			return true;
		}
	}

	return false;
}
