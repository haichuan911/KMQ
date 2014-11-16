#ifndef _CRC_H_
#define _CRC_H_

#include <inttypes.h>
#include "decr.h"

KMQ_DECLARATION_START

uint64_t crc64(unsigned char *buf, uint32_t len);
uint16_t crc16(const char *buf, uint32_t len);

}

#endif
