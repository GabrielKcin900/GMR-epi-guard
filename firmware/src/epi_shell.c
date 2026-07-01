/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "epi_state.h"
#include "zbus_channels.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>
#include <zephyr/zbus/zbus.h>

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

static int cmd_epi_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "EPI Guard M1 — WiFi/API ainda nao configurados");
	shell_print(sh, "API: mock local (api_client)");
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
			       SHELL_CMD(status, NULL, "Estado do sistema", cmd_epi_status),
			       SHELL_CMD(last, NULL, "Ultima verificacao", cmd_epi_last),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(epi, &sub_epi, "Comandos EPI Guard", NULL);
