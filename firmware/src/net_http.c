/*
 * SPDX-License-Identifier: Apache-2.0
 * Servidor HTTP M5 — sockets TCP (sem http_server Zephyr no ESP32).
 */

#include "net_http.h"

#include "dashboard_embed.h"
#include "epi_state.h"
#include "json_codec.h"
#include "net.h"
#include "zbus_channels.h"

#if IS_ENABLED(CONFIG_SETTINGS)
#include "epi_settings.h"
#endif

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_DECLARE(net);

#define EPI_HTTP_PORT        80
#define HTTP_REQ_MAX         896
#define HTTP_SEND_CHUNK      256
#define EPI_VERIFY_BODY_MAX  384
#define EPI_VERIFY_RESP_MAX  384
#define HTTP_SRV_STACK       5120

static struct k_sem verify_done;
static struct k_mutex verify_lock;
static uint32_t waiting_req_id;
static struct verify_result waiting_result;
static bool verify_inflight;
static bool http_init_done;
static volatile bool http_srv_listening;

static uint8_t index_html_ram[EPI_DASHBOARD_INDEX_HTML_MAX];
static uint8_t app_js_ram[EPI_DASHBOARD_APP_JS_MAX];
static size_t index_html_len;
static size_t app_js_len;

static char verify_resp_buf[EPI_VERIFY_RESP_MAX];

static int tcp_send_all(int fd, const void *buf, size_t len)
{
	const uint8_t *p = buf;

	while (len > 0) {
		ssize_t n = send(fd, p, len, 0);

		if (n < 0) {
			return -errno;
		}
		if (n == 0) {
			return -EIO;
		}

		p += n;
		len -= (size_t)n;
	}

	return 0;
}

static int tcp_send_body(int fd, const uint8_t *body, size_t len)
{
	uint8_t chunk[HTTP_SEND_CHUNK];

	while (len > 0) {
		size_t n = MIN(len, sizeof(chunk));

		memcpy(chunk, body, n);

		int err = tcp_send_all(fd, chunk, n);

		if (err < 0) {
			return err;
		}

		body += n;
		len -= n;
	}

	return 0;
}

static int http_reply(int fd, int code, const char *status, const char *ctype,
		      const uint8_t *body, size_t body_len)
{
	char hdr[192];
	int hlen = snprintk(hdr, sizeof(hdr),
			    "HTTP/1.1 %d %s\r\n"
			    "Content-Type: %s\r\n"
			    "Connection: close\r\n"
			    "Access-Control-Allow-Origin: *\r\n"
			    "Content-Length: %u\r\n\r\n",
			    code, status, ctype, (unsigned int)body_len);
	int err;

	if (hlen < 0 || (size_t)hlen >= sizeof(hdr)) {
		return -ENOSPC;
	}

	err = tcp_send_all(fd, hdr, (size_t)hlen);
	if (err < 0) {
		return err;
	}

	if (body_len == 0) {
		return 0;
	}

	return tcp_send_body(fd, body, body_len);
}

static char *http_find_body(char *req)
{
	char *sep = strstr(req, "\r\n\r\n");

	return sep != NULL ? sep + 4 : NULL;
}

static int http_content_length(const char *req)
{
	const char *p = req;
	const char *key;
	char *end;
	long val;

	while (*p != '\0') {
		if (strncmp(p, "Content-Length:", 15) == 0) {
			key = p + 15;
		} else if (strncmp(p, "content-length:", 15) == 0) {
			key = p + 15;
		} else {
			p = strchr(p, '\n');
			if (p == NULL) {
				break;
			}
			p++;
			continue;
		}

		while (*key == ' ') {
			key++;
		}
		val = strtol(key, &end, 10);
		if (end == key || val < 0 || val > HTTP_REQ_MAX) {
			return -1;
		}
		return (int)val;
	}

	return -1;
}

