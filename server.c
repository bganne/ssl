#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "err.h"
#include "sock_helper.h"
#include "ssl_helper.h"
#include "stats.h"
#include "packet.h"

#define SERVER_CERT			"cert.pem"
#define SERVER_KEY			SERVER_CERT
#define SESSION_MAX			1
#define STATS_UPDATE_FREQ	1

typedef struct {
	SSL *ssl;
	int in;
	int out;
	pthread_t thread;
	int tid;
	stats_t stats;
} __attribute__((aligned(64))) ssl_worker_t;

ssl_worker_t workers[2*SESSION_MAX];

static void * ssl_worker(void *worker_)
{
	ssl_worker_t *worker = worker_;
	SSL *ssl = worker->ssl;
	int in = worker->in;
	int out = worker->out;

	int sock = SSL_get_fd(ssl);
	ASSERT_SSL(sock >= 0);
	int nfds = sock < in ? in : sock;
	fd_set rfds;
	FD_ZERO(&rfds);

	int rem = 0;
	for (;;) {
		timestats_t ts;
		timestats_start(&ts);
		long bytes = 0;
		for (int i=0; i<STATS_UPDATE_FREQ; i++) {
			FD_SET(sock, &rfds);
			FD_SET(in, &rfds);
			int err = select(nfds+1, &rfds, NULL, NULL, NULL);
			ASSERT(-1 != err, "select()");
			if (FD_ISSET(sock, &rfds)) {
				static __thread char buf[PACKET_MAXSZ];
				err = ssl_helper_read(ssl, buf, sizeof(buf));
				if (0 == err) return 0;
				ASSERT_SSL(err > 0);
				err = write(out, buf, err);
				if (0 == err) return 0;
				ASSERT(err > 0, "write()");
				bytes += err;
			}
			if (FD_ISSET(in, &rfds)) {
				static __thread char buf[PACKET_MAXSZ];
#if 0
				(void)rem;
				err = read(in, buf, sizeof(buf));
				if (0 == err) return 0;
				ASSERT(err > 0, "read()");
				err = ssl_helper_write(ssl, buf, err);
				ASSERT_SSL(err > 0);
				bytes += err;
#else
				int len = read(in, &buf[rem], sizeof(buf)-rem);
				if (0 == len) return 0;
				ASSERT(len > 0, "read()");
				const packet_t *pkt;
				foreach_packet(pkt, (void *)buf, len) {
					err = ssl_helper_write(ssl, pkt, PACKET_TOTSZ(pkt));
					if (0 == err) return 0;
					ASSERT_SSL(err > 0);
					bytes += err;
				}
				rem = packet_remaining(pkt, buf, len);
				if (rem) memmove(buf, pkt, rem);
#endif
			}
		}
		timestats_stop(&ts);
		stats_update(&worker->stats, STATS_UPDATE_FREQ, bytes, &ts);
	}
	return NULL;
}

static void ssl_worker_spawn(int tid, SSL *ssl, int in, int out)
{
	workers[tid].ssl = ssl;
	workers[tid].in = in;
	workers[tid].out = out;
	workers[tid].tid = tid;
	int err = pthread_create(&workers[tid].thread, NULL, ssl_worker, &workers[tid]);
	ASSERT(0 == err, "pthread_create(ssl_worker) failed");
}

static SSL * ssl_accept(SSL_CTX *ctx, int listen_sock)
{
	int sock = sock_helper_accept(listen_sock);
	sock_helper_set_nonblock(sock);

	SSL *ssl = SSL_new(ctx);
	ASSERT_SSL(ssl);

	SSL_set_fd(ssl, sock);
	SSL_set_accept_state(ssl);

	return ssl;
}

static void dump_worker_stats__(const stats_t *stats, const char *fmt, ...)
{
	printf("STATS ");
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(" %llu packets (%g Gbps) - %g CPU load\n", stats->pkt, stats->bw_gbps, stats->cpu_load);
}

static void stats_add(const stats_t *a, const stats_t *b, stats_t *c)
{
	c->pkt      = a->pkt      + b->pkt;
	c->bw_gbps  = a->bw_gbps  + b->bw_gbps;
	c->cpu_load = a->cpu_load + b->cpu_load;
}

static void dump_worker_stats(int tid, stats_t *acc)
{
	const stats_t *stats = &workers[tid].stats;
	stats_add(stats, acc, acc);
	dump_worker_stats__(stats, "thr %i", tid);
}

static void dump_stats(void)
{
	stats_t stats = {};
	for (int tid=0; tid<2*SESSION_MAX; tid++) {
		dump_worker_stats(tid, &stats);
	}
	dump_worker_stats__(&stats, "TOTAL ");
}

int main(int argc, const char **argv)
{
	if (3 != argc) {
		fprintf(stderr, "Usage: %s <port> <cipher>\n", argv[0]);
		exit(-1);
	}
	int port = atoi(argv[1]);

	SSL_library_init();
	SSL_load_error_strings();

	const SSL_METHOD *meth = SSLv23_method();
	SSL_CTX *ctx = SSL_CTX_new(meth);
	ASSERT_SSL(ctx);
	int err = SSL_CTX_set_cipher_list(ctx, argv[2]);
	ASSERT_SSL(1 == err);

	err = SSL_CTX_use_certificate_file(ctx, SERVER_CERT, SSL_FILETYPE_PEM);
	ASSERT_SSL(1 == err);
	err = SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY, SSL_FILETYPE_PEM);
	ASSERT_SSL(1 == err);

	int sock = sock_helper_listen(port);

	printf("Server started on port %i with ciphers %s...\n", port, argv[2]);

	for (int i=0; i<SESSION_MAX; i++) {
		SSL *ssl1 = ssl_accept(ctx, sock);
		SSL *ssl2 = ssl_accept(ctx, sock);
		int pipe1[2], pipe2[2];
		err = pipe(pipe1);
		ASSERT(0 == err, "pipe()");
		err = pipe(pipe2);
		ASSERT(0 == err, "pipe()");
		ssl_worker_spawn(2*i+0, ssl1, pipe1[0], pipe2[1]);
		ssl_worker_spawn(2*i+1, ssl2, pipe2[0], pipe1[1]);
	}

	for (;;) {
		sleep(1);
		dump_stats();
	}

	return 0;
}
