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
	{ "Óculos", "Oculos" },
};

static int ascii_fold(char c)
{
	switch (c) {
	case 'ó':
	case 'Ó':
		return 'o';
	case 'á':
	case 'Á':
		return 'a';
	case 'ã':
	case 'Ã':
		return 'a';
	case 'ç':
	case 'Ç':
		return 'c';
	case 'é':
	case 'É':
		return 'e';
	case 'í':
	case 'Í':
		return 'i';
	case 'ú':
	case 'Ú':
		return 'u';
	default:
		return c;
	}
}

static void normalize_epi_name(const char *in, char *out, size_t out_len)
{
	size_t i;

	if (out_len == 0) {
		return;
	}

	for (i = 0; in[i] != '\0' && i < out_len - 1; i++) {
		char c = in[i];

		if (c == '_') {
			c = ' ';
		}
		out[i] = (char)ascii_fold(c);
	}
	out[i] = '\0';
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
