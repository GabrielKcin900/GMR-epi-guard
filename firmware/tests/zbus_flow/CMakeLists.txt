# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(epi_zbus_flow_test)

set(EPI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

target_include_directories(app PRIVATE ${EPI_SRC})
target_sources(app PRIVATE
	src/main.c
	src/channels_test.c
	${EPI_SRC}/api_client_mock.c
	${EPI_SRC}/epi_names.c
)
