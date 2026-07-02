# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(DTC_OVERLAY_FILE boards/doit_esp32_26mhz.overlay)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(epi_guard)

target_sources(app PRIVATE
	src/main.c
	src/zbus_channels.c
	src/api_client.c
	src/validator.c
	src/actuator.c
	src/net.c
	src/epi_state.c
	src/epi_names.c
	src/epi_shell.c
	src/epi_config.c
)

if(CONFIG_SETTINGS)
	target_sources(app PRIVATE src/epi_settings.c)
endif()

if(CONFIG_EPI_API_USE_MOCK)
	target_sources(app PRIVATE src/api_client_mock.c)
else()
	target_sources(app PRIVATE src/api_client_http.c src/json_codec.c)
endif()
