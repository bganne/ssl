#ifndef COMMON_H_
#define COMMON_H_

#include <openssl/err.h>

#define PACKET_SIZE	300

#define ASSERT(cond, str)	if (!(cond)) { perror(str); abort(); }
#define ASSERT_SSL(cond)	if (!(cond)) { ERR_print_errors_fp(stderr); if (errno) perror(""); abort(); }

#endif	/* COMMON_H_ */
