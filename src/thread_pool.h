#ifndef ROAMINGID_THREAD_POOL_H_
#define ROAMINGID_THREAD_POOL_H_

#include <condition_variable>
#include <cstring>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

// A C++11 friendly thread pool
class ThreadPool
{
 public:
  ThreadPool(std::size_t);

  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>;

  const std::vector<std::thread::id>& GetThreadIDArray() const { return tids; }

  ~ThreadPool();

 private:
  // thread_id array - add by wang.song(180117)
  std::vector<std::thread::id> tids;
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers;
  // the task queue
  std::queue<std::function<void()>> tasks;

  // synchronization
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads) : stop(false)
{
  for (std::size_t i = 0; i < threads; ++i)
  {
    std::promise<std::thread::id> p;
    std::future<std::thread::id> f = p.get_future();
    workers.emplace_back(
        [this, &p]
        {
          p.set_value(std::this_thread::get_id());
          for (;;)
          {
            std::function<void()> task;

            {
              std::unique_lock<std::mutex> lock(this->queue_mutex);
              this->condition.wait(
                  lock, [this] { return this->stop || !this->tasks.empty(); });
              if (this->stop && this->tasks.empty())
              {
                return;
              }
              task = std::move(this->tasks.front());
              this->tasks.pop();
            }

            task();
          }
        });
    std::thread::id thread_id = f.get();
    tids.push_back(thread_id);
  }
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // don't allow enqueueing after stopping the pool
    if (stop)
    {
      throw std::runtime_error("enqueue on stopped ThreadPool");
    }

    tasks.emplace([task]() { (*task)(); });
  }
  condition.notify_one();
  return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  for (std::thread& worker : workers)
  {
    worker.join();
  }
}

#endif  // ROAMINGID_THREAD_POOL_H_
