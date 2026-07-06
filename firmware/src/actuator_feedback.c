/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "actuator_feedback.h"

enum epi_feedback_kind epi_feedback_from_result(const struct verify_result *res)
{
	if (res == NULL || res->status != VERIFY_OK) {
		return EPI_FEEDBACK_ERROR;
	}

	if (res->unknown_person) {
		return EPI_FEEDBACK_UNKNOWN;
	}

	if (res->allowed) {
		return EPI_FEEDBACK_ALLOWED;
	}

	return EPI_FEEDBACK_DENIED;
}