static ssize_t http_read_request(int fd, char *buf, size_t buflen)
{
	ssize_t total = 0;
	int content_len = -1;
	char *body;

	while (total < (ssize_t)buflen - 1) {
		ssize_t n = recv(fd, buf + total, buflen - 1 - (size_t)total, 0);

		if (n < 0) {
			return -errno;
		}
		if (n == 0) {
			break;
		}

		total += n;
		buf[total] = '\0';

		body = http_find_body(buf);
		if (body == NULL) {
			continue;
		}

		if (content_len < 0) {
			content_len = http_content_length(buf);
		}

		if (content_len < 0) {
			break;
		}

		if ((size_t)(buf + total - body) >= (size_t)content_len) {
			break;
		}
	}

	return total;
}

static int run_verify_http(const char *json_body, size_t json_len, char *resp, size_t resp_len)
{
	struct verify_request req;
	struct verify_result err_res;
	uint32_t timeout_ms;
	int err;

	if (json_len >= EPI_VERIFY_BODY_MAX) {
		return -EMSGSIZE;
	}

	memset(&err_res, 0, sizeof(err_res));
	err_res.status = VERIFY_ERROR;

	if (k_mutex_lock(&verify_lock, K_MSEC(200)) != 0) {
		return -EBUSY;
	}

	if (verify_inflight) {
		k_mutex_unlock(&verify_lock);
		return -EBUSY;
	}

	char body_copy[EPI_VERIFY_BODY_MAX];

	memcpy(body_copy, json_body, json_len);
	body_copy[json_len] = '\0';

	err = json_parse_verify_request(body_copy, json_len, &req);
	if (err < 0) {
		k_mutex_unlock(&verify_lock);
		return err;
	}

	req.req_id = epi_state_alloc_req_id();
	waiting_req_id = req.req_id;
	verify_inflight = true;
	memset(&waiting_result, 0, sizeof(waiting_result));

	while (k_sem_take(&verify_done, K_NO_WAIT) == 0) {
		/* drena semaforo residual */
	}

	err = zbus_chan_pub(&chan_verify_request, &req, K_SECONDS(2));
	if (err < 0) {
		verify_inflight = false;
		k_mutex_unlock(&verify_lock);
		return err;
	}

#if IS_ENABLED(CONFIG_SETTINGS)
	timeout_ms = epi_settings_get_timeout_ms();
#else
	timeout_ms = 3000;
#endif

	if (k_sem_take(&verify_done, K_MSEC(timeout_ms)) != 0) {
		verify_inflight = false;
		k_mutex_unlock(&verify_lock);
		return -ETIMEDOUT;
	}

	verify_inflight = false;
	k_mutex_unlock(&verify_lock);

	err = json_encode_verify_response(&waiting_result, resp, resp_len);
	return err;
}

static void http_handle_client(int fd)
{
	char req[HTTP_REQ_MAX];
	ssize_t rlen;
	char *body;
	int err;

	rlen = http_read_request(fd, req, sizeof(req));
	if (rlen <= 0) {
		return;
	}

	if (strncmp(req, "GET /ping", 9) == 0) {
		static const char pong[] = "ok";

		(void)http_reply(fd, 200, "OK", "text/plain", (const uint8_t *)pong,
				 sizeof(pong) - 1);
		return;
	}

	if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET / HTTP", 10) == 0) {
		(void)http_reply(fd, 200, "OK", "text/html; charset=utf-8", index_html_ram,
				 index_html_len);
		return;
	}

	if (strncmp(req, "GET /app.js", 11) == 0) {
		(void)http_reply(fd, 200, "OK", "text/javascript; charset=utf-8", app_js_ram,
				 app_js_len);
		return;
	}

	if (strncmp(req, "POST /verify", 12) == 0) {
		static const char err_json[] =
			"{\"status\":\"error\",\"allowed\":false,\"missing\":[]}";

		body = http_find_body(req);
		if (body == NULL) {
			(void)http_reply(fd, 400, "Bad Request", "application/json",
					 (const uint8_t *)err_json, sizeof(err_json) - 1);
			return;
		}

		size_t body_len = (size_t)((req + rlen) - body);

		err = run_verify_http(body, body_len, verify_resp_buf, sizeof(verify_resp_buf));
		if (err == -EBUSY) {
			(void)http_reply(fd, 503, "Service Unavailable", "application/json",
					 (const uint8_t *)err_json, sizeof(err_json) - 1);
			return;
		}
		if (err < 0) {
			(void)http_reply(fd, err == -ETIMEDOUT ? 504 : 400,
					 err == -ETIMEDOUT ? "Gateway Timeout" : "Bad Request",
					 "application/json", (const uint8_t *)err_json,
					 sizeof(err_json) - 1);
			return;
		}

		(void)http_reply(fd, 200, "OK", "application/json",
				 (const uint8_t *)verify_resp_buf, strlen(verify_resp_buf));
		return;
	}

	static const char nf[] = "Not Found";

	(void)http_reply(fd, 404, "Not Found", "text/plain", (const uint8_t *)nf, sizeof(nf) - 1);
}

