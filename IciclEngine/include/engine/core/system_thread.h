#pragma once
#include <thread>
#include <queue>
#include <atomic>
#include <algorithm>
#include <engine/resources/message_queue.h>
#include <functional>

struct SystemThreadPool
{
	SystemThreadPool(int a_num_threads = std::thread::hardware_concurrency());

	~SystemThreadPool();
	template <typename F>
	void enqueue(F&& f)
	{
		jobs.add_functor_message(std::forward<F>(f));
		job_cv.notify_one();
	}

	bool wait(); // do not add additional jobs from another thread after this is called

protected:
	std::vector<std::thread> system_threads;
	MessageQueue<std::function<void()>> jobs;
	//std::queue<std::function<void()>> jobs;
	std::mutex jobs_mutex;
	std::condition_variable job_cv;
	std::atomic<bool> stop{ false };
	std::atomic<int> active_threads{ 0 };
	std::condition_variable join_cv;
};