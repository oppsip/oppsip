#
# Operating System and Developing system specific macros
#

# target library file full name

TRGTEXE = cm
LIB = ua osiptrans common osipparser2  rt lua

#
# Target dependent macros
#

MODULES = base64 main hybi10 sha1 wsHandshake wsLuaHook wsServer AppTimer


BUILD_RELEASE = 2
include ../makefile.inc

$(TRGTEXE) : $(OBJECTS)
	$(LK) $(TRGTEXE)  $(OBJECTS) $(LIBS) $(LIBPATH)  -lpthread -ldl -L ../lua-5.2.3/install/lib

clean:
	$(RM) -f $(TRGTEXE)
	$(RM) -f $(OBJECTS)
