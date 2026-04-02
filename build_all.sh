#!/usr/bin/env bash
set -e

west build -b native_sim samples/tcp_echo_server -d build_echo_server
west build -b native_sim samples/tcp_echo_client -d build_echo_client
