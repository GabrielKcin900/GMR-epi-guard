/*
 * SPDX-License-Identifier: Apache-2.0
 * Mapeamento puro: resultado de verificacao -> padrao de feedback (LED/buzzer).
 */

#ifndef EPI_ACTUATOR_FEEDBACK_H_
#define EPI_ACTUATOR_FEEDBACK_H_

#include "zbus_channels.h"

enum epi_feedback_kind {
	EPI_FEEDBACK_ERROR,
	EPI_FEEDBACK_ALLOWED,
	EPI_FEEDBACK_DENIED,
	EPI_FEEDBACK_UNKNOWN,
};

/** Classifica o resultado para o padrao de atuador (testavel sem GPIO). */
enum epi_feedback_kind epi_feedback_from_result(const struct verify_result *res);

#endif /* EPI_ACTUATOR_FEEDBACK_H_ */
