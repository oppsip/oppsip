#
# Operating System and Developing system specific macros
#
RM      = rm
MAKE    = make

CC      = gcc  -c -g
CPP     = g++ -c -g
AS      = as 
AR      = ar
LD      = ld
LK      = g++ -g -o

#CC      = arm-softfloat-linux-gnu-gcc  -c  
#CPP     = arm-softfloat-linux-gnu-g++ -c  
#AS      = arm-softfloat-linux-gnu-as 
#AR      = arm-softfloat-linux-gnu-ar
#LD      = arm-softfloat-linux-gnu-ld
#LK      = arm-softfloat-linux-gnu-g++ -o
#MD      = arm-softfloat-linux-gnu-g++ -shared -o
COPY    = cp
CFLAGS	 := -O2  -Wall -DLINUX -DLINUX_OS -DNO_AUTOCONFIG -DWEBRTC_LINUX $(CFLAGS)

CPPFLAGS   := -O2  -Wall -DLINUX -DLINUX_OS -DNO_AUTOCONFIG -DWEBRTC_LINUX $(CPPFLAGS)

INCLUDES = ../osipparser2/ ../common ../trans ../ua ../lua-5.2.3/install/include

INCFLAGS = $(INCLUDES:%=-I%)
CFLAGS  += $(INCFLAGS)
CPPFLAGS += $(INCFLAGS)
OBJECTS  = $(MODULES:%=%.o)
LIBS = $(LIB:%=-l%)
LIBPATH = $(INCLUDES:%=-L%)

.cpp.o :
	$(CPP) $(CPPFLAGS) -o $*.o $*.cpp

.c.o :
	$(CC) $(CFLAGS) -o $*.o $*.c

.s.o :
	$(AS) -o $*.o $*.s

