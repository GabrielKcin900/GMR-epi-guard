/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("EPI Guard firmware — marco M1");
	LOG_INF("Threads: validator, actuator, net (stub)");
	LOG_INF("Teste: epi verify Gabriel Capacete,Oculos,Cinto");

	return 0;
}
