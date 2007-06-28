#include <time.h>
#include <pthread.h>
#include <poll.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include <iostream>

#include "ThreadPool.H"
#include "Exception.H"


static inline
void
tv_sub(struct timeval &a, const struct timeval &b) {
        a.tv_usec -= b.tv_usec;		
        a.tv_sec -= b.tv_sec;
        if (a.tv_usec < 0) {
                a.tv_sec--;
                a.tv_usec += 1000000;
        }
}


static inline
int 
tv_cmp(const struct timeval &a, const struct timeval &b) {
	if(a.tv_sec < b.tv_sec) {
		return -1;
	} else 	if(a.tv_sec > b.tv_sec) {
		return 1;
	} else {
		if (a.tv_usec < b.tv_usec) {
			return -1;
		} else if(a.tv_usec > b.tv_usec) {
			return 1;
		} else {
			return 0;
		}
	} 
}


inline
void 
ThreadPool::WorkDescription::doWork() {
	switch(event) {
	case READY:
		onReady();
		break;
	case TIMEOUT:
		onTimeout();
		break;
	case ERROR:
		onError();
		break;
	default:
		break;
	}
}


inline
void
ThreadPool::WaitDesc::adjustTimeout(const struct timeval &delta)
{
	tv_sub(timeout, delta);
}


ThreadPool::WorkDescription::~WorkDescription() {
}


ThreadPool::ThreadPool() 
	: f_exit(false), work_count(0), wait_count(0), ufds_size(0), ufds(NULL) 
{
	pthread_mutex_init(&wait_queue_mutex, NULL);
	pthread_mutex_init(&work_queue_mutex, NULL);
	pthread_cond_init(&work_queue_cond_ready, NULL);
	pthread_cond_init(&work_queue_cond_full, NULL);
	pthread_cond_init(&wait_queue_cond_ready, NULL);
	pipe(pd);
	ufds = static_cast<struct pollfd *>(malloc(sizeof(struct pollfd)));
	if(ufds == NULL) {
		throw new Exception;
	}
	ufds->fd = pd[0];
	ufds->events = POLLIN;
	ufds_size = 1;
}


ThreadPool::~ThreadPool()
{
	pthread_cond_destroy(&work_queue_cond_full);
	pthread_cond_destroy(&work_queue_cond_ready);
	pthread_cond_destroy(&wait_queue_cond_ready);
	pthread_mutex_destroy(&work_queue_mutex);
	pthread_mutex_destroy(&wait_queue_mutex);
}


void
ThreadPool::startWorkers(unsigned int n) 
{
	workers = new pthread_t[n];

	num_workers = n;
	for(unsigned int i = 0; i < n; i++) {
		// XXX check return 
		pthread_create(&workers[i], NULL, ThreadPool::threadMain, NULL);
	}
}


void 
ThreadPool::stopWorkers()
{
	for(int i = 0; i < num_workers; i++) {
		pthread_cancel(workers[i]);
		pthread_join(workers[i], NULL);
	}
	delete[] workers;
}


void 
ThreadPool::postWork(WorkDescription *work_unit)
{
	E_ASSERT(pthread_mutex_lock(&work_queue_mutex) >= 0);
	work_queue.push_back(work_unit);
	work_count++;
	E_ASSERT(pthread_cond_signal(&work_queue_cond_ready) >= 0);
	E_ASSERT(pthread_mutex_unlock(&work_queue_mutex) >= 0);
}


inline
void 
ThreadPool::queueWork(WaitDesc *wd)
{
	E_ASSERT(pthread_mutex_lock(&wait_queue_mutex) >= 0);
	wait_queue.push_back(wd);
	wait_count++;
	E_ASSERT(pthread_cond_signal(&wait_queue_cond_ready) >= 0);
	E_ASSERT(pthread_mutex_unlock(&wait_queue_mutex) >= 0);
	if(write(pd[1], "1", 1) != 1) {
		throw new Exception;
	}
}


void 
ThreadPool::queueWorkAccept(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLIN));
}
	

void 
ThreadPool::queueWorkRead(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLIN));
}


void
ThreadPool::queueWorkWrite(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLOUT));
}
	

void 
ThreadPool::queueWorkTimeout(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, 0));
}


void 
ThreadPool::queueWorkConnect(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLIN));
}


void 
ThreadPool::setWorkAccept(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLIN, true));
}
	

void 
ThreadPool::setWorkRead(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLIN, true));
}


void
ThreadPool::setWorkWrite(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, POLLOUT, true));
}
	

void 
ThreadPool::setWorkTimeout(WorkDescription *work_unit) 
{
	queueWork(new WaitDesc(work_unit, 0, true));
}


ThreadPool::WorkDescription *
ThreadPool::getWork()
{
	WorkDescription *work_unit = NULL;
	struct timespec timeout;

	E_ASSERT(pthread_mutex_lock(&work_queue_mutex) >= 0);
	if(work_count == 0) {
		timeout.tv_sec = 1;
		timeout.tv_nsec = 0;
//		pthread_cond_timedwait(&work_queue_cond_ready, &work_queue_mutex, &timeout);
		E_ASSERT(pthread_cond_wait(&work_queue_cond_ready, &work_queue_mutex) == 0);
	}
	if(work_count > 0) {
		work_count--;
		work_unit = work_queue.front();
		work_queue.pop_front();
	}
	E_ASSERT(pthread_mutex_unlock(&work_queue_mutex) >= 0);
	return work_unit;
}

