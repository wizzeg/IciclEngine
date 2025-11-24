//#pragma once
//
//#include <vector>
//#include <memory>
//#include <mutex>
//#include <random>
//#include <engine/utilities/macros.h>
//#include <engine/renderer/render_info.h>
//
//struct LockKey
//{
//	uint8_t key;
//	std::unique_lock<std::mutex> lock;
//	LockKey(uint8_t a_key, std::unique_lock<std::mutex>&& a_lock) : key(a_key), lock(std::move(a_lock)) {};
//};
//
//struct DataStorageBase
//{
//	virtual const std::type_info& get_data_type() = 0;
//	virtual const std::type_info& get_class_type() = 0;
//};
//
//template <typename TData>
//struct DataStorage : DataStorageBase
//{
//
//	DataStorage() : key(0) {};
//
//	virtual LockKey get_mutex_lock()
//	{
//		std::unique_lock<std::mutex> lock(mutex);
//		key = get_random_key();
//		return LockKey{ key, lock };
//	}
//
//
//	virtual std::optional<LockKey> try_get_mutex_lock() {
//		std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
//		if (lock.owns_lock())
//		{
//			key = get_random_key();
//			return LockKey{ key, lock };
//		}
//		else
//		{
//			return std::nullopt;
//		}
//	};
//
//	virtual bool add_data(const TData& a_data, const LockKey& a_key)
//	{
//		if (a_key.key == key)
//		{
//			data_storage.emplace_back(std::make_unique<TData>(a_data));
//			return true;
//		}
//		else
//		{
//			PRINTLN("WARNING: tried to add data with invalid key");
//		}
//		return false;
//	}
//	virtual void add_data_nodupe(const TData& a_data, const LockKey& a_key)
//	{
//		PRINTLN("WARNING: no implenetation");
//	};
//	virtual std::vector<std::unique_ptr<TData>>* get_data_vector(const LockKey& a_key)
//	{
//		if (a_key.key == key)
//		{
//			return &data_storage;
//		}
//		else
//		{
//			PRINTLN("WARNING: tried to add data with invalid key");
//			return nullptr;
//		}
//	}
//	virtual TData* get_data_at(size_t a_index, const LockKey& a_key)
//	{
//		if (a_key.key == key)
//		{
//			if (a_index < data_storage.size())
//			{
//				return data_storage[a_index].get();
//			}
//			else
//			{
//				PRINTLN("data_storage index out of bounds");
//				return nullptr;
//			}
//		}
//		else
//		{
//			PRINTLN("WARNING: tried to add data with invalid key");
//			return nullptr;
//		}
//	};
//
//	virtual typename std::optional<typename std::vector<std::unique_ptr<TData>>::const_iterator> begin(const LockKey& a_key) const
//	{ 
//		if (a_key.key == key)
//		{
//			return data_storage.begin();
//		}
//		else
//		{
//			PRINTLN("WARNING: tried to add data with invalid key");
//			return std::nullopt;
//		}
//	}
//	virtual typename std::optional<typename std::vector<std::unique_ptr<TData>>::const_iterator> end(const LockKey& a_key) const
//	{
//		if (a_key.key == key)
//		{
//			return data_storage.end();
//		}
//		else
//		{
//			PRINTLN("WARNING: tried to add data with invalid key");
//			return std::nullopt;
//		}
//		
//	}
//
//	virtual const std::type_info& get_data_type() override { return typeid(TData); }
//	virtual const std::type_info& get_class_type() override { return typeid(*this); }
//
//	
//
//private:
//	uint8_t get_random_key()
//	{
//		static thread_local std::mt19937 rng(std::random_device{}());
//		std::uniform_int_distribution<int> dist(1, 255);
//		return (uint8_t)dist(rng);
//	}
//
//	std::vector<std::unique_ptr<TData>> data_storage;
//	std::mutex mutex;
//	uint8_t key;
//};
//
//
//struct MeshDataStorage : DataStorage<MeshData>
//{
//
//};