static void http_srv_thread(void *p1, void *p2, void *p3)
{
	int srv = -1;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	epi_http_init();
	LOG_INF("HTTP thread (socket) iniciada");

	for (;;) {
		char ip[NET_IPV4_ADDR_LEN];

		if (epi_net_get_ipv4_str(ip, sizeof(ip)) != 0 || ip[0] == '\0') {
			if (srv >= 0) {
				close(srv);
				srv = -1;
				http_srv_listening = false;
			}
			k_msleep(1000);
			continue;
		}

		if (srv < 0) {
			struct sockaddr_in addr;
			int opt = 1;
			int err;

			srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (srv < 0) {
				LOG_ERR("socket(): %d", errno);
				k_msleep(2000);
				continue;
			}

			(void)setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(EPI_HTTP_PORT);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			err = bind(srv, (struct sockaddr *)&addr, sizeof(addr));
			if (err < 0) {
				LOG_ERR("bind(:%d): %d", EPI_HTTP_PORT, errno);
				close(srv);
				srv = -1;
				k_msleep(2000);
				continue;
			}

			err = listen(srv, 2);
			if (err < 0) {
				LOG_ERR("listen(): %d", errno);
				close(srv);
				srv = -1;
				k_msleep(2000);
				continue;
			}

			http_srv_listening = true;
			LOG_INF("HTTP escutando http://%s:%d/ (GET /ping para teste)", ip,
				EPI_HTTP_PORT);
		}

		{
			struct sockaddr_in peer;
			socklen_t plen = sizeof(peer);
			int cli = accept(srv, (struct sockaddr *)&peer, &plen);

			if (cli < 0) {
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					LOG_WRN("accept(): %d", errno);
				}
				k_msleep(50);
				continue;
			}

			http_handle_client(cli);
			close(cli);
		}
	}
}

K_THREAD_DEFINE(http_srv_tid, HTTP_SRV_STACK, http_srv_thread, NULL, NULL, NULL, 5, 0, 0);

void epi_http_init(void)
{
	if (http_init_done) {
		return;
	}

	k_sem_init(&verify_done, 0, 1);
	k_mutex_init(&verify_lock);

	if (epi_dashboard_index_html_len > sizeof(index_html_ram) ||
	    epi_dashboard_app_js_len > sizeof(app_js_ram)) {
		LOG_ERR("dashboard embed maior que buffer RAM");
		return;
	}

	memcpy(index_html_ram, epi_dashboard_index_html, epi_dashboard_index_html_len);
	memcpy(app_js_ram, epi_dashboard_app_js, epi_dashboard_app_js_len);
	index_html_len = epi_dashboard_index_html_len;
	app_js_len = epi_dashboard_app_js_len;
	http_init_done = true;
}

bool epi_http_is_running(void)
{
	return http_srv_listening;
}

int epi_http_server_try_start(void)
{
	epi_http_init();
	return http_srv_listening ? 0 : -EAGAIN;
}

void epi_http_on_verify_result(const struct verify_result *res)
{
	if (res == NULL || !verify_inflight || res->req_id != waiting_req_id) {
		return;
	}

	waiting_result = *res;
	k_sem_give(&verify_done);
}
