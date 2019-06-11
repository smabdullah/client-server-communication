/****************************************************************************
	Code By: S.M.Abdullah
	Roll: 14
	Session: 2007-2008
	Email: abdullah_csedu06@yahoo.com
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include "client.h"

/****************************************************************************
Global section. 
****************************************************************************/
PEEROPS senops,recops;
char ip_addr[] = {"127.0.0.1"};
char ignore_ip[] = {"0.0.0.0"};
char server_ip[] = {"127.0.0.1"};
char file[255];
char packet_form[MAX_DGRAM_SIZE];
md5_byte_t hashsen[16] = {0};
md5_byte_t hashrec[16] = {0};
int sockpeersen,sockpeerrec;

int main(int argc, char *argv[])
{
	DATAGRAM *recpack, *senpack, *error;
	
	/************************************************************************
		Act as a sender peer.
    ************************************************************************/
	if(issender(argc,argv) == 1) {
		int length = 0;
		struct sockaddr_in incoming_addr;
		socklen_t addrlen = sizeof(incoming_addr);
		
		senderpeer(argc,argv,&senops);	//parsing the command line arguments or set default values.
		sockpeersen = createUDPSocket(senops.port);	//create a udp socket & bind it
		registeredsen(senpack,&senops);	//register the sender peer
		handshaking(&senops);	//send handshake message.
		
		//receive the handshake ack message forwarded by the server.
		length = recvfrom(sockpeersen,packet_form,header_size,0,(struct sockaddr *)&incoming_addr,&addrlen);
		
		if(length == -1) {
			perror("Peer can't receive the handshaking ack message: ");
			exit(1);
		}
		//check for error
		error = (DATAGRAM *)packet_form;
		checkerror(error);
		printf("Sender receive handshaking ack.\n");

		sendfile(&senops);	//sending the data file		
	}
	/************************************************************************
	Act as a receiver peer
	************************************************************************/
	else {
		int length = 0;		
		struct sockaddr_in incoming_addr;
		socklen_t addrlen = sizeof(incoming_addr);
				
		receiverpeer(argc,argv,&recops);	//prsing the command line arguments or set the default
		sockpeerrec = createUDPSocket(recops.port);	//create a udp socket & bind it
		registeredrec(recpack,&recops);	//register the receiver peer
				
		//wait for handshaking forwarded message from the server
		length = recvfrom(sockpeerrec,packet_form,header_size,0,(struct sockaddr *)&incoming_addr,&addrlen);
		if(length == -1) {
			perror("Peer can't receive the handshaking forward message: ");	
			exit(1);
		}
		//check error
		error = (DATAGRAM *)packet_form;
		checkerror(error);

		printf("Peer receives the handshaking message\n");
		
		handshakeack(&recops);	//send the handshaking ack
		recfile(&recops);	//receiving file
	}
return 0;
}

/****************************************************************************
This procedure is called by the sender peer. This is for sending 
file(possibly segmented) to the receiver peer. To recognize the end of the 
data file we send a special packet with no data(data length is zero). This 
packet gives the receiver that it is the last packet so it is safe to 
terminate now.
****************************************************************************/

int sendfile(PEEROPS *recops) {
	DATAGRAM *dfile, *recdack, *error;
	struct sockaddr_in sending_addr;	
	long length,cur_length,k,p;
	int i = 0,l;

	/************************************************************************
	Open the file to send the containt of the file. If the file does not
	exist print a message and exit.
	************************************************************************/

	FILE *fp2 = fopen(file,"rb");	
	if(fp2 == NULL) {
		printf("Can't open the file to read\n");
		exit(1);
	}	

	length = get_filesize(file);

	socklen_t	addrlen = sizeof(recdack);

	while(1) {
		
		if(length > MAX_PAYLOAD_SIZE) {
			cur_length = MAX_PAYLOAD_SIZE;
			length = length - MAX_PAYLOAD_SIZE;
		}
		else if(length <= MAX_PAYLOAD_SIZE) {
			cur_length = length;
			length = 0;
		}
		if(i == 0)
			k = 0;
			
		fseek(fp2,k,0);
		
		k+= cur_length;
		
		
		/********************************************************************
		Packet is formed to send data messgae.
		********************************************************************/

		dfile = malloc(sizeof(DATAGRAM));
		dfile->peer_ip = recops->peer_ip;
		dfile->peer_port = htons(recops->peer_port);
		dfile->seq_num = 0;
		dfile->length = ntohs(cur_length);	
		dfile->flags = 0;
		dfile->code = 0;
		bcopy(hashsen,&(dfile->hash),16);
		
		fread(dfile->message,sizeof(char),cur_length,fp2);
		
		sending_addr.sin_family = AF_INET;
		sending_addr.sin_port = htons(recops->server_port);
		inet_aton(server_ip,&sending_addr.sin_addr);
		
		/********************************************************************
		Send the data packet to the receiver.
		********************************************************************/
		
		if(-1 == sendto(sockpeersen,(void *)dfile,MAX_DGRAM_SIZE,0,(struct sockaddr *)&sending_addr,sizeof(struct sockaddr_in))) {
			perror("Data message send error: ");
			close(sockpeersen);			
			exit(1);
		}
		printf("Datagram %d is sent\n",i);
		
		
		/********************************************************************
		Receive data packet's receive ack
		********************************************************************/
		p = length;
		l = recvfrom(sockpeersen,packet_form,MAX_DGRAM_SIZE,0,(struct sockaddr *)&recdack,&addrlen);
		length = p;
		
		if(l == -1) {
			perror("Data ack error: ");
			close(sockpeersen);
			exit(1);
		}
		//check error
		error = (DATAGRAM *)packet_form;
		checkerror(error);
		
		i++;
		
		if(length == 0)
		    break;
	}
	
	/************************************************************************
	Closing section. Now all the datagrams are send. So close the socket as 
	well as close the file which was open for reading data.
	************************************************************************/

fclose(fp2);
close(sockpeersen);
}

