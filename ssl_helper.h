#ifndef SSL_HELPER_H_
#define SSL_HELPER_H_

#include <openssl/ssl.h>

int ssl_helper_read(SSL *ssl, void *buf, int num);
int ssl_helper_write(SSL *ssl, const void *buf, int num);

#endif	/* SSL_HELPER_H_ */
