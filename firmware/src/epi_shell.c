/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "epi_config.h"
#include "epi_state.h"
#include "zbus_channels.h"

#if IS_ENABLED(CONFIG_SETTINGS)
#include "epi_settings.h"
#endif

#if IS_ENABLED(CONFIG_WIFI)
#include "net.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/zbus/zbus.h>

#if IS_ENABLED(CONFIG_WIFI)
#include <zephyr/net/net_ip.h>
#endif

static uint32_t next_req_id = 1;

static int parse_epi_list(char *list, struct verify_request *req)
{
	char *saveptr;
	char *token = strtok_r(list, ",", &saveptr);

	req->item_count = 0;

	while (token != NULL && req->item_count < EPI_MAX_ITEMS) {
		while (*token == ' ') {
			token++;
		}

		strncpy(req->items[req->item_count], token, EPI_NAME_LEN - 1);
		req->items[req->item_count][EPI_NAME_LEN - 1] = '\0';
		req->item_count++;

		token = strtok_r(NULL, ",", &saveptr);
	}

	return 0;
}

static int cmd_epi_verify(const struct shell *sh, size_t argc, char **argv)
{
	struct verify_request req;
	char list_buf[192];
	int err;

	if (argc < 3) {
		shell_error(sh, "uso: epi verify <who> <epi1,epi2,...>");
		shell_print(sh, "dica: use ASCII (Oculos, nao Óculos) no monitor serial");
		return -EINVAL;
	}

	memset(&req, 0, sizeof(req));
	req.req_id = next_req_id++;
	strncpy(req.who, argv[1], EPI_WHO_LEN - 1);

	strncpy(list_buf, argv[2], sizeof(list_buf) - 1);
	list_buf[sizeof(list_buf) - 1] = '\0';
	parse_epi_list(list_buf, &req);

	err = zbus_chan_pub(&chan_verify_request, &req, K_SECONDS(2));
	if (err < 0) {
		shell_error(sh, "falha ao publicar pedido: %d", err);
		return err;
	}

	shell_print(sh, "pedido %u enfileirado para '%s' (%u EPIs)", req.req_id, req.who,
		    req.item_count);
	return 0;
}

static int cmd_epi_api(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		const char *url = epi_config_get_api_url();

		shell_print(sh, "API URL: %s", (url[0] != '\0') ? url : "(nao configurada)");
		return 0;
	}

	epi_config_set_api_url(argv[1]);
	shell_print(sh, "API URL salva: %s", argv[1]);
	return 0;
}

#if IS_ENABLED(CONFIG_WIFI)
static const char *wifi_state_str(enum epi_wifi_state state)
{
	switch (state) {
	case EPI_WIFI_CONNECTED:
		return "conectado";
	case EPI_WIFI_CONNECTING:
		return "conectando";
	default:
		return "desconectado";
	}
}
#endif

static int cmd_epi_wifi(const struct shell *sh, size_t argc, char **argv)
{
#if !IS_ENABLED(CONFIG_SETTINGS) || !IS_ENABLED(CONFIG_WIFI)
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_error(sh, "WiFi/settings nao habilitados neste build");
	return -ENOTSUP;
#else
	int err;
	if (argc < 3) {
		char ssid[EPI_WIFI_SSID_MAX + 1];

		epi_settings_get_wifi_ssid(ssid, sizeof(ssid));
		shell_print(sh, "WiFi SSID: %s", ssid[0] ? ssid : "(nao configurado)");
		shell_print(sh, "uso: epi wifi <ssid> <psk>");
		return -EINVAL;
	}

	err = epi_settings_set_wifi(argv[1], argv[2]);
	if (err < 0) {
		shell_error(sh, "falha ao salvar WiFi: %d", err);
		return err;
	}

	shell_print(sh, "WiFi salvo para '%s' — reconectando...", argv[1]);
	err = epi_net_reconnect();
	if (err < 0) {
		shell_error(sh, "falha ao conectar: %d", err);
		return err;
	}

	shell_print(sh, "pedido de conexao enviado (aguarde DHCP)");
	return 0;
#endif
}