/****************************************************************************
This procedure is called by receiver peer. This is for receiving file packet 
forwarded by the server.
****************************************************************************/

int recfile(PEEROPS *senops) {
	FILE *fp1;
	int length1;
	DATAGRAM *recfile , *sendack, *error;
	struct sockaddr_in sending_addr,incoming_addr;
	socklen_t addrlen = sizeof(incoming_addr);	
	int i = 0;

	/************************************************************************
	Open a default file 'rcvdfile' to write the data message on it. If there 
	is an error to open the file give a message and exit.
	************************************************************************/

	fp1 = fopen("rcvdfile","wb");
	if(fp1 == NULL) {
		printf("The file can't open to write\n");
		exit(1);
	}
	
	while(1) {	
		
		/********************************************************************
		Receiving the data packet from the sender.
		********************************************************************/

		length1 = recvfrom(sockpeerrec,packet_form,MAX_DGRAM_SIZE,0,(struct sockaddr *)&incoming_addr,&addrlen);
			if(length1 == -1) {
				perror("R_peer can't receive the data forward message: ");
				exit(1);
			}
			//check error
			error = (DATAGRAM *)packet_form;
			checkerror(error);			
			printf("Datagram %d is received from server\n",i);
			
			/****************************************************************
			Write the contains of the receive packet(message section) to the
			file.
			****************************************************************/

			recfile = (DATAGRAM *)packet_form;
			if(ntohs(recfile->length))			
				fwrite(recfile->message,sizeof(char),ntohs(recfile->length),fp1);
			
			/****************************************************************
			Forming packet to send data message receive ack
			****************************************************************/

			sendack = malloc(sizeof(DATAGRAM));
			sendack->peer_ip = senops->peer_ip;
			sendack->peer_port = htons(senops->peer_port);
			sendack->seq_num = 0;
			sendack->length = 0;	
			sendack->flags = FLAG_ACK;
			sendack->code = 0;
			bcopy(hashrec,&(sendack->hash),16);

			sending_addr.sin_family = AF_INET;
			sending_addr.sin_port = htons(senops->server_port);
			inet_aton(server_ip,&sending_addr.sin_addr);

			/****************************************************************
			Send data message ack
			****************************************************************/

			if(-1 == sendto(sockpeerrec,(void *)sendack,sizeof(DATAGRAM),0,(struct sockaddr *)&sending_addr,sizeof(struct sockaddr_in))) {
				perror("Data ack send error: ");
				exit(1);
		}
		printf("Ack message for datagram %d is sent\n",i);
		i++;
		
		if(ntohs(recfile->length) < MAX_PAYLOAD_SIZE)
			break;
	}
	/************************************************************************
	Close the socket and the file
	************************************************************************/

fclose(fp1);
close(sockpeerrec);
}

/****************************************************************************
When a packet is received by any peer it is first check that the forwarded 
packet contains data or error message. If the error flag is set by the server 
we give the corresponding error message depending on the error code and exit.
****************************************************************************/

int checkerror(DATAGRAM *error) {
	if(error->flags == FLAG_ERROR) {
		if(error->code == 1) {
			printf("Incorrect hash with the error code 1.\n");
			exit(1);
		}
		if(error->code == 2) {
			printf("Datagram forwarding error with error code 2.\n");
			exit(1);
		}
	}
return 0;
}

/****************************************************************************
This procedure is called by the sender peer when sending a handshake message. 
This message is forwarded by the server to the receiving peer.
****************************************************************************/

