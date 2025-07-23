#pragma once
#include "../util/logger.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace mc::utils;

namespace mc::threading {

class ThreadManager {
public:
  ThreadManager(size_t poolSize = std::thread::hardware_concurrency()) {
    running = true;
    log(LogLevel::INFO, "Starting thread pool with ", poolSize, " threads.");
    for (size_t i = 0; i < poolSize; ++i) {
      threadPool.emplace_back([this, i]() {
        log(LogLevel::DEBUG, "ThreadPool worker #", i, " started.");
        this->poolWorker();
        log(LogLevel::DEBUG, "ThreadPool worker #", i, " exiting.");
      });
    }

    startDedicatedThread("Audio");
    startDedicatedThread("Networking");
    startDedicatedThread("Rendering");
  }

  ~ThreadManager() { stop(); }

  template <typename Func, typename... Args>
  auto submitToPool(Func &&f, Args &&...args)
      -> std::future<decltype(f(args...))> {
    using ReturnType = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

    {
      std::unique_lock<std::mutex> lock(poolMutex);
      poolQueue.emplace([task]() { (*task)(); });
    }

    poolCv.notify_one();
    log(LogLevel::DEBUG, "Task submitted to general thread pool.");
    return task->get_future();
  }

  void submitToDedicated(const std::string &name, std::function<void()> task) {
    std::unique_lock<std::mutex> lock(dedicatedMutex);
    if (dedicatedThreads.count(name)) {
      dedicatedQueues[name].emplace(std::move(task));
      dedicatedCvs[name].notify_one();
      log(LogLevel::DEBUG, "Task submitted to dedicated thread: ", name);
    } else {
      log(LogLevel::ERROR, "No dedicated thread named: ", name);
    }
  }

  void stop() {
    {
      std::unique_lock<std::mutex> lock(poolMutex);
      running = false;
    }
    poolCv.notify_all();
    for (auto &t : threadPool) {
      if (t.joinable())
        t.join();
    }
    log(LogLevel::INFO, "Thread pool shut down.");

    {
      std::unique_lock<std::mutex> lock(dedicatedMutex);
      running = false;
    }
    for (auto &[name, cv] : dedicatedCvs) {
      cv.notify_all();
    }
    for (auto &[name, t] : dedicatedThreads) {
      if (t.joinable()) {
        t.join();
        log(LogLevel::INFO, "Dedicated thread shut down: ", name);
      }
    }
  }

private:
  std::atomic<bool> running;

  std::vector<std::thread> threadPool;
  std::queue<std::function<void()>> poolQueue;
  std::mutex poolMutex;
  std::condition_variable poolCv;

  void poolWorker() {
    while (running) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(poolMutex);
        poolCv.wait(lock, [this]() { return !poolQueue.empty() || !running; });
        if (!running && poolQueue.empty())
          return;
        task = std::move(poolQueue.front());
        poolQueue.pop();
      }
      task();
    }
  }

  std::unordered_map<std::string, std::thread> dedicatedThreads;
  std::unordered_map<std::string, std::queue<std::function<void()>>>
      dedicatedQueues;
  std::unordered_map<std::string, std::condition_variable> dedicatedCvs;
  std::mutex dedicatedMutex;

  void startDedicatedThread(const std::string &name) {
    {
      std::unique_lock<std::mutex> lock(dedicatedMutex);

      dedicatedQueues.emplace(name, std::queue<std::function<void()>>{});

      // ✅ Wichtig: In-place Konstruktion der condition_variable
      dedicatedCvs.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple());
    }

    dedicatedThreads[name] = std::thread([this, name]() {
      log(LogLevel::INFO, "Dedicated thread started: ", name);
      while (running) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(dedicatedMutex);
          dedicatedCvs[name].wait(lock, [&]() {
            return !dedicatedQueues[name].empty() || !running;
          });
          if (!running && dedicatedQueues[name].empty())
            return;
          task = std::move(dedicatedQueues[name].front());
          dedicatedQueues[name].pop();
        }
        task();
      }
    });
  }
};

} // namespace mc::threading
