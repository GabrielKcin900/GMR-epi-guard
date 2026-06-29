/*
 * SPDX-License-Identifier: Apache-2.0
 * Comparacao de nomes de EPI — aceita variantes ASCII (shell serial sem UTF-8).
 */

#ifndef EPI_NAMES_H_
#define EPI_NAMES_H_

#include <stdbool.h>
#include <stdint.h>

#include "zbus_channels.h"

bool epi_name_equal(const char *a, const char *b);
bool epi_item_in_list(const char *item, const char items[][EPI_NAME_LEN], uint8_t count);

#endif /* EPI_NAMES_H_ */
