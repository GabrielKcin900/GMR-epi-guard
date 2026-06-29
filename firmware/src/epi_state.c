/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "epi_state.h"

#include <zephyr/kernel.h>

static struct verify_request last_request;
static struct verify_result last_result;
static bool has_result;
static struct k_mutex state_lock;

static int epi_state_init(void)
{
	k_mutex_init(&state_lock);
	return 0;
}

SYS_INIT(epi_state_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

void epi_state_store_request(const struct verify_request *req)
{
	if (req == NULL) {
		return;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	last_request = *req;
	k_mutex_unlock(&state_lock);
}

void epi_state_store_result(const struct verify_result *res)
{
	if (res == NULL) {
		return;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	last_result = *res;
	has_result = true;
	k_mutex_unlock(&state_lock);
}

void epi_state_get_last_request(struct verify_request *req)
{
	if (req == NULL) {
		return;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	*req = last_request;
	k_mutex_unlock(&state_lock);
}

void epi_state_get_last_result(struct verify_result *res)
{
	if (res == NULL) {
		return;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	*res = last_result;
	k_mutex_unlock(&state_lock);
}

bool epi_state_has_result(void)
{
	bool value;

	k_mutex_lock(&state_lock, K_FOREVER);
	value = has_result;
	k_mutex_unlock(&state_lock);

	return value;
}
