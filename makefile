#Makefile adapted from "Introduction to Make" by Jennifer Vesperman
#  http://www.linuxdevcenter.com/pub/a/linux/2002/01/31/make_intro.html

#PLATFORM NOTES:
#This makefile now includes some conditional evaluation rules that are
#supported by GNU's version of make but not by some other standard
#versions.
#To build on CSE Machines (LINUX):
#	make all platform=LINUX 
#To build on SUN Workstations
#	/soft/sparc/bin/gmake all platform=SUN
#


#Set a default platform.
#You can always override the default by specifying "platform=XXX" on
#the make command line.
ifndef platform
platform=LINUX
# UNCOMMENT THE NEXT LINE TO MAKE SUN THE DEFAULT PLATFORM
# platform=SUN
endif

#Define compiler & options
ifeq (${platform}, SUN)
CC=/soft/sparc/bin/gcc
CFLAGS=-g
LFLAGS=-g -lsocket -lresolv
endif

ifeq (${platform}, LINUX)
CC=gcc
CFLAGS=-g
LFLAGS=-g -lm
endif


# Defining the object files:
client-objs = common.o client.o

# The default rule - compiling our main program:
all: client
	echo all: make complete

test:
	echo platform is ${platform}

client: $(client-objs) 
	# If we get here, all the dependencies are now built.
	# Link it:
	$(CC) $(LFLAGS) -o $@ $(client-objs)

# Tell make how to build .o files from .c files:
%.o:%.c
	$(CC) $(CFLAGS) -c $<

#Now make sure that make rebuilds files if included headers change:
client.o: client.h common.h 


clean:	
	rm *.o client
