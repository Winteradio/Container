#ifndef __WTR_HASHMAP_H__
#define __WTR_HASHMAP_H__

#include "HashTable.h"

#include <unordered_map>

std::unordered_map<int, int> map;

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
			auto itr = Find(key);
			if (itr == End())
			{
				auto inserted = Emplace(key, Value{}).first;
				
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
			auto itr = Find(key);

			assert(itr != End() && "Invalid Key");

			return itr->second;
		}

		const Value& At(const Key& key) const
		{
			auto itr = Find(key);

			assert(itr != End() && "Invalid Key");

			return itr->second;
		}
	};
};

#endif // __WTR_HASHMAP_H__