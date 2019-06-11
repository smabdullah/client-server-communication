
#if !defined(RDT_DEFS_H)
#define RDT_DEFS_H

#include<netinet/in.h>

#define TRUE 1
#define FALSE 0

#define MAX_DGRAM_SIZE 1024
#define MAX_PAYLOAD_SIZE (MAX_DGRAM_SIZE - sizeof(RDTHEADER))

#define DFLT_SENDER_PORT 7777
#define DFLT_RCVR_PORT 8888
#define DFLT_SERVER_PORT 9999


#define FLAG_ACK  0x01
#define FLAG_REGISTER 0x02
#define FLAG_HANDSHAKE 0x04
#define FLAG_ERROR 0x08
#define FLAG_REG_ACK (FLAG_REGISTER | FLAG_ACK)
#define FLAG_HS_ACK (FLAG_HANDSHAKE | FLAG_ACK)

typedef struct s_hash {
  unsigned char h[16]; //an MD5 hash 
} SHASH;

typedef struct RDTHeader {
  struct in_addr	peer_ip;
  unsigned short int	peer_port;
  unsigned short int	seq_num;
  unsigned short int    length;
  unsigned char 	flags;
  unsigned char         code;
  SHASH		        hash;
} RDTHEADER;

int does_file_exist(char *);
long get_filesize(char *FileName);
int createUDPSocket(unsigned short port);

#endif // !defined( RDT_DEFS_H )
