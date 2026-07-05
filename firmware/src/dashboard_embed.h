/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_DASHBOARD_EMBED_H_
#define EPI_DASHBOARD_EMBED_H_

#include <stddef.h>
#include <stdint.h>

#define EPI_DASHBOARD_INDEX_HTML_MAX 4096
#define EPI_DASHBOARD_APP_JS_MAX     2048

extern const uint8_t epi_dashboard_index_html[];
extern const size_t epi_dashboard_index_html_len;

extern const uint8_t epi_dashboard_app_js[];
extern const size_t epi_dashboard_app_js_len;

#endif /* EPI_DASHBOARD_EMBED_H_ */
