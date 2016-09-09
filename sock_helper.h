#ifndef SOCK_HELPER_H_
#define SOCK_HELPER_H_

int sock_helper_listen(int port);
int sock_helper_accept(int listen_sock);
void sock_helper_set_nonblock(int sock);
int sock_helper_connect(const char *addr, int port);

#endif	/* SOCK_HELPER_H_ */
