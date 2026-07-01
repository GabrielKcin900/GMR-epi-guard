/*
 * SPDX-License-Identifier: Apache-2.0
 * EPI Guard — canais zbus e structs de mensagem (plano §5.2)
 */

#ifndef EPI_ZBUS_CHANNELS_H_
#define EPI_ZBUS_CHANNELS_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/zbus/zbus.h>

#define EPI_MAX_ITEMS 8
#define EPI_NAME_LEN  24
#define EPI_WHO_LEN   32

struct verify_request {
	uint32_t req_id;
	char     who[EPI_WHO_LEN];
	char     items[EPI_MAX_ITEMS][EPI_NAME_LEN];
	uint8_t  item_count;
};

enum verify_status {
	VERIFY_OK = 0,
	VERIFY_ERROR,
};

struct verify_result {
	uint32_t           req_id;
	enum verify_status status;
	bool               allowed;
	bool               unknown_person;
	char               missing[EPI_MAX_ITEMS][EPI_NAME_LEN];
	uint8_t            missing_count;
};

ZBUS_CHAN_DECLARE(chan_verify_request);
ZBUS_CHAN_DECLARE(chan_verify_result);

#endif /* EPI_ZBUS_CHANNELS_H_ */
