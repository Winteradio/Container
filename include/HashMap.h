#ifndef __WTR_HASHMAP_H__
#define __WTR_HASHMAP_H__

#include "HashTable.h"

namespace wtr
{
	template<typename Key, typename Value>
	struct PairSelector
	{
		const Key& operator()(const std::pair<Key, Value>& data) const
		{
			return data.first;
		}
	};

	template<typename Key, typename Value,
		typename Hasher = DefaultHasher<Key>,
		typename Comparer = DefaultComparer<Key>,
		typename Selector = PairSelector<Key, Value>,
		typename Allocator = Arena>
	class HashMap : public HashTable<Key, std::pair<Key, Value>, Hasher, Comparer, Selector, Allocator>
	{
	public :
		using Base = HashTable<Key, std::pair<Key, Value>, Hasher, Comparer, Selector, Allocator>;
		using Base::HashTable;

		using MappedType = Value;

		Value& operator[](const Key& key)
		{
			auto itr = this->Find(key);
			if (itr == this->End())
			{
				auto inserted = this->Emplace(key, Value{}).first;
				
				return inserted->second;
			}
			else
			{
				return itr->second;
			}
		}

	public :
		Value& At(const Key& key)
		{
			auto itr = this->Find(key);

			assert(itr != this->End() && "Invalid Key");

			return itr->second;
		}

		const Value& At(const Key& key) const
		{
			auto itr = this->Find(key);

			assert(itr != this->End() && "Invalid Key");

			return itr->second;
		}

		template<typename... Args>
		std::pair<typename Base::HashTable::Iterator, bool> TryEmplace(const Key& key, Args&&... args)
		{
			auto itr = this->Find(key);
			if (itr != this->End())
			{
				return std::make_pair(itr, false);
			}
			else
			{
				return this->Emplace(key, Value{ std::forward<Args>(args)... });
			}
		}
	};
};

#endif // __WTR_HASHMAP_H__