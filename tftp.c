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

#include <errno.h>
#include <arpa/inet.h>

#include "tftp.h"

extern int h_errno;
extern int errno;

#define TFTP_TYPE_GET 0
#define TFTP_TYPE_PUT 1

/* Should cover most needs */
#define MSGBUF_SIZE (TFTP_DATA_HDR_LEN + BLOCK_SIZE)

#ifdef DEBUG
int dbgflag = 1;
#else
int dbgflag = 0;
#endif

size_t maxfnamelen = 12;
#define MAXERRLEN 30

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
        struct addrinfo hints;
        struct addrinfo *servinfo; 
	int sockfd;
        int status;
	char servportstr[4];

	if (!fname || !mode || !hostname)
		return NULL;

	tc = malloc(sizeof(struct tftp_conn));
	
	if (!tc)
		return NULL;

	/* Create a socket. 
	 * Check return value. */
       
        /*  TAREK - getaddrinfo() first, then create socket() */
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET;       // force IPv4 address
	hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;     // Not sure about this, what if we have multi IFs?
	sprintf(servportstr,"%d", TFTP_PORT);
	if ((status = getaddrinfo(hostname, servportstr, &hints, &servinfo)) != 0) {
    		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    		return NULL;
	} 

	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sockfd == -1) {
		fprintf(stderr,"Error creating socket: %d\n", errno);
		return NULL;
	}
        tc->sock = sockfd;

	if (dbgflag) { 
		struct sockaddr_in *localip =  (struct sockaddr_in *)servinfo->ai_addr;
		char ipstr[20];
		void *addr = NULL;
		addr = &(localip->sin_addr);
		inet_ntop(servinfo->ai_family, addr, ipstr, sizeof ipstr);
		
		printf("Created socket to %s:%s from %s:%d\n", 
				hostname, servportstr, ipstr, localip->sin_port);
	}

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
	/* TAREK - Already did this above before socket()
         * using getaddr() but using gethostbyname() to fill
         * hent as needed below..
         */
        hent = gethostbyname(hostname);
	if (!hent) {
		fprintf(stderr, "Error gethostbyname(), code=%d\n", h_errno);	
                close(tc->sock);
                free(tc);
                return NULL;
	}

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
/* Send opcode 1 (2 bytes) + Filename (string) + "0" (1 byte) + Mode (string) + "0" (1 byte) */
int tftp_send_rrq(struct tftp_conn *tc)
{
	struct tftp_rrq *rrq;
	int ret =0; 
	int bytecount = 0;
        u_int8_t packet_len = TFTP_RRQ_LEN(tc->fname, tc->mode);

        void *rrqmsg = calloc(1, packet_len);
        if (!rrqmsg) return 1;

        rrq = (struct tftp_rrq *) rrqmsg;

   	rrq->opcode = htons((u_int16_t) OPCODE_RRQ);
   	memcpy(rrq->req, tc->fname, strlen(tc->fname) * sizeof(u_int8_t));
   	bytecount+=strlen(tc->fname)+1; // skip to mode position
   	memcpy(rrq->req+bytecount, tc->mode, strlen(tc->mode)* sizeof(u_int8_t));
   	bytecount+=strlen(tc->mode)+1;

{
int x;
printf("sending: opcode=%d\n", ntohs(rrq->opcode));
for (x=0; x < bytecount; x++) 
printf("%x ", rrq->req[x]);
printf("\n");
}

	ret=sendto(tc->sock, rrq, packet_len, 0, (struct sockaddr*)&tc->peer_addr, sizeof(tc->peer_addr));
	if (ret == -1) {
		fprintf(stderr, "sento() failed while sending RRQ with %d\n", errno);
		if (rrqmsg) free(rrqmsg);
		return 1;
	}	
	
 	printf("send_rrq(): sendto() sent %d bytes.\n", ret);
 
	if (rrqmsg) free(rrqmsg);

   	return 0;
}
/*
  
  Send a write request to the server.
  1. Format message.
  2. Send the request using the connection handle.
  3. Return the number of bytes sent, or negative on error.
 */
