/*
 * SPDX-License-Identifier: Apache-2.0
 * Mock da API — lógica espelha api/db.json do plano (§6).
 */

#include "api_client.h"
#include "epi_names.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(api_client, LOG_LEVEL_INF);

struct person_rule {
	const char *name;
	const char *required[EPI_MAX_ITEMS];
	uint8_t     required_count;
};

static const struct person_rule people[] = {
	{
		.name = "Gabriel",
		.required = { "Capacete", "Óculos", "Cinto", "Bota" },
		.required_count = 4,
	},
	{
		.name = "Ana",
		.required = { "Capacete", "Luva" },
		.required_count = 2,
	},
	{
		.name = "Joao",
		.required = { "Capacete", "Óculos", "Protetor auricular" },
		.required_count = 3,
	},
};

static const char *default_required[] = { "Capacete", "Óculos", "Cinto" };
static const uint8_t default_required_count = 3;

static bool item_present(const struct verify_request *req, const char *name)
{
	return epi_item_in_list(name, req->items, req->item_count);
}

static void fill_missing(const struct verify_request *req,
			 const char *const *required, uint8_t required_count,
			 struct verify_result *out)
{
	out->missing_count = 0;

	for (uint8_t i = 0; i < required_count && out->missing_count < EPI_MAX_ITEMS; i++) {
		if (!item_present(req, required[i])) {
			strncpy(out->missing[out->missing_count], required[i], EPI_NAME_LEN - 1);
			out->missing[out->missing_count][EPI_NAME_LEN - 1] = '\0';
			out->missing_count++;
		}
	}

	out->allowed = (out->missing_count == 0);
}

int api_check(const struct verify_request *req, struct verify_result *out)
{
	const char *const *required = default_required;
	uint8_t required_count = default_required_count;

	if (req == NULL || out == NULL) {
		return -EINVAL;
	}

	out->status = VERIFY_OK;
	out->allowed = false;
	out->missing_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(people); i++) {
		if (strcmp(req->who, people[i].name) == 0) {
			required = people[i].required;
			required_count = people[i].required_count;
			break;
		}
	}

	fill_missing(req, required, required_count, out);

	LOG_INF("mock check %s: allowed=%d missing=%u", req->who, out->allowed,
		out->missing_count);

	return 0;
}
