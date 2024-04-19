/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef __wasi__
#include "inc/wasi_socket_ext.h"
#endif


/* HTTP server to connect to */
#define HTTP_HOST "192.0.2.10" 
/* Port to connect to, as string */
#define HTTP_PORT "8000"
/* HTTP path to request */
#define HTTP_PATH "/"


#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r) { if (r == -1) { printf("Error %d: " #r "\n", errno); exit(1); } }

#define REQUEST "GET " HTTP_PATH " HTTP/1.0\r\nHost: " HTTP_HOST "\r\n\r\n"

static char response[1024];

int main(void)
{
	int st, sock;
	struct sockaddr_in addr;

	printf("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(HTTP_PORT));
	inet_pton(AF_INET, HTTP_HOST, &(addr.sin_addr));

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	CHECK(sock);
	printf("sock = %d\n", sock);

	CHECK(connect(sock, (struct sockaddr*)&addr, sizeof(addr)));

	CHECK(send(sock, REQUEST, SSTRLEN(REQUEST), 0));

	printf("Response:\n\n");

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			printf("Error reading response\n");
			return 0;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		printf("%s", response);
	}

	printf("\n");

	(void)close(sock);
	return 0;
}
