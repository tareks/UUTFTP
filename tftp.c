/* A tftp client implementation.
   Author: Erik Nordström <erikn@it.uu.se>
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include "tftp.h"

extern int h_errno;

#define TFTP_TYPE_GET 0
#define TFTP_TYPE_PUT 1

/* Should cover most needs */
#define MSGBUF_SIZE (TFTP_DATA_HDR_LEN + BLOCK_SIZE)


/* 
 * NOTE:
 * In tftp.h you will find definitions for headers and constants. Make
 * sure these are used throughout your code.
 */


/* A connection handle */
struct tftp_conn {
	int type; /* Are we putting or getting? */
	FILE *fp; /* The file we are reading or writing */
	int sock; /* Socket to communicate with server */
	int blocknr; /* The current block number */
	char *fname; /* The file name of the file we are putting or getting */
	char *mode; /* TFTP mode */
	struct sockaddr_in peer_addr; /* Remote peer address */
	socklen_t addrlen; /* The remote address length */
	char msgbuf[MSGBUF_SIZE]; /* Buffer for messages being sent or received */
};

/* Close the connection handle, i.e., delete our local state. */
void tftp_close(struct tftp_conn *tc) 
{
	if (!tc)
		return;

	fclose(tc->fp);
	close(tc->sock);
	free(tc);
}

/* Connect to a remote TFTP server. */
struct tftp_conn *tftp_connect(int type, char *fname, char *mode, 
			       const char *hostname)
{
	struct hostent *hent;
	struct tftp_conn *tc;
	
	if (!fname || !mode || !hostname)
		return NULL;

	tc = malloc(sizeof(struct tftp_conn));
	
	if (!tc)
		return NULL;

	/* Create a socket. 
	 * Check return value. */

	/* ... */

	if (type == TFTP_TYPE_PUT)
		tc->fp = fopen(fname, "rb");
	else if (type == TFTP_TYPE_GET)
		tc->fp = fopen(fname, "wb");
	else {
		fprintf(stderr, "Invalid TFTP mode, must be put or get\n");
		return NULL;		
	}
		 
	if (tc->fp == NULL) {
		fprintf(stderr, "File I/O error!\n");
		close(tc->sock);
		free(tc);
		return NULL;
	}
		
	/* get address from host name.
	 * If error, gracefully clean up.*/

	/* ... */

	/* Assign address to the connection handle.
	 * You can assume that the first address in the hostent
	 * struct is the correct one */

	tc->peer_addr.sin_family = PF_INET;
	tc->peer_addr.sin_port = htons(TFTP_PORT);
	
	memcpy(&tc->peer_addr.sin_addr, 
	       hent->h_addr_list[0], sizeof(struct in_addr));
	
	tc->addrlen = sizeof(struct sockaddr_in);

	tc->type = type;
	tc->mode = mode;
	tc->fname = fname;
	tc->blocknr = 0;

	memset(tc->msgbuf, 0, MSGBUF_SIZE);
	
	return tc;
}

/*
  Send a read request to the server.
  1. Format message.
  2. Send the request using the connection handle.
  3. Return the number of bytes sent, or negative on error.
 */
int tftp_send_rrq(struct tftp_conn *tc)
{
	/* struct tftp_rrq *rrq; */

        /* ... */

        return 0;
}
/*
  
  Send a write request to the server.
  1. Format message.
  2. Send the request using the connection handle.
  3. Return the number of bytes sent, or negative on error.
 */
int tftp_send_wrq(struct tftp_conn *tc)
{
	/* struct tftp_wrq *wrq; */

        /* ... */

        return 0;
}


/*
  Acknowledge reception of a block.
  1. Format message.
  2. Send the acknowledgement using the connection handle.
  3. Return the number of bytes sent, or negative on error.
 */
int tftp_send_ack(struct tftp_conn *tc)
{
	/* struct tftp_ack *ack; */
        
        return 0;
}

/*
  Send a data block to the other side.
  1. Format message.
  2. Add data block to message according to length argument.
  3. Send the data block message using the connection handle.
  4. Return the number of bytes sent, or negative on error.

  TIP: You need to be able to resend data in case of a timeout. When
  resending, the old message should be sent again and therefore no
  new message should be created. This can, for example, be handled by
  passing a negative length indicating that the creation of a new
  message should be skipped.
 */
int tftp_send_data(struct tftp_conn *tc, int length)
{	
	/* struct tftp_data *tdata; */
        return 0;
}

/*
  Transfer a file to or from the server.

 */
int tftp_transfer(struct tftp_conn *tc)
{
	int retval = 0;
	int len;
	int totlen = 0;
	struct timeval timeout;

        /* Sanity check */
	if (!tc)
		return -1;

        len = 0;

	/* After the connection request we should start receiving data
	 * immediately */
	
        /* Set a timeout for resending data. */

        timeout.tv_sec = TFTP_TIMEOUT;
        timeout.tv_usec = 0;

        /* Check if we are putting a file or getting a file and send
         * the corresponding request. */

        /* ... */

        /*
          Put or get the file, block by block, in a loop.
         */
	do {
		/* 1. Wait for something from the server (using
                 * 'select'). If a timeout occurs, resend last block
                 * or ack depending on whether we are in put or get
                 * mode. */

                /* ... */

                /* 2. Check the message type and take the necessary
                 * action. */
		switch ( 0 /* change for msg type */ ) {
		case OPCODE_DATA:
                        /* Received data block, send ack */
			break;
		case OPCODE_ACK:
                        /* Received ACK, send next block */
			break;
		case OPCODE_ERR:
                        /* Handle error... */
                        break;
		default:
			fprintf(stderr, "\nUnknown message type\n");
			goto out;

		}

	} while ( 0 /* 3. Loop until file is finished */);

	printf("\nTotal data bytes sent/received: %d.\n", totlen);
out:	
	return retval;
}

int main (int argc, char **argv) 
{
	
	char *fname = NULL;
	char *hostname = NULL;
	char *progname = argv[0];
	int retval = -1;
	int type = -1;
	struct tftp_conn *tc;

        /* Check whether the user wants to put or get a file. */
	while (argc > 0) {
		
		if (strcmp("-g", argv[0]) == 0) {
			fname = argv[1];
			hostname = argv[2];

			type = TFTP_TYPE_GET;
			break;		
		} else if (strcmp("-p", argv[0]) == 0) {
			fname = argv[1];
			hostname = argv[2];
			
			type = TFTP_TYPE_PUT;
			break;
		}
		argc--;
		argv++;	
	}
	
        /* Print usage message */
	if (!fname || !hostname) {
		fprintf(stderr, "Usage: %s [-g|-p] FILE HOST\n",
			progname);
		return -1;
	}

        /* Connect to the remote server */
	tc = tftp_connect(type, fname, MODE_OCTET, hostname);

	if (!tc) {
		fprintf(stderr, "Failed to connect!\n");
		return -1;
	}

        /* Transfer the file to or from the server */	
	retval = tftp_transfer(tc);
	
        if (retval < 0) {
                fprintf(stderr, "File transfer failed!\n");
        }

        /* We are done. Cleanup our state. */
	tftp_close(tc);
	
	return retval;
}
