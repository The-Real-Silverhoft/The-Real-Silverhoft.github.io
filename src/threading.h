/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef THREADING_H
#define THREADING_H 1

#include "compat.h"
#include <cstdlib>
#include <cassert>
#include <vector>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

namespace lightspark
{
class Mutex
{
friend class Cond;
friend class Locker;
	SDL_mutex* m;
public:
	Mutex()
	{
		m = SDL_CreateMutex();
	}
	~Mutex()
	{
		SDL_DestroyMutex(m);
	}
	inline void lock()
	{
		SDL_LockMutex(m);
	}
	inline void unlock()
	{
		SDL_UnlockMutex(m);
	}
	inline bool trylock()
	{
		return SDL_TryLockMutex(m)==0;
	}
};

class Locker
{
	SDL_mutex* m;
public:
	Locker(Mutex& mutex):m(mutex.m)
	{
		SDL_LockMutex(m);
	}
	~Locker()
	{
		SDL_UnlockMutex(m);
	}
	inline void acquire()
	{
		SDL_LockMutex(m);
	}
	inline void release()
	{
		SDL_UnlockMutex(m);
	}
};
class Cond
{
	SDL_cond* c;
public:
	Cond()
	{
		c= SDL_CreateCond();
	}
	~Cond()
	{
		SDL_DestroyCond(c);
	}
	inline void broadcast()
	{
		SDL_CondBroadcast(c);
	}
	inline void wait(Mutex& mutex)
	{
		SDL_CondWait(c,mutex.m);
	}
	inline void signal()
	{
		SDL_CondSignal(c);
	}
	inline bool wait_until(Mutex& mutex,uint32_t ms)
	{
		return SDL_CondWaitTimeout(c,mutex.m,ms)==0;
	}
};

#define DEFINE_AND_INITIALIZE_TLS(name) static SDL_TLSID name = SDL_TLSCreate()
void tls_set(SDL_TLSID key, void* value);
void* tls_get(SDL_TLSID key);

class DLL_PUBLIC Semaphore
{
private:
	SDL_sem * sem;
public:
	Semaphore(uint32_t init);
	~Semaphore();
	void signal();
	void wait();
	bool try_wait();
};

class SemaphoreLighter
{
private:
	Semaphore& _s;
	bool lighted;
public:
	SemaphoreLighter(Semaphore& s):_s(s),lighted(false){}
	~SemaphoreLighter()
	{
		if(!lighted)
			_s.signal();
	}
	void light()
	{
		assert(!lighted);
		_s.signal();
		lighted=true;
	}
};
class ASWorker;

class IThreadJob
{
friend class ThreadPool;
private:
	ASWorker* fromWorker;
public:
	/*
	 * Set to true by the ThreadPool just before threadAbort()
	 * is called. For some implementations, it may be enough
	 * to poll threadAborted and not implement threadAbort().
	 */
	volatile bool threadAborting;
	/*
	 * Called in a dedicated thread to do the actual
	 * work. You may throw a JobTerminationException
	 * to quit the job. (Or better: just return)
	 */
	virtual void execute()=0;
	/*
	 * Called asynchronously to abort a job
	 * who is currently in execute().
	 * 'aborting' is set to true before calling
	 * this function.
	 */
	virtual void threadAbort() {}
	/*
	 * Called after the job has finished execute()'ing or
	 * if the ThreadPool aborts and the job did not have
	 * a chance to run yet.
	 * You should use jobFence to signal blocking semaphores
	 * and a like. There is no access to this object after
	 * jobFence() is called, so you may safely call
	 * 'delete this'.
	 */
	virtual void jobFence()=0;
	IThreadJob() : fromWorker(nullptr),threadAborting(false) {}
	virtual ~IThreadJob() {}
	void setWorker(ASWorker* w) { fromWorker = w;}
};

template<class T, uint32_t size>
class BlockingCircularQueue
{
private:
	T queue[size];
	//Counting semaphores for the queue
	Semaphore freeBuffers;
	Semaphore usedBuffers;
	uint32_t bufferHead;
	uint32_t bufferTail;
	ACQUIRE_RELEASE_FLAG(empty);
public:
	BlockingCircularQueue():freeBuffers(size),usedBuffers(0),bufferHead(0),bufferTail(0),empty(true)
	{
	}
	template<class GENERATOR>
	BlockingCircularQueue(const GENERATOR& g):freeBuffers(size),usedBuffers(0),bufferHead(0),bufferTail(0),empty(true)
	{
		for(uint32_t i=0;i<size;i++)
			g.init(queue[i]);
	}
	bool isEmpty() const { return empty; }
	T& front()
	{
		assert(!this->empty);
		return queue[bufferHead];
	}
	const T& front() const
	{
		assert(!this->empty);
		return queue[bufferHead];
	}
	bool nonBlockingPopFront()
	{
		//We don't want to block if empty
		if(!usedBuffers.try_wait())
			return false;
		//A frame is available
		bufferHead=(bufferHead+1)%size;
		if(bufferHead==bufferTail)
			empty=true;
		freeBuffers.signal();
		return true;
	}
	T& acquireLast()
	{
		freeBuffers.wait();
		uint32_t ret=bufferTail;
		bufferTail=(bufferTail+1)%size;
		return queue[ret];
	}
	void commitLast()
	{
		empty=false;
		usedBuffers.signal();
	}
	template<class GENERATOR>
	void regen(const GENERATOR& g)
	{
		for(uint32_t i=0;i<size;i++)
			g.init(queue[i]);
	}
	uint32_t len() const
	{
		uint32_t tmp=(bufferTail+size-bufferHead)%size;
		if(tmp==0 && !empty)
			tmp=size;
		return tmp;
	}

};

// This class represents the end time when waiting on a conditional
// variable.
class CondTime {
private:
	gint64 timepoint;
public:
	CondTime(long milliseconds);
	bool operator<(CondTime& c) const;
	bool operator>(CondTime& c) const;
	bool isInTheFuture() const;
	void addMilliseconds(long ms);
	bool wait(Mutex &mutex, Cond& cond);
};
}

#endif /* THREADING_H */
