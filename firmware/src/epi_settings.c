/*
 * SPDX-License-Identifier: Apache-2.0
 * Settings NVS — chaves do plano §5.3.
 */

#include "epi_settings.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(epi_settings, LOG_LEVEL_INF);

#define KEY_WIFI_SSID    "wifi/ssid"
#define KEY_WIFI_PSK     "wifi/psk"
#define KEY_API_URL      "api/base_url"
#define KEY_DEVICE_NAME  "device/name"
#define KEY_TIMEOUT_MS   "policy/timeout_ms"

static char wifi_ssid[EPI_WIFI_SSID_MAX + 1];
static char wifi_psk[EPI_WIFI_PSK_MAX + 1];
static char api_url[EPI_API_URL_MAX];
static char device_name[EPI_DEVICE_NAME_MAX];
static uint32_t timeout_ms = 3000;
static struct k_mutex settings_lock;

struct load_ctx {
	void *dest;
	size_t len;
	bool found;
};

static int load_string_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg,
			  void *param)
{
	struct load_ctx *ctx = param;
	size_t copy_len;
	int rc;

	ARG_UNUSED(name);

	if (ctx->len == 0) {
		return -EINVAL;
	}

	copy_len = MIN(len, ctx->len - 1);
	rc = read_cb(cb_arg, ctx->dest, copy_len);
	if (rc > 0) {
		((char *)ctx->dest)[copy_len] = '\0';
		ctx->found = true;
	}

	return rc >= 0 ? 0 : rc;
}

static int load_u32_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg,
		       void *param)
{
	struct load_ctx *ctx = param;
	uint32_t val;
	int rc;

	ARG_UNUSED(name);

	if (len != sizeof(val)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &val, sizeof(val));
	if (rc == sizeof(val)) {
		*(uint32_t *)ctx->dest = val;
		ctx->found = true;
	}

	return rc == sizeof(val) ? 0 : -EINVAL;
}

static int load_string_key(const char *key, char *dest, size_t dest_len)
{
	struct load_ctx ctx = {
		.dest = dest,
		.len = dest_len,
		.found = false,
	};
	int rc;

	dest[0] = '\0';

	rc = settings_load_subtree_direct(key, load_string_cb, &ctx);
	if (rc < 0) {
		return rc;
	}

	return ctx.found ? 0 : -ENOENT;
}

static int load_u32_key(const char *key, uint32_t *dest)
{
	struct load_ctx ctx = {
		.dest = dest,
		.len = sizeof(*dest),
		.found = false,
	};
	int rc;

	rc = settings_load_subtree_direct(key, load_u32_cb, &ctx);
	if (rc < 0) {
		return rc;
	}

	return ctx.found ? 0 : -ENOENT;
}

int epi_settings_init(void)
{
	k_mutex_init(&settings_lock);
	return settings_subsys_init();
}

int epi_settings_load(void)
{
	uint32_t loaded_timeout;
	int rc;

	k_mutex_lock(&settings_lock, K_FOREVER);

	wifi_ssid[0] = '\0';
	wifi_psk[0] = '\0';
	api_url[0] = '\0';
	strncpy(device_name, "epi-guard-01", sizeof(device_name) - 1);
	device_name[sizeof(device_name) - 1] = '\0';
	timeout_ms = 3000;

	rc = load_string_key(KEY_WIFI_SSID, wifi_ssid, sizeof(wifi_ssid));
	if (rc < 0 && rc != -ENOENT) {
		LOG_WRN("load %s failed: %d", KEY_WIFI_SSID, rc);
	}

	rc = load_string_key(KEY_WIFI_PSK, wifi_psk, sizeof(wifi_psk));
	if (rc < 0 && rc != -ENOENT) {
		LOG_WRN("load %s failed: %d", KEY_WIFI_PSK, rc);
	}

	rc = load_string_key(KEY_API_URL, api_url, sizeof(api_url));
	if (rc < 0 && rc != -ENOENT) {
		LOG_WRN("load %s failed: %d", KEY_API_URL, rc);
	}

	rc = load_string_key(KEY_DEVICE_NAME, device_name, sizeof(device_name));
	if (rc < 0 && rc != -ENOENT) {
		LOG_WRN("load %s failed: %d", KEY_DEVICE_NAME, rc);
	}

	rc = load_u32_key(KEY_TIMEOUT_MS, &loaded_timeout);
	if (rc == 0) {
		timeout_ms = loaded_timeout;
	}

	k_mutex_unlock(&settings_lock);

	LOG_INF("settings carregados (ssid=%s, api=%s)", wifi_ssid[0] ? wifi_ssid : "-",
		api_url[0] ? api_url : "-");

	return 0;
}

