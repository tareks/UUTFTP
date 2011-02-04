#ifndef _TFTP_H
#define _TFTP_H

#if defined(SUNOS_5)
#define OS_SUNOS5
typedef short u_int16_t;
typedef int u_int32_t;
typedef char u_int8_t;
#elif defined(__GNUC__) && (defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__))
#define OS_MACOSX
#elif defined(__GNUC__) && defined(__linux__)
#define OS_LINUX
#include <linux/types.h>
#else
#error "Unsupported operating system"
#endif 

#define BLOCK_SIZE 512

#define OPCODE_RRQ   1
#define OPCODE_WRQ   2
#define OPCODE_DATA  3
#define OPCODE_ACK   4
#define OPCODE_ERR   5


#define MODE_NETASCII "netascii"
#define MODE_OCTET    "octet"
#define MODE_MAIL     "mail"

#define TFTP_PORT 69

/* Timeout in seconds */
#define TFTP_TIMEOUT 2

static char *err_codes[8] = {
	"Undef",
	"File not found",
	"Access violation",
	"Disk full or allocation exceeded",
	"Illegal TFTP operation",
	"Unknown transfer ID",
	"File already exists",
	"No such user"
};

/*
  A generic header for TFTP messages.
 */
struct tftp_msg {
	u_int16_t opcode;
	char msg[0];
};

/*
  A TFTP read request.
 */
struct tftp_rrq {
	u_int16_t opcode;
	char req[0];
};

#define TFTP_RRQ_HDR_LEN sizeof(struct tftp_rrq)
#define TFTP_RRQ_LEN(f,m) (sizeof(struct tftp_rrq) + strlen(f) + strlen(m) + 2)

/*
  A TFTP write request.
 */
struct tftp_wrq {
	u_int16_t opcode;
	char req[0];
};

#define TFTP_WRQ_HDR_LEN sizeof(struct tftp_wrq)
#define TFTP_WRQ_LEN(f,m) (sizeof(struct tftp_wrq) + strlen(f) + strlen(m) + 2)

/*
  A TFTP data block message.
 */
struct tftp_data {
	u_int16_t opcode;
	u_int16_t blocknr;
	char data[0];
};

#define TFTP_DATA_HDR_LEN sizeof(struct tftp_data)

/*
  A TFTP ack message.
 */
struct tftp_ack {
	u_int16_t opcode;
	u_int16_t blocknr;
};

#define TFTP_ACK_HDR_LEN sizeof(struct tftp_ack)

/*
  A TFTP error message.
 */
struct tftp_err {
	u_int16_t opcode;
	u_int16_t errcode;
	char errmsg[0];
};

#define TFTP_ERR_HDR_LEN sizeof(struct tftp_err)

static inline char *tftp_err_to_str(int err)
{
	if (err < 0 || err > 7)
		return NULL;
	
	return err_codes[err];
}

#endif /* TFTP_H_ */
