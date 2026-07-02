/*
 * SPDX-License-Identifier: Apache-2.0
 * Cliente HTTP — POST /check na API Express (M3).
 */

#include "api_client_internal.h"
#include "epi_config.h"
#include "json_codec.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>

LOG_MODULE_DECLARE(api_client);

#if IS_ENABLED(CONFIG_SETTINGS)
#include "epi_settings.h"
#endif

#define HTTP_RECV_BUF_LEN 512
#define HTTP_BODY_BUF_LEN 384
#define HTTP_REQ_TIMEOUT_MS_DEFAULT 10000
#define EPI_API_DEFAULT_PORT 3000

struct http_collect_ctx {
	char body[HTTP_BODY_BUF_LEN];
	size_t body_len;
};

static int parse_api_url(const char *url, char *host, size_t host_len, uint16_t *port)
{
	const char *p = url;
	const char *colon;
	const char *slash;
	size_t hlen;

	if (url == NULL || host == NULL || port == NULL) {
		return -EINVAL;
	}

	if (strncmp(p, "http://", 7) == 0) {
		p += 7;
	}

	colon = strchr(p, ':');
	slash = strchr(p, '/');

	if (colon != NULL && (slash == NULL || colon < slash)) {
		hlen = (size_t)(colon - p);
		if (hlen == 0 || hlen >= host_len) {
			return -EINVAL;
		}
		memcpy(host, p, hlen);
		host[hlen] = '\0';
		*port = (uint16_t)strtoul(colon + 1, NULL, 10);
		if (*port == 0) {
			return -EINVAL;
		}
		return 0;
	}

	hlen = slash != NULL ? (size_t)(slash - p) : strlen(p);
	if (hlen == 0 || hlen >= host_len) {
		return -EINVAL;
	}

	memcpy(host, p, hlen);
	host[hlen] = '\0';
	*port = EPI_API_DEFAULT_PORT;

	return 0;
}

static int http_response_collect(struct http_response *rsp, enum http_final_call final_data,
				 void *user_data)
{
	struct http_collect_ctx *ctx = user_data;
	size_t room;

	if (rsp == NULL || ctx == NULL) {
		return -EINVAL;
	}

	if (rsp->data_len > 0 && rsp->body_frag_start != NULL) {
		room = sizeof(ctx->body) - ctx->body_len - 1;
		if (room > 0) {
			size_t copy = MIN((size_t)rsp->data_len, room);

			memcpy(ctx->body + ctx->body_len, rsp->body_frag_start, copy);
			ctx->body_len += copy;
			ctx->body[ctx->body_len] = '\0';
		}
	}

	if (final_data == HTTP_DATA_FINAL) {
		return 0;
	}

	return 0;
}

static int tcp_connect_ipv4(const char *host, uint16_t port)
{
	struct sockaddr_in addr;
	int sock;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	ret = inet_pton(AF_INET, host, &addr.sin_addr);
	if (ret != 1) {
		LOG_ERR("host invalido (use IP IPv4): %s", host);
		return -EINVAL;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		return -errno;
	}

	ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		close(sock);
		return -errno;
	}

	return sock;
}

int api_check_http(const struct verify_request *req, struct verify_result *out)
{
	const char *url = epi_config_get_api_url();
	char host[64];
	char port_str[8];
	uint16_t port;
	char payload[256];
	uint8_t recv_buf[HTTP_RECV_BUF_LEN];
	struct http_collect_ctx collect;
	struct http_request http_req;
	const char *headers[] = {
		"Content-Type: application/json\r\n",
		NULL,
	};
	int sock;
	int ret;

	if (req == NULL || out == NULL) {
		return -EINVAL;
	}

	if (!epi_config_api_ready()) {
		LOG_ERR("URL da API nao configurada (epi api <url>)");
		return -EINVAL;
	}

	ret = parse_api_url(url, host, sizeof(host), &port);
	if (ret < 0) {
		LOG_ERR("URL invalida: %s", url);
		return ret;
	}

	snprintk(port_str, sizeof(port_str), "%u", port);

	ret = json_encode_check_request(req, payload, sizeof(payload));
	if (ret < 0) {
		LOG_ERR("falha ao serializar JSON: %d", ret);
		return ret;
	}

	sock = tcp_connect_ipv4(host, port);
	if (sock < 0) {
		LOG_ERR("conexao TCP falhou: %d", sock);
		return sock;
	}

	memset(&collect, 0, sizeof(collect));
	memset(&http_req, 0, sizeof(http_req));

	http_req.method = HTTP_POST;
	http_req.url = "/check";
	http_req.host = host;
	http_req.port = port_str;
	http_req.protocol = "HTTP/1.1";
	http_req.header_fields = headers;
	http_req.payload = payload;
	http_req.payload_len = strlen(payload);
	http_req.response = http_response_collect;
	http_req.recv_buf = recv_buf;
	http_req.recv_buf_len = sizeof(recv_buf);

	ret = http_client_req(sock, &http_req,
#if IS_ENABLED(CONFIG_SETTINGS)
			      (int32_t)epi_settings_get_timeout_ms(),
#else
			      HTTP_REQ_TIMEOUT_MS_DEFAULT,
#endif
			      &collect);
	close(sock);

	if (ret < 0) {
		LOG_ERR("HTTP POST falhou: %d", ret);
		return ret;
	}

	if (collect.body_len == 0) {
		LOG_ERR("resposta HTTP vazia");
		return -EIO;
	}

	memset(out, 0, sizeof(*out));
	ret = json_parse_check_response(collect.body, collect.body_len, out);
	if (ret < 0) {
		LOG_ERR("JSON invalido na resposta: %d", ret);
		return ret;
	}

	LOG_INF("HTTP check %s: allowed=%d unknown=%d missing=%u", req->who, out->allowed,
		out->unknown_person, out->missing_count);

	return 0;
}