int handshaking(PEEROPS *recops) {
	DATAGRAM *handshake;
	struct sockaddr_in sending_addr;
	
	handshake = malloc(sizeof(DATAGRAM));

	/************************************************************************
	Form packet for handshake message
	************************************************************************/

	handshake->peer_ip = recops->peer_ip;
	handshake->peer_port = htons(recops->peer_port);
	handshake->seq_num = 0;
	handshake->length = 0;
	handshake->flags = FLAG_HANDSHAKE;
	handshake->code = 0;
	bcopy(hashsen,&(handshake->hash),16);
	
	sending_addr.sin_family = AF_INET;
	sending_addr.sin_port = htons(recops->server_port);
	inet_aton(server_ip,&sending_addr.sin_addr);

	/************************************************************************
	Send handshake message
	************************************************************************/

	if(-1 == sendto(sockpeersen,(void *)handshake,header_size,0,(struct sockaddr *)&sending_addr,sizeof(struct sockaddr_in))) {
		perror("Handshaking message error: ");
		exit(1);
	}
	printf("Peer send handshaking message\n");
return 0;
}

/****************************************************************************
This procedure is called by the receiving peer. To ack the sender that it 
receive the handshake message correctly.
****************************************************************************/

int handshakeack(PEEROPS *senops) {
	DATAGRAM *handack;
	struct sockaddr_in sending_addr;

	handack = malloc(sizeof(DATAGRAM));

	/************************************************************************
	Form packet for handshake ack
	************************************************************************/

	handack->peer_ip = senops->peer_ip;
	handack->peer_port = htons(senops->peer_port);
	handack->seq_num = 0;
	handack->length = 0;
	handack->flags = FLAG_HS_ACK;
	handack->code = 0;
	bcopy(hashrec,&(handack->hash),16);

	sending_addr.sin_family = AF_INET;
	sending_addr.sin_port = htons(senops->server_port);
	inet_aton(server_ip,&sending_addr.sin_addr);

	/************************************************************************
	Send handshake ack
	************************************************************************/

	if(-1 == sendto(sockpeerrec,(void *)handack,header_size,0,(struct sockaddr *)&sending_addr,sizeof(struct sockaddr_in))) {
		perror("Handshake ack message error: ");	
		exit(1);
	}
	printf("Peer send handshaking ack message\n");
return 0;
}

/****************************************************************************
To deside a peer either it is a sender or receiver. It checks if the argument 
send by the command line when creating a peer it is a sender or receiver. 
If the option contains a  '-f' in it's argument list we consider it as a 
sender either it is a receiver.
****************************************************************************/

int issender(int argc, char **argv)
{
	int i;
	for(i = 1 ; i < argc ; i++) {
		if(strcmp(argv[i],"-f"))
			return 1;
	}
return 0;
}

/****************************************************************************
This is for registering the receiver. It creates a socket and bind it to the 
receiver port on which it will listen.
****************************************************************************/

int registeredrec(DATAGRAM *recpack, PEEROPS *recops) {
	struct sockaddr_in sending_addr,rec_addr;
	int dlength;
	DATAGRAM *ack, *error;
	socklen_t	addrlen = sizeof(rec_addr);
	recpack = malloc(sizeof(DATAGRAM));

	/************************************************************************
	Form packet
	************************************************************************/

	inet_aton(ignore_ip,&recpack->peer_ip);
	recpack->peer_port = 0;
	recpack->seq_num = 0;
	recpack->length = 0;	
	recpack->flags = FLAG_REGISTER;
	recpack->code = 0;
	bcopy(hashrec,&(recpack->hash),sizeof(hashrec));

	sending_addr.sin_family = AF_INET;
	sending_addr.sin_port = htons(recops->server_port);
	inet_aton(server_ip,&sending_addr.sin_addr);

	if(-1 == sendto(sockpeerrec,(void *)recpack,header_size,0,(struct sockaddr *)&sending_addr,sizeof(struct sockaddr_in))) {
		perror("Can't send register packet: ");
		exit(1);
	}
	
	dlength = recvfrom(sockpeerrec,packet_form,header_size,0,(struct sockaddr *)&rec_addr,&addrlen);
	if(dlength == -1) {
		perror("Can't get register acknowledement: ");
		exit(1);
	}
	error = (DATAGRAM *)packet_form;
	checkerror(error);
	
	printf("Peer successfully registered\n");
	
	ack = (DATAGRAM *)packet_form;
	bcopy(&(ack->hash),hashrec,16);
return 0;
}

/****************************************************************************
This is for registering the sender. It creates a socket and bind it to the 
sender port on which it will listen.
****************************************************************************/

