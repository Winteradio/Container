#ifndef __WTR_HASHSET_H__
#define __WTR_HASHSET_H__

#include "HashTable.h"

namespace wtr
{
	template<typename Value, 
		typename Hasher = DefaultHasher<Value>,
		typename Comparer = DefaultComparer<Value>,
		typename Selector = DefaultSelector<Value>,
		typename Allocator = Arena>
	class HashSet : public HashTable<Value, Value, Hasher, Comparer, Selector, Allocator>
	{
	public :
		using Base = HashTable<Value, Value, Hasher, Comparer, Selector, Allocator>;
		using Base::HashTable;
	};
};

#endif // __WTR_HASHSET_H__