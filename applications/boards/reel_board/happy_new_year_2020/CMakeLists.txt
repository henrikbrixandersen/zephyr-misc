# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include($ENV{ZEPHYR_BASE}/cmake/cfb.cmake NO_POLICY_SCOPE)
project(happy_new_year_2020)

target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_cfb_font_for_target(app fonts/zephyr-logo.png ${gen_dir}/cfb_font_zephyr_logo.h 250 120 --first 90 --last 90)
