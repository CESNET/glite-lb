
CXX = c++
CC = gcc

CFLAGS = -g -Wall

COMPILEPP = $(CXX) $(CXXFLAGS) $(CFLAGS)
COMPILE = $(CC) $(CFLAGS)
LINK = c++ $(LDFLAGS)

THREAD_LIB = -lpthread


plain: SocketInput.o PlainConnection.o HTTPTransport.o PluginManager.o ThreadPool.o main.o
	$(LINK) -o $@ $+ $(THREAD_LIB)

%.o: %.cpp
	$(COMPILEPP) -o $@ -c $<

%.o: %.c
	$(COMPILE) -o $@ -c $<