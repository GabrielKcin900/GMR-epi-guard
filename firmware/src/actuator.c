/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "epi_state.h"
#include "zbus_channels.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(actuator, LOG_LEVEL_INF);

extern struct zbus_observer actuator_sub;

#define LED0_NODE   DT_ALIAS(led0)
#define BUZZER_NODE DT_ALIAS(buzzer)
#define SW0_NODE    DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#define EPI_HAS_LED 1
#else
#define EPI_HAS_LED 0
#endif

#if DT_NODE_HAS_STATUS(BUZZER_NODE, okay)
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER_NODE, gpios);
#define EPI_HAS_BUZZER 1
#else
#define EPI_HAS_BUZZER 0
#endif

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
#define EPI_HAS_BUTTON 1
#else
#define EPI_HAS_BUTTON 0
#endif

static bool led_ready;
static bool buzzer_ready;
static bool button_ready;

struct feedback_ctx {
	struct k_work work;
	struct verify_result result;
};

static struct feedback_ctx feedback;

static int actuator_gpio_init(void)
{
#if EPI_HAS_LED
	if (gpio_is_ready_dt(&led) && gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) == 0) {
		led_ready = true;
	} else {
		LOG_WRN("led0 indisponivel");
	}
#endif

#if EPI_HAS_BUZZER
	if (gpio_is_ready_dt(&buzzer) &&
	    gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_INACTIVE) == 0) {
		buzzer_ready = true;
		LOG_INF("buzzer em GPIO%d", buzzer.pin);
	} else {
		LOG_WRN("buzzer indisponivel");
	}
#endif

#if EPI_HAS_BUTTON
	if (gpio_is_ready_dt(&button) &&
	    gpio_pin_configure_dt(&button, GPIO_INPUT) == 0) {
		button_ready = true;
		LOG_INF("botao BOOT em GPIO%d", button.pin);
	} else {
		LOG_WRN("botao sw0 indisponivel");
	}
#endif

	return 0;
}

static void led_on(void)
{
#if EPI_HAS_LED
	if (led_ready) {
		gpio_pin_set_dt(&led, 1);
	}
#endif
}

static void led_off(void)
{
#if EPI_HAS_LED
	if (led_ready) {
		gpio_pin_set_dt(&led, 0);
	}
#endif
}

static void buzzer_on(void)
{
#if EPI_HAS_BUZZER
	if (buzzer_ready) {
		gpio_pin_set_dt(&buzzer, 1);
	}
#endif
}

static void buzzer_off(void)
{
#if EPI_HAS_BUZZER
	if (buzzer_ready) {
		gpio_pin_set_dt(&buzzer, 0);
	}
#endif
}

static void beep_ms(uint32_t ms)
{
	buzzer_on();
	k_msleep(ms);
	buzzer_off();
}

static bool button_pressed(void)
{
#if EPI_HAS_BUTTON
	int val;

	if (!button_ready) {
		return false;
	}

	val = gpio_pin_get_dt(&button);
	if (val < 0) {
		return false;
	}

	/* gpio_pin_get_dt: 1 = ativo (pressionado com GPIO_ACTIVE_LOW no DT) */
	return val != 0;
#else
	return false;
#endif
}

static void log_missing(const struct verify_result *res)
{
	if (res->missing_count == 0) {
		return;
	}

	LOG_INF("Faltando:");
	for (uint8_t i = 0; i < res->missing_count; i++) {
		LOG_INF("  - %s", res->missing[i]);
	}
}

static void run_allowed(void)
{
	LOG_INF("LIBERADO — bip curto, LED fixo ~3 s");
	beep_ms(120);
	led_on();
	k_msleep(3000);
	led_off();
}

static void run_denied(const struct verify_result *res)
{
	LOG_INF("NEGADO — bip longo, LED piscando ~3 s");
	log_missing(res);

	beep_ms(900);

	for (int i = 0; i < 6; i++) {
		led_on();
		k_msleep(250);
		led_off();
		k_msleep(250);
	}
}

static void run_unknown(void)
{
	LOG_INF("NAO CADASTRADO — LED azul piscando, buzzer continuo (pressione BOOT para parar)");

	while (!button_pressed()) {
		buzzer_on();
		k_msleep(120);
		buzzer_off();
		k_msleep(80);

		led_on();
		k_msleep(200);
		if (button_pressed()) {
			break;
		}
		led_off();
		k_msleep(200);
	}

	buzzer_off();
	led_off();
	LOG_INF("alarma encerrada (botao pressionado)");
}

static void run_error(void)
{
	LOG_INF("ERRO — 3 bips curtos");

	for (int i = 0; i < 3; i++) {
		beep_ms(120);
		k_msleep(120);
	}
}

static void feedback_handler(struct k_work *work)
{
	struct feedback_ctx *ctx = CONTAINER_OF(work, struct feedback_ctx, work);
	const struct verify_result *res = &ctx->result;

	if (res->status != VERIFY_OK) {
		run_error();
		return;
	}

	if (res->allowed) {
		run_allowed();
	} else {
		run_denied(res);
	}
}

static void schedule_feedback(const struct verify_result *res)
{
	feedback.result = *res;
	k_work_submit(&feedback.work);
}

static void handle_result(const struct verify_result *res)
{
	epi_state_store_result(res);

	if (res->status != VERIFY_OK) {
		schedule_feedback(res);
		return;
	}

	if (res->unknown_person) {
		LOG_INF("result req_id=%u NAO CADASTRADO", res->req_id);
		run_unknown();
		return;
	}

	if (res->allowed) {
		LOG_INF("result req_id=%u LIBERADO", res->req_id);
	} else {
		LOG_INF("result req_id=%u NEGADO (missing=%u)", res->req_id, res->missing_count);
	}

	schedule_feedback(res);
}

static void actuator_thread(void)
{
	const struct zbus_channel *chan;

	actuator_gpio_init();
	k_work_init(&feedback.work, feedback_handler);
	LOG_INF("actuator thread started");

	while (!zbus_sub_wait(&actuator_sub, &chan, K_FOREVER)) {
		struct verify_result res;

		if (chan != &chan_verify_result) {
			continue;
		}

		if (zbus_chan_read(&chan_verify_result, &res, K_MSEC(500)) < 0) {
			continue;
		}

		handle_result(&res);
	}
}

K_THREAD_DEFINE(actuator_thread_id, 2048, actuator_thread, NULL, NULL, NULL, 7, 0, 0);
