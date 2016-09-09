#ifndef TUN_H_
#define TUN_H_

/* dev must be big enough to hold IFNAMSIZ */
int tun_alloc(const char *fmt, char *name);

#endif	/* TUN_H_ */