/* Send opcode 2 (2 bytes) + Filename (string) + "0" (1 byte) + Mode (string) + "0" (1 byte) */
int tftp_send_wrq(struct tftp_conn *tc)
{
	struct tftp_wrq *wrq;
        int ret =0;
        int bytecount = 0;
	u_int8_t packet_len = TFTP_WRQ_LEN(tc->fname, tc->mode);

        void *wrqmsg = calloc(1, packet_len);
	if (!wrqmsg) return 1;

	wrq = (struct tftp_wrq *) wrqmsg;
   	wrq->opcode = htons((u_int16_t) OPCODE_WRQ);

	memcpy(wrq->req, tc->fname, strlen(tc->fname) * sizeof(u_int8_t));
	bytecount+=strlen(tc->fname)+1; // skip to mode position
	memcpy(wrq->req+bytecount, tc->mode, strlen(tc->mode)* sizeof(u_int8_t));
	bytecount+=strlen(tc->mode)+1;

{
int x;
printf("sending: opcode=%x\n", ntohs(wrq->opcode));
for (x=0; x < bytecount;  x++)
printf("%x ", wrq->req[x]);
printf("\n");
}
        ret=sendto(tc->sock, wrq, packet_len, 0, (struct sockaddr*)&tc->peer_addr, sizeof(tc->peer_addr));
        if (ret == -1) {
                fprintf(stderr, "sento() failed while sending WRQ with %d\n", errno);
                if (wrqmsg) free(wrqmsg);
                return 1;
        }

       printf("send_wrq(): sendto() sent %d bytes.\n", ret);

	if (wrqmsg) free(wrqmsg);

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
	struct tftp_ack *ack; 
	u_int16_t sentbytes=0;
   /* Send opcode 3 (2 bytes) + Block # (2 bytes) */

  	ack = (struct tftp_ack *) calloc(1, TFTP_ACK_HDR_LEN);

   ack->opcode = htons((u_int16_t) OPCODE_ACK);
   ack->blocknr = htons((u_int16_t) tc->blocknr);

{
printf("sending: \n");
printf("%x %x", (u_int16_t) ack->opcode, ack->blocknr);
printf("\n");
}
	sentbytes=sendto(tc->sock, ack, TFTP_ACK_HDR_LEN, 0, (struct sockaddr*)&tc->peer_addr, sizeof(tc->peer_addr));
	if (sentbytes == -1) {
		fprintf(stderr, "sento() failed while sending ACK with %d\n", errno);
		free(ack);
		return 1;
	}	
	
 	printf("send_ack(): sent ack for block number %d using %d bytes.\n", ntohs(ack->blocknr), sentbytes);
 
 	free(ack);	
        
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
/* Send opcode 3 (2 bytes) + Block # (2 bytes) + Data (512 bytes, except last chunk) */
int tftp_send_data(struct tftp_conn *tc, int length)
{	
	struct tftp_data *tdata;
	u_int8_t sentbytes = 0;
	u_int16_t packet_len = TFTP_DATA_HDR_LEN + (length * sizeof(u_int8_t));
	void *data = calloc (1, packet_len);
	if (!data) return 1;

	tdata = (struct tftp_data *) data;

	tdata->opcode = htons((u_int16_t) OPCODE_DATA);

	if (length < 0) {
	/* We either timed out or got a dup ack, seek back and resend last block */

	} else {
	/* Send incremental blocks until we're out of data  */
		tdata->blocknr = htons(tc->blocknr);
		fread(tdata->data, length, 1, tc->fp);
	}
{
int x;
printf("sending: opcode=%d, block=%d\n", ntohs(tdata->opcode), ntohs(tdata->blocknr));
for (x=0; x < length;  x++)
printf("%x ", tdata->data[x]);
printf("\n");
}

        sentbytes=sendto(tc->sock, tdata, packet_len, 0, (struct sockaddr*)&tc->peer_addr, sizeof(tc->peer_addr));
        if (sentbytes == -1) {
                fprintf(stderr, "sento() failed while sending DATA with %d\n", errno);
                if (data) free(data);
                return -1;
        }

        printf("send_data(): sent data for block number %d using %d bytes.\n", ntohs(tdata->blocknr), sentbytes);

        if (data) free(data);     

        return (sentbytes-TFTP_DATA_HDR_LEN);
}


int tftp_recv_data(struct tftp_conn *tc, int length)
{	
	char data[BLOCK_SIZE];
	/* Receive opcode 3 (2 bytes) + Block # (2 bytes) + Data (512 bytes, except last chunk) */
	
	memcpy(&tc->blocknr, tc->msgbuf + sizeof(u_int16_t), sizeof(u_int16_t));
	tc->blocknr = ntohs(tc->blocknr);
	printf("At block number %d\n", tc->blocknr);
	
	memset (&data, 0, sizeof(char) * BLOCK_SIZE);
	memcpy(&data, tc->msgbuf + (sizeof(u_int16_t)*2), (length - 4) * sizeof(u_int8_t));
	
	fwrite(data, sizeof(u_int8_t), length - 4, tc->fp);  
	
	if (tftp_send_ack(tc)) {
		fprintf(stderr, "recv_data(): tftp_send_ack() failed.\n"); 
		return 1;
	}		

	if (length-4 < 512) { /* subtract opcode and block num - last block */
		return 0; /* Close file & return */
	}

	tc->blocknr++;
	
	printf("Sent ack(), now waiting for block %d\n", tc->blocknr);
	
	return 0;
}

int tftp_handle_error(char *msgbuf, int len)
{	
	int errnum = 0;
	char errmsg[MAXERRLEN];			

	memcpy((void *)&errnum, msgbuf+3, sizeof(u_int8_t)); /* Get byte 4*/
	memcpy((void *)&errmsg, msgbuf+4, sizeof(u_int8_t) *(len - 4)); /* Get byte 4*/
	fprintf(stderr, "TFTP error message : %d, %s\n", errnum, errmsg);
			 	
        return 0;
}

/*
  Transfer a file to or from the server.

 */
int tftp_transfer(struct tftp_conn *tc)
{
	int retval = 0;
	int bytesrecvd =0; 
	int bytesleft = 0;
	int totlen = 0;
	struct timeval timeout;
	fd_set fds;
	socklen_t fromaddrlen=0;
	u_int16_t opcode = 0;
	u_int16_t blocknr = 0;

        /* Sanity check */
	if (!tc)
		return -1;

	/* After the connection request we should start receiving data
	 * immediately */
	
        /* Set a timeout for resending data. */

	timeout.tv_sec = TFTP_TIMEOUT;
	timeout.tv_usec = 0;

        /* Check if we are putting a file or getting a file and send
         * the corresponding request. */

	/* TAREK - check if this is a GET or PUT */
	switch (tc->type) {
		case TFTP_TYPE_GET:
			/* GET means we send a Read Request */
			tftp_send_rrq(tc);
			break;
		case TFTP_TYPE_PUT: 
			/* PUT means we send a Write Request */
			tftp_send_wrq(tc);
			tc->blocknr = 0;
			/* Get file size */
			fseek(tc->fp, 0L, SEEK_END);
			bytesleft = ftell(tc->fp);
			fseek(tc->fp, 0L, SEEK_SET);
			printf("File %s is %d bytes.\n", tc->fname, bytesleft);
			break;
		default: /* Should never happen */
			goto out; /* FIXME - print error? */
			break;
	}

        /*
          Put or get the file, block by block, in a loop.
         */
	do {
		FD_ZERO(&fds);
		FD_SET(tc->sock, &fds);
	
		/* 1. Wait for something from the server (using
                 * 'select'). If a timeout occurs, resend last block
                 * or ack depending on whether we are in put or get
                 * mode. */
		retval = select(tc->sock+1, &fds , NULL,NULL, &timeout);
		if (retval == -1) {
			fprintf(stderr, "Error in select: %d\n", retval);	
			goto out;
		} else if (retval == 0) {
			/* TODO - timeout resend or ack depending on mode*/
			fprintf(stderr, "Timeout\n");
			goto out; 
		} else {
			/* Read msg data into tc->msg, parse OPCODE & data */
			if (FD_ISSET(tc->sock, &fds)) {
				bytesrecvd = recvfrom(tc->sock, tc->msgbuf, MSGBUF_SIZE,0, (struct sockaddr *)&tc->peer_addr, &fromaddrlen);
				if ( (bytesrecvd == 0) || (bytesrecvd == -1) ) {
					fprintf(stderr, "Error receiving data from recvfrom %d\n", errno);
					goto out;
				}
			}
			
			printf("Recvfrom() got %d bytes.\n", bytesrecvd);
		}
		memcpy((void *)&opcode, tc->msgbuf+1, sizeof(u_int8_t) *1); /* Get opcode */
{
int x;
for (x=0; x < bytesrecvd; x++) 
printf("%x ", tc->msgbuf[x]);
printf("\n");
}
printf("transfer() got opcode =%d\n",opcode);
		
                /* 2. Check the message type and take the necessary
                 * action. */
		switch ( opcode ) {
		case OPCODE_DATA:
                        /* Received data block, send ack */
			tftp_recv_data(tc, bytesrecvd);
			break;
		case OPCODE_ACK:
                        /* Received ACK, send next block */
			memcpy((void *)&blocknr, tc->msgbuf+2, sizeof(u_int8_t)*2); /* Get blocknr */
			printf("got ACK for blocknumber=%d\n", blocknr);
			if (blocknr == tc->blocknr) {
				tc->blocknr++; blocknr++;
				printf("Current block ACK'd, increment block number.\n");
			}
			if ((retval = tftp_send_data(tc, bytesleft)) < 0) {
				printf("Error sending data, resend..\n");
			}
			else {
				printf("Sent %d bytes, %d left\n", retval, bytesleft-retval);
				bytesleft-=retval;
			}
			break;
		case OPCODE_ERR:
                        /* Handle error... */
			tftp_handle_error(tc->msgbuf,bytesrecvd);
			return -1;
         		break;
		default:
			fprintf(stderr, "\nUnknown message type\n");
			goto out;

		}

	} while ( 1 /* 3. Loop until file is finished */);

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

	/* TODO - sanity check for fname, hostname */

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
