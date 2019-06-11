#if !defined(CLIENT_H)
#define CLIENT_H

#include "common.h"
#include "md5.h"
#define header_size 28

/*
define a structure to hold options
passed in from the command line
*/
typedef struct peer_ops {
  unsigned short  port;
	struct in_addr	server_ip;
	unsigned short  server_port;
  struct in_addr	peer_ip;
	unsigned short  peer_port;
  char *filename;
} PEEROPS;

typedef struct packet {
  struct in_addr	peer_ip;
  unsigned short int	peer_port;
  unsigned short int	seq_num;
  unsigned short int    length;
  unsigned char 	flags;
  unsigned char         code;
  SHASH		        hash;
  char message[MAX_PAYLOAD_SIZE];
} DATAGRAM;

int issender(int argc, char **argv);
int senderpeer(int argc, char **argv , PEEROPS *senops);
int receiverpeer(int argc, char **argv, PEEROPS *recops);
int registeredrec(DATAGRAM *recpack, PEEROPS *recops);
int registeredsen(DATAGRAM *senpack, PEEROPS *recops);
int handshaking(PEEROPS *recops);
int waitforsenreg(void);
int handshakeack(PEEROPS *senops);
int checkerror(DATAGRAM *error);
int recfile(PEEROPS *senops);

#endif // !defined( CLIENT_H )
