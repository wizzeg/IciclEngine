#pragma once
#include <mutex>
#include <deque>
#include <optional>
#include <vector>


template <typename TMessage>
struct MessageQueue
{
	std::deque<TMessage> messages;
	std::mutex mutex;

	void add_message(const TMessage& a_message)
	{
		std::lock_guard guard(mutex);
		messages.push_back(a_message);
	}

	void add_messages(const std::vector<TMessage>& a_messages)
	{
		std::lock_guard guard(mutex);
		for (const auto& message : a_messages)
		{
			messages.push_back(message);
		}
	}

	bool is_empty()
	{
		std::lock_guard guard(mutex);
		return messages.empty();
	}
	bool try_is_empty()
	{
		std::unique_lock lock(mutex, std::try_to_lock);
		if (lock.owns_lock())
			return messages.empty();
		return true;
	}

	std::optional<TMessage> get_message()
	{
		std::lock_guard guard(mutex);
		if (messages.empty())
			return std::nullopt;

		TMessage message = messages.front();
		messages.pop_front();
		return message;
	}

	std::optional<std::vector<TMessage>> get_messages()
	{
		std::lock_guard guard(mutex);
		if (messages.empty())
			return std::nullopt;

		std::vector<TMessage> temp_messages;
		temp_messages.reserve(messages.size());
		while (!messages.empty())
		{
			temp_messages.push_back(messages.front());
			messages.pop_front();
		}
		return temp_messages;
	}


};