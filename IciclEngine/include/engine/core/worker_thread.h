#pragma once
#include <thread>
#include <queue>
#include <atomic>
#include <algorithm>
#include <engine/resources/message_queue.h>
#include <functional>

struct WorkerThreadPool
{
	WorkerThreadPool(int a_num_threads = std::thread::hardware_concurrency());
	~WorkerThreadPool();
	template <typename F>
	void enqueue(F&& f)
	{
		jobs.add_functor_message(std::forward<F>(f));
		job_cv.notify_one();
	}

	bool wait(); // do not add additional jobs from another thread after this is called
	bool poll();
	size_t get_num_threads() { return num_threads; }
	void start();
protected:
	size_t num_threads = 0;
	std::vector<std::thread> worker_threads;
	MessageQueue<std::function<void()>> jobs;
	//std::queue<std::function<void()>> jobs;
	std::mutex jobs_mutex;
	std::condition_variable job_cv;
	bool stop = false;
	int active_threads = 0;
	std::condition_variable join_cv;
	std::condition_variable poll_cv;
};