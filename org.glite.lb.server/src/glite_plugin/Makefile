GSSAPI_ROOT = /software/globus-3.2-pre
FLAVOR = gcc32dbg
GSOAP_ROOT = /software/gsoap-2.6
ARES_ROOT =

CPPFLAGS = -DSTANDALONE_BUILD \
	-I$(GSSAPI_ROOT)/include/$(FLAVOR) -I$(GSSAPI_ROOT)/include \
	-I$(GSOAP_ROOT)/include
CFLAGS = -Wall
LDFLAGS = -L$(GSSAPI_ROOT)/lib -L$(GSOAP_ROOT)/lib
LDLIBS = -lglobus_gssapi_gsi_$(FLAVOR) -lares

PLUGIN_OBJS = ws_plugin.o lb_gss.o

all: server

server: $(PLUGIN_OBJS) server.o

client: $(PLUGIN_OBJS) client.o

clean:
	$(RM) *.o client server core 
