#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

typedef struct {
	uint64_t len;
	uint8_t data[];
} __attribute__((packed)) packet_t;

#define PACKET_TOTSZ(pkt)	((pkt)->len + sizeof(uint64_t))
#define PACKET_MAXSZ		(65536+sizeof(uint64_t))

#define foreach_packet(iter, base, len) \
	for ((iter)=(base); \
			(uintptr_t)(iter)->data < (uintptr_t)(base) + (len) \
			&& (uintptr_t)(iter)->data + (iter)->len <= (uintptr_t)(base) + (len); \
			(iter) = (void *)((uintptr_t)(iter)->data + (iter)->len))

#define packet_remaining(iter, base, len)	((uintptr_t)(base) + (len) - (uintptr_t)(iter))

#endif	/* PACKET_H_ */
