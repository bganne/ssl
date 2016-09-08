#include <stdio.h>
#include <sys/select.h>
#include <openssl/ssl.h>
#include "common.h"
#include "ssl_helper.h"

static void handle_err(int err_, SSL *ssl)
{
	fd_set fds, *rfds = NULL, *wfds = NULL;
	int err = SSL_get_error(ssl, err_);
	if (SSL_ERROR_WANT_READ == err) {
		fprintf(stderr, "got want read\n");
		rfds = &fds;
	} else if (SSL_ERROR_WANT_WRITE == err) {
		wfds = &fds;
	} else {
		ASSERT_SSL(0);
	}
	int sock = SSL_get_fd(ssl);
	ASSERT_SSL(sock >= 0);
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	err = select(sock+1, rfds, wfds, NULL, NULL);
	ASSERT(-1 != err, "select");
}

int ssl_helper_read(SSL *ssl, void *buf, int num)
{
	for (;;) {
		int err = SSL_read(ssl, buf, num);
		if (err >= 0) return err;
		handle_err(err, ssl);
	}
}

int ssl_helper_write(SSL *ssl, const void *buf, int num)
{
	for (;;) {
		int err = SSL_write(ssl, buf, num);
		if (err >= 0) return err;
		handle_err(err, ssl);
	}
}
