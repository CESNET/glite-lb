#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <pthread.h>
#include <poll.h>
#include <time.h>

#include <list>

class ThreadPool {
public:
	const static int default_timeout = 5;

	class WorkDescription {
		friend class ThreadPool;
	public:
		int fd;
		
		WorkDescription(int afd) 
			: fd(afd), event(NONE) {}

		virtual void onReady() 
			{}

		virtual void onTimeout() 
			{}

		virtual void onError() 
			{}

	protected:
		enum Event { NONE, READY, TIMEOUT, ERROR } event;
		void doWork();
	};

public:
	static ThreadPool theThreadPool;

	static ThreadPool *getThreadPool() 
		{ return &theThreadPool; }

	void startWorkers(unsigned int n);
	void stopWorkers();

	void postWork(WorkDescription *work_unit);

	void queueWorkAccept(WorkDescription *work_unit);
	void queueWorkRead(WorkDescription *work_unit);
	void queueWorkWrite(WorkDescription *work_unit);
	void queueWorkTimeout(WorkDescription *work_unit);
	void queueWorkConnect(WorkDescription *work_unit);

	void setWorkAccept(WorkDescription *work_unit);
	void setWorkRead(WorkDescription *work_unit);
	void setWorkWrite(WorkDescription *work_unit);
	void setWorkTimeout(WorkDescription *work_unit);

	void run();
	void exit() 
		{ f_exit = true; pthread_cond_signal(&wait_queue_cond_ready); }

protected:
	ThreadPool();
	~ThreadPool();

	WorkDescription *getWork();

private:
	class WaitDesc {
	public:
		WorkDescription *wd;
		short event;
		bool f_permanent;
		struct timeval timeout;

		WaitDesc(WorkDescription *w, short e, 
			 bool permanent = false, int t = default_timeout) 
			: wd(w), event(e), f_permanent(permanent) {
			timeout.tv_sec = t;
			timeout.tv_usec = 0;
		}
		
		int get_fd() { return wd->fd; };
		void adjustTimeout(const struct timeval &delta);
		int timeoutExpired() { return((timeout.tv_sec < 0) ||
					      (timeout.tv_sec == 0 && timeout.tv_usec == 0)); }
	};

private:
	bool f_exit;
	int num_workers;
	pthread_t *workers;
	int work_count;
	int wait_count;
	int ufds_size;
	std::list<WorkDescription *> work_queue;
	std::list<WaitDesc *> wait_queue;
	pthread_mutex_t work_queue_mutex;
	pthread_cond_t work_queue_cond_ready;
	pthread_cond_t work_queue_cond_full;
	pthread_mutex_t wait_queue_mutex;
	pthread_cond_t wait_queue_cond_ready;
	struct pollfd *ufds;
	int pd[2];
	struct timeval min_timeout;

	void prepareDescriptorArray();
	void removeWaitDesc(std::list<WaitDesc *>::iterator &i);
	void removeWorkDesc();
	void queueWork(WaitDesc *);

	static void *threadMain(void *);
	static void threadCleanup(void *);
};

#endif
