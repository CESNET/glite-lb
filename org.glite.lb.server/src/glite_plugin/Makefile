GSSAPI_ROOT = /software/globus-3.2-pre
FLAVOR = gcc32dbg
GSOAP_ROOT = /software/gsoap-2.6

PLUGIN_OBJS = ws_plugin.o lb_gss.o

CPPFLAGS = -DSTANDALONE_BUILD -D_GNU_SOURCE \
	-I$(GSSAPI_ROOT)/include/$(FLAVOR) -I$(GSSAPI_ROOT)/include \
	-I$(GSOAP_ROOT)/include
CFLAGS = -Wall

all: client server

client: $(PLUGIN_OBJS)

clean:
	$(RM) $(PLUGIN_OBJS) client server core 
