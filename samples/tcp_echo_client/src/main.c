/*
 * TCP Echo Client — Zephyr Course Exercise
 *
 * Connects to the TCP Echo Server at 127.0.0.1:4242, sends numbered
 * messages, and prints what the server echoes back.
 * Designed to run on native_sim with NSOS alongside the echo server.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <stdio.h>
LOG_MODULE_REGISTER(tcp_echo_client, LOG_LEVEL_INF);

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 4242
#define SEND_INTERVAL_MS 2000
#define MSG_BUF_SIZE 128

int main(void)
{
	int sock;
	struct sockaddr_in dest;
	static char tx_buf[MSG_BUF_SIZE];
	static char rx_buf[MSG_BUF_SIZE];
	int msg_count = 0;

	/* 1. Create a TCP socket */
	sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		return -1;
	}
	LOG_INF("Socket created");

	/* 2. Connect to the echo server */
	dest.sin_family = AF_INET;
	dest.sin_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET, SERVER_ADDR, &dest.sin_addr);

	LOG_INF("Connecting to %s:%d …", SERVER_ADDR, SERVER_PORT);

	if (zsock_connect(sock, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
		LOG_ERR("connect() failed: %d", errno);
		zsock_close(sock);
		return -1;
	}
	LOG_INF("Connected!");

	/* 3. Send / receive loop */
	while (1) {
		int len = snprintf(tx_buf, sizeof(tx_buf),
				   "Hello from Zephyr #%d", msg_count++);

		ssize_t sent = zsock_send(sock, tx_buf, len, 0);

		if (sent < 0) {
			LOG_ERR("send() failed: %d", errno);
			break;
		}
		LOG_INF("Sent: %s", tx_buf);

		/* Wait for echo */
		ssize_t received = zsock_recv(sock, rx_buf, sizeof(rx_buf) - 1, 0);

		if (received <= 0) {
			LOG_ERR("recv() returned %zd (errno %d)", received, errno);
			break;
		}
		rx_buf[received] = '\0';
		LOG_INF("Echo: %s", rx_buf);

		k_msleep(SEND_INTERVAL_MS);
	}

	zsock_close(sock);
	LOG_INF("Disconnected");
	return 0;
}