void
ThreadPool::threadCleanup(void *data)
{
	ThreadPool *pool = ThreadPool::instance();

	E_ASSERT(pthread_mutex_unlock(&(pool->work_queue_mutex)) >= 0);
}


void *
ThreadPool::threadMain(void *data)
{
	ThreadPool *pool  = ThreadPool::instance();
	WorkDescription *work_unit;

	pthread_cleanup_push(ThreadPool::threadCleanup, NULL); 
	while(true) {

		work_unit = pool->getWork();
		if(work_unit) {
			// something to work on
			work_unit->doWork();
		} else {
			// timed out waiting for work
		}
	}
	pthread_cleanup_pop(1);
}


void 
ThreadPool::removeWaitDesc(std::list<WaitDesc *>::iterator &i)
{
	std::list<WaitDesc *>::iterator j = i;
	
	// actually this is safe even for the first element
	E_ASSERT(pthread_mutex_lock(&wait_queue_mutex) >= 0);
	j--;
	wait_queue.erase(i);
	wait_count--;
	i = j;
	E_ASSERT(pthread_mutex_unlock(&wait_queue_mutex) >= 0);
}


void
ThreadPool::prepareDescriptorArray()
{
	std::list<WaitDesc *>::iterator theIterator;
	struct pollfd *p;

	E_ASSERT(pthread_mutex_lock(&wait_queue_mutex) >= 0);
	if(wait_count == 0) {
		E_ASSERT(pthread_cond_wait(&wait_queue_cond_ready, &wait_queue_mutex) != 0);
	}
	if(wait_count == 0) {
		E_ASSERT(pthread_mutex_unlock(&wait_queue_mutex) >= 0);
		return;
	}
	if(ufds_size != wait_count + 1) {
		ufds = static_cast<struct pollfd *>(realloc(ufds, (1 + wait_count) * sizeof(struct pollfd)));
		if(ufds == NULL) {
			throw new Exception();
		}
		ufds_size = wait_count + 1;
	}
	min_timeout.tv_sec = default_timeout;
	min_timeout.tv_usec = 0;
	for(theIterator = wait_queue.begin(), p = ufds + 1;
	    theIterator != wait_queue.end(); 
	    theIterator++, p++) {
		WaitDesc *w = *theIterator;
		p->fd = w->get_fd();
		p->events = w->event;
		if(tv_cmp(min_timeout, w->timeout) > 0) {
			min_timeout = w->timeout;
		}
	}
	E_ASSERT(pthread_mutex_unlock(&wait_queue_mutex) >= 0);
}


void
ThreadPool::run()
{
	f_exit = false;
	while(!f_exit) {
		struct pollfd *p;
		struct timeval before, after;
		int ret;

		// may block waiting for new work
		prepareDescriptorArray();

		gettimeofday(&before, NULL);
		ret = poll(ufds, ufds_size, 1000*min_timeout.tv_sec + min_timeout.tv_usec/1000);
		gettimeofday(&after, NULL);
		tv_sub(after, before);

		if((ret >= 0) || // ready or timeout
		   ((ret < 0) && (errno == EINTR))) { // interrupted 
			std::list<WaitDesc *>::iterator i;
			WaitDesc *w;

			// handle the pipe
			if(ufds->revents & POLLIN) {
				char discard[1];
				read(ufds->fd, discard, 1);
			}

			// at least we have to adjust timeouts
			E_ASSERT(pthread_mutex_lock(&wait_queue_mutex) >= 0);
			i = wait_queue.begin();
			E_ASSERT(pthread_mutex_unlock(&wait_queue_mutex) >= 0);
			// the wait queue mutex is unlocked inside the loop
			// to allow handlers to add queue new
			// WorkDescriptions - these are added at the
			// end of the list so we should be safe 
			for(p = ufds + 1; p - ufds < ufds_size; p++) {
				enum WorkDescription::Event event = WorkDescription::NONE;
				
				w = *i;
				// check for consistency
				if(p->fd != w->get_fd()) {
					// mismatch, what shall we do?
					throw new Exception;
				}

				// subtract the time passed from timeout
				w->adjustTimeout(after);

				// see what happened
				if(ret <= 0) {
					// timeout or interrupted
					if(w->timeoutExpired()) {
						event = WorkDescription::TIMEOUT;
					}
				} else {
					// ready or error
					if(p->revents & POLLERR) {
						event = WorkDescription::ERROR;
					} else if(p->revents & w->event) {
						event = WorkDescription::READY;
					} else if(w->timeoutExpired()) {
						event = WorkDescription::TIMEOUT;
					}
				}
				if(event != WorkDescription::NONE) {
					WorkDescription *wd;
					wd = w->wd;
					wd->event = event;
					if(!w->f_permanent) {
						postWork(wd);
						removeWaitDesc(i);
						delete w;
					} else {
						w->wd->doWork();
						// we have to reset the timeout
						w->timeout.tv_sec = default_timeout;
						w->timeout.tv_usec = 0;
					}
				}
				E_ASSERT(pthread_mutex_lock(&wait_queue_mutex) >= 0);
				i++;
				E_ASSERT(pthread_mutex_unlock(&wait_queue_mutex) >= 0);
			}
		} else {
			// some nasty error
			throw new Exception;
		}
	}
}