bool epi_settings_has_wifi(void)
{
	bool has;

	k_mutex_lock(&settings_lock, K_FOREVER);
	has = (wifi_ssid[0] != '\0');
	k_mutex_unlock(&settings_lock);

	return has;
}

int epi_settings_get_wifi_ssid(char *buf, size_t len)
{
	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(buf, wifi_ssid, len - 1);
	buf[len - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	return 0;
}

int epi_settings_get_wifi_psk(char *buf, size_t len)
{
	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(buf, wifi_psk, len - 1);
	buf[len - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	return 0;
}

int epi_settings_set_wifi(const char *ssid, const char *psk)
{
	int rc;

	if (ssid == NULL || psk == NULL || ssid[0] == '\0') {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(wifi_ssid, ssid, sizeof(wifi_ssid) - 1);
	wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
	strncpy(wifi_psk, psk, sizeof(wifi_psk) - 1);
	wifi_psk[sizeof(wifi_psk) - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	rc = settings_save_one(KEY_WIFI_SSID, wifi_ssid, strlen(wifi_ssid) + 1);
	if (rc < 0) {
		return rc;
	}

	rc = settings_save_one(KEY_WIFI_PSK, wifi_psk, strlen(wifi_psk) + 1);
	if (rc < 0) {
		return rc;
	}

	LOG_INF("WiFi salvo: ssid=%s", wifi_ssid);
	return 0;
}

int epi_settings_get_api_url(char *buf, size_t len)
{
	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(buf, api_url, len - 1);
	buf[len - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	return 0;
}

int epi_settings_set_api_url(const char *url)
{
	int rc;

	if (url == NULL || url[0] == '\0') {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(api_url, url, sizeof(api_url) - 1);
	api_url[sizeof(api_url) - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	rc = settings_save_one(KEY_API_URL, api_url, strlen(api_url) + 1);
	if (rc == 0) {
		LOG_INF("API URL salva: %s", api_url);
	}

	return rc;
}

int epi_settings_get_device_name(char *buf, size_t len)
{
	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(buf, device_name, len - 1);
	buf[len - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	return 0;
}

int epi_settings_set_device_name(const char *name)
{
	int rc;

	if (name == NULL || name[0] == '\0') {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	strncpy(device_name, name, sizeof(device_name) - 1);
	device_name[sizeof(device_name) - 1] = '\0';
	k_mutex_unlock(&settings_lock);

	rc = settings_save_one(KEY_DEVICE_NAME, device_name, strlen(device_name) + 1);
	return rc;
}

uint32_t epi_settings_get_timeout_ms(void)
{
	uint32_t val;

	k_mutex_lock(&settings_lock, K_FOREVER);
	val = timeout_ms;
	k_mutex_unlock(&settings_lock);

	return val;
}

int epi_settings_set_timeout_ms(uint32_t ms)
{
	int rc;

	k_mutex_lock(&settings_lock, K_FOREVER);
	timeout_ms = ms;
	k_mutex_unlock(&settings_lock);

	rc = settings_save_one(KEY_TIMEOUT_MS, &timeout_ms, sizeof(timeout_ms));
	return rc;
}
