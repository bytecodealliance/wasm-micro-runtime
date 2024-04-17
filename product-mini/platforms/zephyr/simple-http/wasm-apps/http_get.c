/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#else
#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>
#endif

// #ifdef __wasi__
// #include <wasi_socket_ext.h>
// #endif


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

void dump_addrinfo(const struct addrinfo *ai)
{
	printf("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
	       "sa_family=%d, sin_port=%x\n",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol,
	       ai->ai_addr->sa_family,
	       ((struct sockaddr_in *)ai->ai_addr)->sin_port);
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock;

	printf("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "\n");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
	printf("getaddrinfo status: %d\n", st);

	if (st != 0) {
		printf("Unable to resolve address, quitting\n");
		return 0;
	}

	dump_addrinfo(res);

	//sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	CHECK(sock);
	printf("sock = %d\n", sock);
	
	CHECK(connect(sock, res->ai_addr, res->ai_addrlen));
	// int rc = 0;
	// for(int i = 0; i < 10; i++){
	// 	rc = connect(sock, res->ai_addr, res->ai_addrlen);
	// 	if (rc == 0) {
	// 		break;
	// 	}
	// 	else{
	// 		printf("[Debug] Connect try %d: status %d\n", i+1, errno);
	// 		close(sock);
	// 		k_sleep(K_MSEC(100)); // 10 mil seconds 
	// 		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	// 		if (sock < 0) {
	// 			printf("[Error] Unable to create socket, exiting...");
	// 			exit(1);
	// 		}
	// 	}
	// 	k_sleep(K_MSEC(1000 * 5)); // 5 seconds 
	// }
	// if(rc){
	// 	printf("[Error] Unable to Connect exiting...");
	// 	exit(1);
	// }

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
