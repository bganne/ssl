#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tun.h"
#include "sock_helper.h"
#include "ssl_helper.h"
#include "err.h"

#define DEV	"stun%d"

int main(int argc, const char **argv)
{
	if (4 != argc) {
		fprintf(stderr, "Usage: %s <host> <port> <cipher>\n", argv[0]);
		exit(-1);
	}
	const char *addr = argv[1];
	int port = atoi(argv[2]);
	const char *cipher = argv[3];

	int tunfd = tun_alloc(DEV, (char [IFNAMSIZ]){});
	ASSERT(tunfd >= 0, "tun_alloc()");

	SSL_library_init();
	SSL_load_error_strings();

	const SSL_METHOD *meth = SSLv23_method();
	SSL_CTX *ctx = SSL_CTX_new(meth);
	ASSERT_SSL(ctx);
	int err = SSL_CTX_set_cipher_list(ctx, cipher);
	ASSERT_SSL(1 == err);

	int ssl_sock = sock_helper_connect(addr, port);
	SSL *ssl = SSL_new(ctx);
	ASSERT_SSL(ssl);
	SSL_set_fd(ssl, ssl_sock);
	err = SSL_connect(ssl);
	ASSERT_SSL(1 == err);

	fd_set rfds;
	FD_ZERO(&rfds);
	int nfds = ssl_sock < tunfd ? tunfd : ssl_sock;

	for (;;) {
		static __thread char buf[65536];
		FD_SET(tunfd, &rfds);
		FD_SET(ssl_sock, &rfds);
		err = select(nfds+1, &rfds, NULL, NULL, NULL);
		ASSERT(-1 != err, "select()");
		if (FD_ISSET(tunfd, &rfds)) {
			err = read(tunfd, buf, sizeof(buf));
			if (0 == err) break;
			ASSERT(err > 0, "read()");
			err = ssl_helper_write(ssl, buf, err);
			if (0 == err) break;
			ASSERT_SSL(err > 0);
		}
		if (FD_ISSET(ssl_sock, &rfds)) {
			err = ssl_helper_read(ssl, buf, sizeof(buf));
			if (0 == err) break;
			ASSERT_SSL(err > 0);
			err = write(tunfd, buf, err);
			if (0 == err) break;
			ASSERT_SSL(err > 0);
		}
	}

	return 0;
}
