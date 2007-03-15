
CXX = c++
CC = gcc

CFLAGS = -g -Wall

COMPILEPP = $(CXX) $(CXXFLAGS) $(CFLAGS)
COMPILE = $(CC) $(CFLAGS)
LINK = libtool --mode=link g++ $(LDFLAGS)

THREAD_LIB = -lpthread

CPPUNIT_ROOT = /afs/ruk.cuni.cz/home/michal/egee/repository/externals/cppunit/1.10.2/slc3_ia32_gcc323
CPPUNIT_LIB = -L$(CPPUNIT_ROOT)/lib -lcppunit -ldl
CPPUNIT_INCLUDE = -I$(CPPUNIT_ROOT)/include

TEST_OBJS= \
	test/ThreadPoolTest.o \
	test/PluginManagerTest.o \
	test/EventManagerTest.o \
	test/SingletonTest.o \
	test/test_main.o

plain: SocketInput.o PlainConnection.o HTTPTransport.o ThreadPool.o main.o
	$(LINK) -o $@ $+ $(THREAD_LIB)

utest: ThreadPool.o PluginManager.o EventManager.o $(TEST_OBJS)
	$(LINK) -o $@ $+ $(CPPUNIT_LIB) $(THREAD_LIB)

stest: test/SingletonTest.o test/test_main.o
	$(LINK) -o $@ $+ $(CPPUNIT_LIB) $(THREAD_LIB)

$(TEST_OBJS): %.o: %.cpp
	$(COMPILEPP) -I. $(CPPUNIT_INCLUDE) -o $@ -c $<

%.o: %.cpp
	$(COMPILEPP) -o $@ -c $<

%.o: %.c
	$(COMPILE) -o $@ -c $<