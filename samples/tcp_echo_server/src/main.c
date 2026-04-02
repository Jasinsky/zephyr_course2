/*
 * TCP Echo Server — Zephyr Course Exercise
 *
 * A minimal TCP server that accepts one client at a time and echoes back
 * everything it receives.  Designed to run on native_sim with NSOS
 * (Native Socket Offload) so it binds to the host's loopback interface.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(tcp_echo_server, LOG_LEVEL_INF);

#define SERVER_PORT 4242
#define RECV_BUF_SIZE 256

int main(void)
{
	int serv_fd, client_fd;
	struct sockaddr_in bind_addr;
	static char rx_buf[RECV_BUF_SIZE];

	/* 1. Create a TCP socket */
	serv_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serv_fd < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		return -1;
	}
	LOG_INF("Socket created");

	/* Allow address reuse so we can restart quickly */
	int optval = 1;

	zsock_setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	/* 2. Bind to 0.0.0.0:4242 */
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(SERVER_PORT);

	if (zsock_bind(serv_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		LOG_ERR("Failed to bind: %d", errno);
		zsock_close(serv_fd);
		return -1;
	}
	LOG_INF("Bound to port %d", SERVER_PORT);

	/* 3. Listen for incoming connections */
	if (zsock_listen(serv_fd, 1) < 0) {
		LOG_ERR("Failed to listen: %d", errno);
		zsock_close(serv_fd);
		return -1;
	}
	LOG_INF("Listening… waiting for a client");

	/* 4. Main accept loop — one client at a time */
	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		client_fd = zsock_accept(serv_fd, (struct sockaddr *)&client_addr,
					 &client_addr_len);
		if (client_fd < 0) {
			LOG_ERR("accept() failed: %d", errno);
			continue;
		}
		LOG_INF("Client connected (fd %d)", client_fd);

		/* 5. Echo loop — read & send back */
		while (1) {
			ssize_t len = zsock_recv(client_fd, rx_buf, sizeof(rx_buf), 0);

			if (len <= 0) {
				if (len == 0) {
					LOG_INF("Client disconnected");
				} else {
					LOG_ERR("recv() error: %d", errno);
				}
				break;
			}

			LOG_INF("Received %zd bytes: %.*s", len, (int)len, rx_buf);

			/* Echo the data back */
			ssize_t sent = zsock_send(client_fd, rx_buf, len, 0);

			if (sent < 0) {
				LOG_ERR("send() error: %d", errno);
				break;
			}
			LOG_INF("Echoed %zd bytes", sent);
		}

		zsock_close(client_fd);
		LOG_INF("Connection closed, waiting for next client…");
	}

	zsock_close(serv_fd);
	return 0;
}
