#
# Operating System and Developing system specific macros
#

# target library file full name

TRGTLIB = libcommon.a

MODULES = OPPEvent OPPState OPPTimerMonitor OPPDebug  \
		  osip_port port_thread port_sema port_fifo port_condv 

BUILD_RELEASE = 2
include ../makefile.inc

$(TRGTLIB) : $(OBJECTS)
	$(AR) -r $(TRGTLIB) $(OBJECTS)

clean:
	$(RM) -f $(TRGTLIB)
	$(RM) -f $(OBJECTS)


