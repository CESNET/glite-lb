
CXX = c++
CC = gcc

CFLAGS = -g -Wall

COMPILEPP = $(CXX) $(CXXFLAGS) $(CFLAGS)
COMPILE = $(CC) $(CFLAGS)
LINK = libtool --mode=link g++ $(LDFLAGS)

THREAD_LIB = -lpthread

CPPUNIT_ROOT = 
CPPUNIT_LIB = -lcppunit -ldl
CPPUNIT_INCLUDE = 

TEST_OBJS= \
	test/ThreadPoolTest.o \
	test/PluginManagerTest.o \
	test/EventManagerTest.o \
	test/SingletonTest.o \
	test/test_main.o

OBJS = \
	PluginManager.o \
	SocketInput.o \
	Connection.o \
	PlainConnection.o \
	Transport.o \
	HTTPTransport.o \
	ThreadPool.o \
	EventManager.o

plain: main.o $(OBJS)
	$(LINK) -o $@ $+ $(THREAD_LIB)

utest: ThreadPool.o PluginManager.o EventManager.o $(TEST_OBJS)
	$(LINK) -o $@ $+ $(CPPUNIT_LIB) $(THREAD_LIB)

stest: EventManager.o test/EventManagerTest.o test/test_main.o
	$(LINK) -o $@ $+ $(CPPUNIT_LIB) $(THREAD_LIB)

$(TEST_OBJS): %.o: %.cpp
	$(COMPILEPP) -I. $(CPPUNIT_INCLUDE) -o $@ -c $<

%.o: %.cpp
	$(COMPILEPP) -o $@ -c $<

%.o: %.c
	$(COMPILE) -o $@ -c $<