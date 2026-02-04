#include <engine/core/worker_thread.h>

WorkerThreadPool::WorkerThreadPool(int a_num_threads) 
	: num_threads((size_t)a_num_threads), jobs(), jobs_mutex(), job_cv(), join_cv()
{
	
}

WorkerThreadPool::~WorkerThreadPool()
{
	{
		std::lock_guard jobs_guard(jobs_mutex);
		stop = true;
		job_cv.notify_all();
		join_cv.notify_all();
	}

	for (auto& thread : worker_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void WorkerThreadPool::start()
{
	if (worker_threads.empty())
	{
		for (size_t i = 0; i < num_threads; i++)
		{
			size_t thread_id = i;
			worker_threads.emplace_back([this, thread_id]
				{
					while (true)
					{
						std::function<void()> job;
						{
							std::unique_lock job_lock(jobs_mutex);
							job_cv.wait(job_lock, [this] { return stop || !jobs.is_empty(); });
							if (stop && jobs.is_empty()) return;
							active_threads++;
							if (auto optional_job = jobs.get_message())
								job = optional_job.value();
						}
						job();
						{
							std::unique_lock job_lock(jobs_mutex);
							active_threads--;
							if (jobs.is_empty() && active_threads == 0)
								join_cv.notify_all();
						}
					}
				});
		}
	}
}

bool WorkerThreadPool::wait()
{
	std::unique_lock job_lock(jobs_mutex);
	join_cv.wait(job_lock, [this] { return stop || (jobs.is_empty() && active_threads == 0); });
	if (stop) return false;
	return true;
}