# pragma once

# include <iostream>
# include <queue>
# include "Task.hpp"
# include <pthread.h>
# include "Log.hpp"
# define NUM 6

class ThreadPool
{
private:
  std::queue<Task> _task_queue;
  int _num;
  bool _stop;
  pthread_mutex_t _lock;
  pthread_cond_t _cond;
  
  ThreadPool(int num = NUM):_num(num), _stop(false)
  {
    pthread_mutex_init(&_lock, nullptr);
    pthread_cond_init(&_cond, nullptr);
  }
  static ThreadPool* single_isntance;
public:
  static ThreadPool* getinstance()
  {
    static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
    if (single_isntance == nullptr)
    {
      pthread_mutex_lock(&_mutex);
      if (single_isntance == nullptr)
      {
        single_isntance = new ThreadPool();
        single_isntance->InitThreadPool();
      }
      pthread_mutex_unlock(&_mutex);
    }
    return single_isntance;
  }
  bool IsStop()
  {
    return _stop;
  }

  bool IsTaskQueueEmpty()
  {
    return _task_queue.size() == 0 ? true : false;
  }

  void ThreadWait()
  {
    pthread_cond_wait(&_cond, &_lock);
  }

  void ThreadWakeup()
  {
    pthread_cond_signal(&_cond);
  }
  
  void Lock()
  {
    pthread_mutex_lock(&_lock);
  }

  void Unlock()
  {
    pthread_mutex_unlock(&_lock);
  }

  static void* ThreadRoutine(void* args)
  {
   ThreadPool* tp = (ThreadPool*)args;

   while (true)
   {
    Task t;
    tp->Lock();
    while (tp->IsTaskQueueEmpty())
    {
      tp->ThreadWait();
    }
    tp->PopTask(t);
    tp->Unlock();
    t.ProcessOn();
   }
  }

  bool InitThreadPool()
  {
    for (int i = 0; i < _num; i++)
    {
      pthread_t tid;
      if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
      {
        LOG(FATAL, "create thread pool error");
        return false;
      }
    }
    LOG(INFO, "create thread pool success");
    return true;
  }

  void PushTask(const Task& task)
  {
    Lock();
    _task_queue.push(task);
    Unlock();
    ThreadWakeup();
  }

  void PopTask(Task& task)
  {
    task = _task_queue.front();
    _task_queue.pop();
  }

  ~ThreadPool()
  {
    pthread_mutex_destroy(&_lock);
    pthread_cond_destroy(&_cond);
  }
};

ThreadPool* ThreadPool::single_isntance = nullptr;



