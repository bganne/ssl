#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "err.h"
#include "sock_helper.h"

int sock_helper_listen(int port)
{
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT(sock >= 0, "socket");

	int err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));
	ASSERT(0 == err, "setsockopt");

	struct sockaddr_in sa_serv;
	memset(&sa_serv, 0, sizeof(sa_serv));
	sa_serv.sin_family = AF_INET;
	sa_serv.sin_addr.s_addr = INADDR_ANY;
	sa_serv.sin_port = htons(port);
	err = bind(sock, (struct sockaddr *)&sa_serv, sizeof(sa_serv));
	ASSERT(0 == err, "bind()");

	err = listen(sock, 5);
	ASSERT(0 == err, "listen");

	return sock;
}

int sock_helper_accept(int listen_sock)
{
	struct sockaddr_in sa_cli;
	int sock = accept(listen_sock, (struct sockaddr*)&sa_cli, (socklen_t[]){sizeof(sa_cli)});
	ASSERT(sock >= 0, "accept");
	fprintf(stderr, "Connection from %x, port %x\n", sa_cli.sin_addr.s_addr,
			sa_cli.sin_port);
	return sock;
}

void sock_helper_set_nonblock(int sock)
{
	/* set non-blocking socket */
	int flags = fcntl(sock, F_GETFL);
	ASSERT(-1 != flags, "fcntl");
	flags |= O_NONBLOCK;
	int err = fcntl(sock, F_SETFL, flags);
	ASSERT(-1 != err, "fcntl");
}

int sock_helper_connect(const char *addr, int port)
{
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT(sock >= 0, "socket");

	struct sockaddr_in sa = {};
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	int err = inet_aton(addr, &sa.sin_addr);
	ASSERT(err != 0, "inet_aton()");

	err = connect(sock, (void *)&sa, sizeof(sa));
	ASSERT(0 == err, "connect()");

	return sock;
}
