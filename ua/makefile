#
# Operating System and Developing system specific macros
#

# target library file full name
TRGTLIB = libua.a

#
# Target dependent macros
#

MODULES =   OPPSipChannel  OPPChannelManager  OPPApi   \
            OPPSipService OPPChannelSM  

BUILD_RELEASE = 2
include ../makefile.inc

$(TRGTLIB) : $(OBJECTS)
	$(AR) -r $(TRGTLIB) $(OBJECTS)

clean:
	$(RM) -f $(TRGTLIB)
	$(RM) -f $(OBJECTS)