static int cmd_epi_net(const struct shell *sh, size_t argc, char **argv)
{
#if !IS_ENABLED(CONFIG_WIFI)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_error(sh, "WiFi nao habilitado neste build");
	return -ENOTSUP;
#else
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	err = epi_net_reconnect();
	if (err < 0) {
		shell_error(sh, "falha ao reconectar: %d", err);
		return err;
	}

	shell_print(sh, "reconectando WiFi...");
	return 0;
#endif
}

static int cmd_epi_status(const struct shell *sh, size_t argc, char **argv)
{
	const char *url = epi_config_get_api_url();

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "EPI Guard M4");

#if IS_ENABLED(CONFIG_EPI_API_USE_MOCK)
	shell_print(sh, "API: mock local");
#else
	shell_print(sh, "API: HTTP client");
	shell_print(sh, "URL: %s", epi_config_api_ready() ? url : "(nao configurada — epi api <url>)");
#endif

#if IS_ENABLED(CONFIG_WIFI)
	{
		char ip[NET_IPV4_ADDR_LEN];
		enum epi_wifi_state state = epi_net_wifi_state();

		shell_print(sh, "WiFi: %s", wifi_state_str(state));
		if (epi_net_get_ipv4_str(ip, sizeof(ip)) == 0) {
			shell_print(sh, "IP: %s", ip);
		}
	}
#endif

#if IS_ENABLED(CONFIG_SETTINGS)
	{
		char ssid[EPI_WIFI_SSID_MAX + 1];
		char dev[EPI_DEVICE_NAME_MAX];

		epi_settings_get_wifi_ssid(ssid, sizeof(ssid));
		epi_settings_get_device_name(dev, sizeof(dev));
		shell_print(sh, "SSID (NVS): %s", ssid[0] ? ssid : "-");
		shell_print(sh, "device: %s", dev);
		shell_print(sh, "timeout: %u ms", epi_settings_get_timeout_ms());
	}
#endif

	shell_print(sh, "ultimo resultado: %s",
		    epi_state_has_result() ? "disponivel (epi last)" : "nenhum");

	return 0;
}

static int cmd_epi_last(const struct shell *sh, size_t argc, char **argv)
{
	struct verify_request req;
	struct verify_result res;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!epi_state_has_result()) {
		shell_warn(sh, "nenhuma verificacao realizada ainda");
		return 0;
	}

	epi_state_get_last_request(&req);
	epi_state_get_last_result(&res);

	shell_print(sh, "req_id=%u who=%s itens=%u", req.req_id, req.who, req.item_count);
	for (uint8_t i = 0; i < req.item_count; i++) {
		shell_print(sh, "  with: %s", req.items[i]);
	}

	shell_print(sh, "allowed=%s status=%s", res.allowed ? "true" : "false",
		    res.status == VERIFY_OK ? "ok" : "error");

	if (res.unknown_person) {
		shell_print(sh, "unknown_person=true (nao cadastrado na base)");
	}

	if (res.missing_count > 0) {
		shell_print(sh, "missing:");
		for (uint8_t i = 0; i < res.missing_count; i++) {
			shell_print(sh, "  - %s", res.missing[i]);
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_epi,
			       SHELL_CMD(verify, NULL,
					 "Injeta verificacao: epi verify <who> <epi1,epi2,...>",
					 cmd_epi_verify),
			       SHELL_CMD(api, NULL, "URL da API: epi api [http://IP:3000]",
					 cmd_epi_api),
			       SHELL_CMD(wifi, NULL, "WiFi: epi wifi <ssid> <psk>",
					 cmd_epi_wifi),
			       SHELL_CMD(net, NULL, "Reconecta WiFi", cmd_epi_net),
			       SHELL_CMD(status, NULL, "Estado do sistema", cmd_epi_status),
			       SHELL_CMD(last, NULL, "Ultima verificacao", cmd_epi_last),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(epi, &sub_epi, "Comandos EPI Guard", NULL);