int registeredsen(DATAGRAM *senpack, PEEROPS *senops) {
	struct sockaddr_in sending_addr,rec_addr;
	int dlength,i;
	DATAGRAM *ack, *error;

	socklen_t	addrlen = sizeof(rec_addr);
	senpack = malloc(sizeof(DATAGRAM));
	
	/************************************************************************
	Form packet
	************************************************************************/

	inet_aton(ignore_ip,&senpack->peer_ip);
	senpack->peer_port = 0;	
	senpack->seq_num = 0;
	senpack->length = 0;
	senpack->flags = FLAG_REGISTER;
	senpack->code = 0;
	bcopy(hashsen,&(senpack->hash),sizeof(hashsen));

	sending_addr.sin_family = AF_INET;
	sending_addr.sin_port = htons(senops->server_port);
	inet_aton(server_ip,&sending_addr.sin_addr);
	
	if(-1 == sendto(sockpeersen, (void *)senpack, header_size, 0, (struct sockaddr *)&sending_addr, sizeof(struct sockaddr_in))) {
		perror("Can't send register packet: ");
		exit(1);
	}
	
	dlength = recvfrom(sockpeersen, packet_form, header_size, 0, (struct sockaddr *)&rec_addr, &addrlen);
	if(dlength == -1) {
		perror("Can't get register acknowlwdgement: ");
		exit(1);
	}
	error = (DATAGRAM *)packet_form;
	checkerror(error);
	
	printf("Peer successfully registered\n");
	
	ack = (DATAGRAM *)packet_form;
	bcopy(&(ack->hash),hashsen,16);
return 0;
}

/****************************************************************************
This procdure is for parsing the command line argument of receiver side peer. 
At first set all the value as the default value. And then change the value as 
necessary, when given by the argument
****************************************************************************/

int receiverpeer(int argc, char **argv, PEEROPS *recops) {
	int i;
	
	recops->port = DFLT_RCVR_PORT;
	inet_aton(ip_addr,&recops->server_ip);
	recops->server_port = DFLT_SERVER_PORT;
	inet_aton(ip_addr,&recops->peer_ip);
	recops->peer_port = DFLT_SENDER_PORT;
	
	if(argc >= 2) {
		for(i = 1 ; i < argc ; i++) {
			if(strcmp(argv[i],"-p") == 0) {
				if(argc <= i+1)
					goto error_section;
				recops->port = atoi(argv[++i]);
			}
			else if(strcmp(argv[i],"-s") == 0) {
				if(argc <= i+2)
					goto error_section;
				inet_aton(argv[++i],&recops->server_ip);
				strcpy(server_ip,argv[i]);
				
				recops->server_port = atoi(argv[++i]);
			}
			else if(strcmp(argv[i],"-r") == 0) {
				if(argc <= i+2)
					goto error_section;
				inet_aton(argv[++i],&recops->peer_ip);
				recops->peer_port = atoi(argv[++i]);
			}
			else {
				continue;
			}
		}
	}
return 0;

error_section:
	printf("use peer[-p port] [-s server_ip server_port] [-r peer_ip peer_port] [-f filename]\n");
	exit(1);
}

/****************************************************************************
This procdure is for parsing the command line argument of sender side peer. 
At first set all the value as the default value. And then change the value as 
necessary, when given by the argument
****************************************************************************/

int senderpeer(int argc, char **argv, PEEROPS *senops) {
	int i;
	
	senops->port = DFLT_SENDER_PORT;
	inet_aton(ip_addr,&senops->server_ip);
	senops->server_port = DFLT_SERVER_PORT;
	inet_aton(ip_addr,&senops->peer_ip);
	senops->peer_port = DFLT_RCVR_PORT;

	if(argc >= 2) {
		for(i = 1 ; i < argc ; i++) {
			if(strcmp(argv[i],"-p") == 0) {
				if(argc <= i+1)
					goto error_section;
				senops->port = atoi(argv[++i]);
			}
			else if(strcmp(argv[i],"-s") == 0) {
				if(argc <= i+2)
					goto error_section;
				inet_aton(argv[++i],&senops->server_ip);
				strcpy(server_ip,argv[i]);
				
				senops->server_port = atoi(argv[++i]);
			}
			else if(strcmp(argv[i],"-r") == 0) {
				if(argc <= i+2)
					goto error_section;
				inet_aton(argv[++i],&senops->peer_ip);
				senops->peer_port = atoi(argv[++i]);
			}
			else if(strcmp(argv[i],"-f") == 0) {
				if(argc <= i+1)
					goto error_section;
				senops->filename = argv[++i];
				strcpy(file,senops->filename);
			}
			else {
				continue;
			}
		}
	}
return 0;

error_section:
	printf("use peer[-p port] [-s server_ip server_port] [-r peer_ip peer_port] [-f filename]\n");
	exit(1);
}
