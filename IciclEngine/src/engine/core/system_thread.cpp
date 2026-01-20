#include <engine/core/system_thread.h>

SystemThreadPool::SystemThreadPool(int a_num_threads)
{
	for (size_t i = 0; i < a_num_threads; i++)
	{
		system_threads.emplace_back([this]
			{
				while (true)
				{
					//std::function<void()> job;
					{
						std::unique_lock job_lock(jobs_mutex);
						job_cv.wait(job_lock, [this] { return stop || !jobs.is_empty(); });
						if (stop && jobs.is_empty()) return;
						active_threads++;
					}
					if (auto job = jobs.get_message())
						job.value()();
					active_threads--;
					join_cv.notify_one();

				}
			});
	}
}

SystemThreadPool::~SystemThreadPool()
{
	stop = true;
	job_cv.notify_all();
	join_cv.notify_all();
	for (auto& thread : system_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

bool SystemThreadPool::wait()
{
	std::unique_lock job_lock(jobs_mutex);
	join_cv.wait(job_lock, [this] { return stop || (jobs.is_empty() && active_threads == 0); });
	if (stop) return false;
	return true;
}