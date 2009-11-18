#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <iostream>

#include "ThreadPool.H"

class TestWork : public ThreadPool::WorkDescription {
public:
	int done;
	
	TestWork(int fd) : ThreadPool::WorkDescription(fd), done(0) {};

	virtual void onReady() {
		done++;
	}
	
};


class TestConsumer : public ThreadPool::WorkDescription {
public:
	char buf[2];

	TestConsumer(int fd) : ThreadPool::WorkDescription(fd) {};

	virtual void onReady() {
		int r;
		
		r = read(fd, buf, 1);
		buf[1] = 0;
		ThreadPool::instance()->exit();
	}

	virtual void onTimeout() {
	}

};


class TestProducer : public ThreadPool::WorkDescription {
public:
	TestProducer(int fd) : ThreadPool::WorkDescription(fd) {};

	virtual void onReady() {
		write(fd, "a", 1);
	}
	
	virtual void onTimeout() {
	}

};


class TestSocketRead: public ThreadPool::WorkDescription {
public:
	char buffer[10];

	TestSocketRead(int fd) : ThreadPool::WorkDescription(fd) {
	}

	virtual void onReady() {
		
		int len = recv(fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
		ThreadPool::instance()->exit();
	}

	virtual void onError() {
	}
};


class TestSocketWrite: public ThreadPool::WorkDescription {
public:
	static char buffer[];

	TestSocketWrite(const char *name) 
		: ThreadPool::WorkDescription(0) {
		struct sockaddr_un saddr;
		int ret;
		fd = socket(PF_UNIX, SOCK_STREAM, 0);
		memset(&saddr, 0, sizeof(saddr));
		saddr.sun_family = AF_UNIX;
		strcpy(saddr.sun_path, name);
		if((ret = connect(fd, (struct sockaddr *)&saddr, sizeof(saddr))) < 0) {
		}
	}

	virtual void onReady() {
		int ret;

		ret = send(fd, buffer, strlen(buffer)+1, MSG_NOSIGNAL);
		close(fd);
	}

};

char TestSocketWrite::buffer[] = "ahoj";

class TestSocketAccept : public ThreadPool::WorkDescription {
public:
	TestSocketRead *reader;

	TestSocketAccept(const char *name) 
		: ThreadPool::WorkDescription(0) {
		struct sockaddr_un saddr;

		fd = socket(PF_UNIX, SOCK_STREAM, 0);
		memset(&saddr, 0, sizeof(saddr));
		saddr.sun_family = AF_UNIX;
		strcpy(saddr.sun_path, name);
		bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
		listen(fd, 1);
	}

	virtual void onReady() {
		int nfd;

		nfd = accept(fd, NULL, NULL);
		if(nfd < 0) { 
		} else {
			ThreadPool *pool = ThreadPool::instance();

			reader  = new TestSocketRead(nfd);
			pool->queueWorkRead(reader);
		}
	}
};


class ThreadPoolTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( ThreadPoolTest );
	CPPUNIT_TEST( testWorkQueue );
	CPPUNIT_TEST( testPoll );
	CPPUNIT_TEST( testAccept );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		pool = ThreadPool::instance();
		unlink("/tmp/smazat.sock");
		pool->startWorkers(2);
	}

	void tearDown() {
		pool->stopWorkers();
	}

	void testWorkQueue() {
		TestWork *work_unit = new TestWork(0);
		pool->postWork(work_unit);
	}

	void testPoll() {
		int fd[2];
		TestProducer *p = new TestProducer(0);
		TestConsumer *c = new TestConsumer(0);

		pipe(fd);
		c->fd = fd[0];
		p->fd = fd[1];
		pool->queueWorkRead(c);
		pool->queueWorkWrite(p);
		pool->run();
		CPPUNIT_ASSERT(c->buf[0] == 'a');
		CPPUNIT_ASSERT(c->buf[1] == 0);
	}

	void testAccept() {
		TestSocketAccept *consumer = new TestSocketAccept("/tmp/smazat.sock");
		TestSocketWrite *producer;

		pool->queueWorkAccept(consumer);
		producer = new TestSocketWrite("/tmp/smazat.sock");
		ThreadPool::instance()->queueWorkWrite(producer);
		pool->run();
		CPPUNIT_ASSERT(consumer->reader != NULL);
		CPPUNIT_ASSERT(strcmp(consumer->reader->buffer, TestSocketWrite::buffer) == 0);
	}

private:
	ThreadPool *pool;
};


CPPUNIT_TEST_SUITE_REGISTRATION( ThreadPoolTest );
