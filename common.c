#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include "common.h"

/*
The following function (in a slightly modified form) was posted by
Richard Harter in comp.programming on Sun, 22 Jul 2001

Note: this will return false even if the file is a symbolic link to a valid file
*/
int does_file_exist(char* name)
{
 struct stat st; /* File status structure*/
 if ((stat(name,&st)==0) && ((st.st_mode&S_IFMT)==S_IFREG)) {
     return 1;
 }
 else 
   return 0;
}

/*
The following function was posted on
http://www.experts-exchange.com/Programming/Programming_Languages/C/Q_20790269.html
*/
long get_filesize(char *FileName)
{
	struct stat file;
	if(!stat(FileName, &file))
	{
		return file.st_size;
	}
	
	return 0;
}


int createUDPSocket(u_short port){
  struct sockaddr_in sin;
  int sock = -1;
  
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == sock)
    return -1;
  
  
  /*
    The variable sin is a standard data structure
    used to pass around network address information
    in the Unix C standard library. There are many
    flavors of sockaddr structures corresponding to
    the many types of network addressing schemes, but
    they all begin with a member that identifies the
    address family. In our case, we are dealing with 
    Internet addresses, so we use the sockaddr_in structure
    (_in) stands for Internet, and we set the address
    family to be a predefined constant AF_INET, so that
    any code that looks at our structure will know that 
    it contains Internet address information and not some
    other type.
  */
  sin.sin_family = AF_INET;  
  /*
    For an Internet-type address, the important information is
    the IP address and the port number and so the sockaddr_in 
    structure has fields for both (and basically nothing else).
  */
  sin.sin_port = htons(port); //set the port number in "network-safe" format
  sin.sin_addr.s_addr = INADDR_ANY; //basically a wildcard because we won't need
  
  /*
    Call the bind function, which tells the OS that we want our
    socket to be "bound" to the specified port. Returns zero on 
    success. If there is an error--for example, a socket is 
    already bound to this port--bind() will return -1.
  */
  if (-1 == bind(sock, (struct sockaddr *)&sin, sizeof(sin))){
    perror("\nrdt proxy: ");
    return(-1);
  }
  return sock;
}
