#ifndef __WTR_HASHTABLE_H__
#define __WTR_HASHTABLE_H__

#include "DynamicArray.h"

namespace wtr
{
	template<typename T>
	struct DefaultHasher
	{
		size_t operator()(const T& key) const
		{
			return std::hash<T>()(key);
		}
	};

	template<typename T>
	struct DefaultComparer
	{
		bool operator()(const T& lhs, const T& rhs) const
		{
			return lhs == rhs;
		}
	};

	template<typename T>
	struct DefaultSelector
	{
		const T& operator()(const T& data) const
		{
			return data;
		}
	};

	template<typename Key, typename Data = Key,
		typename Hasher = DefaultHasher<Key>,
		typename Comparer = DefaultComparer<Key>, 
		typename Selector = DefaultSelector<Data>,
		typename Allocator = Arena>
	class HashTable
	{
	public:
		struct Slot
		{
			Data data;
			int16_t psl;

			Slot()
				: data{}
				, psl(-1)
			{}

			Slot(const Slot& other)
				: data(other.data)
				, psl(other.psl)
			{}

			Slot(Slot&& other) noexcept
				: data(std::move(other.data))
				, psl(other.psl)
			{
				other.psl = -1;
			}

			template<typename... Args>
			Slot(const int16_t psl, Args&&... args)
				: data(std::forward<Args>(args)...)
				, psl(psl)
			{}

			Slot& operator=(const Slot& other)
			{
				if (this != &other)
				{
					data = other.data;
					psl = other.psl;
				}

				return *this;
			}

			Slot& operator=(Slot&& other) noexcept
			{
				if (this != &other)
				{
					data = std::move(other.data);
					psl = other.psl;

					other.psl = -1;
				}

				return *this;
			}
		};

		template<bool Const, bool Reverse>
		class BaseIterator
		{
		public :
			using ContainerType = std::conditional_t<Const, const HashTable, HashTable>;
			using ElementType = std::conditional_t<Const, const Data, Data>;

			BaseIterator(ContainerType& refTable, const size_t index, const bool begin = false)
				: m_table(&refTable)
				, m_index(index)
			{
				if (begin)
				{
					if constexpr (Reverse)
					{
						m_index = FindLowerIndex(index);
					}
					else
					{
						m_index = FindUpperIndex(index);
					}
				}
			}

			template<bool ConstOther>
			BaseIterator(const BaseIterator<ConstOther, Reverse>& other)
				: m_table(other.m_table)
				, m_index(other.m_index)
			{}

			~BaseIterator() = default;

			BaseIterator& operator++()
			{
				if constexpr (Reverse)
				{
					m_index = FindLowerIndex(m_index - 1);
				}
				else
				{
					m_index = FindUpperIndex(m_index + 1);
				}

				return *this;
			}

			BaseIterator& operator--()
			{
				if constexpr (Reverse)
				{
					m_index = FindUpperIndex(m_index + 1);
				}
				else
				{
					m_index = FindLowerIndex(m_index - 1);
				}

				return *this;
			}

			BaseIterator operator++(int)
			{
				BaseIterator itr = *this;
				++(*this);
				return itr;
			}

			BaseIterator operator--(int)
			{
				BaseIterator itr = *this;
				--(*this);
				return itr;
			}

			bool operator==(const BaseIterator& other) const
			{
				return m_table == other.m_table && m_index == other.m_index;
			}

			bool operator!=(const BaseIterator& other) const
			{
				return !(*this == other);
			}

			ElementType* operator->() const
			{
				return &(**this);
			}

			ElementType& operator*() const
			{
				if constexpr (Reverse)
				{
					assert(m_index > 0 && "Invalid the static array's reverse iterator's index is end");
					return m_table->m_slotList[m_index - 1].data;
				}
				else
				{
					assert(m_index < m_table->MaxSize() && "Invalid the static array's iterator's index is end");
					return m_table->m_slotList[m_index].data;
				}
			}

		private :
			size_t FindUpperIndex(const size_t startIndex)
			{
				const size_t endIndex = m_table->MaxSize();
				size_t index = startIndex;
				while (true)
				{
					if (index == endIndex)
					{
						break;
					}

					if (m_table->m_slotList[index].psl != -1)
					{
						break;
					}

					index++;
				}

				return index;
			}

			size_t FindLowerIndex(const size_t startIndex)
			{
				const size_t endIndex = m_table->MaxSize();
				size_t index = startIndex;
				while (true)
				{
					if (index == 0)
					{
						break;
					}

					if (index != endIndex && m_table->m_slotList[index].psl != -1)
					{
						break;
					}

					index--;
				}

				return index;
			}

		private:
			template<bool ConstOther, bool ReverseOther>
			friend class BaseIterator;

			friend class HashTable;

			ContainerType* m_table;
			size_t m_index;
		};

		using Iterator = std::conditional_t<std::is_same_v<Key, Data>, BaseIterator<true, false>, BaseIterator<false, false>>;
		using ConstIterator = BaseIterator<true, false>;

		using ReverseIterator = std::conditional_t<std::is_same_v<Key, Data>, BaseIterator<true, true>, BaseIterator<true, false>>;
		using ConstReverseIterator = BaseIterator<true, true>;

	public :
		using KeyType = Key;
		using ValueType = Data;
		using HasherType = Hasher;
		using ComparerType = Comparer;
		using SelectorType = Selector;
		using AllocatorType = Allocator;

		HashTable()
			: m_slotList()
			, m_count(0)
		{}

		HashTable(const HashTable& other)
			: m_slotList(other.m_slotList)
			, m_count(other.m_count)
		{}

		HashTable(HashTable&& other) noexcept
			: m_slotList(std::move(other.m_slotList))
			, m_count(other.m_count)
		{
			other.m_count = 0;
		}

		HashTable(const std::initializer_list<Data>& initList)
			: HashTable()
		{
			for (auto& element : initList)
			{
				Emplace(element);
			}
		}

		virtual ~HashTable() = default;

		HashTable& operator=(const HashTable& other)
		{
			if (this != &other)
			{
				m_slotList = other.m_slotList;
				m_count = other.m_count;
			}

			return *this;
		}

		HashTable& operator=(HashTable&& other)
		{
			if (this != &other)
			{
				m_slotList = std::move(other.m_slotList);
				m_count = other.m_count;

				other.m_count = 0;
			}

			return *this;
		}

	public :
		size_t Size() const
		{
			return m_count;
		}

		size_t MaxSize() const
		{
			return m_slotList.Size();
		}

		bool Empty() const
		{
			return m_count == 0;
		}

		void Reserve(const size_t newCapacity)
		{
			m_slotList.Resize(newCapacity);
		}

		void Rehash(const size_t newSize)
		{
			DynamicArray<Slot, Allocator> newSlotList;
			newSlotList.Resize(newSize);

			for (auto& slot : m_slotList)
			{
				if (slot.psl == -1)
				{
					continue;
				}

				slot.psl = 0;
				auto& key = Selector()(slot.data);
				size_t index = Hasher()(key) % newSlotList.Size();

				while (true)
				{
					auto& oldSlot = newSlotList[index];
					if (oldSlot.psl == -1)
					{
						oldSlot = std::move(slot);
						break;
					}

					if (Selector()(slot.data) == Selector()(oldSlot.data))
					{
						break;
					}

					if (slot.psl > oldSlot.psl)
					{
						std::swap(slot, oldSlot);
					}

					index = (index + 1) % newSlotList.Size();
					slot.psl++;
				}
			}

			m_slotList = std::move(newSlotList);
		}

		void Clear()
		{
			m_slotList.Clear();
			m_count = 0;
		}

	public :
		std::pair<Iterator, bool> Insert(const Data& data)
		{
			const auto& key = Selector()(data);
			auto itr = Find(key);
			if (itr != End())
			{
				return std::make_pair(itr, false);
			}
			else
			{
				return Emplace(data);
			}
		}

		template<typename... Args>
		std::pair<Iterator, bool> TryEmplace(const Key& key, Args&&... args)
		{
			auto itr = Find(key);
			if (itr != End())
			{
				return std::make_pair(itr, false);
			}
			else
			{
				return Emplace(std::forward<Args>(args)...);
			}
		}

		template<typename... Args>
		std::pair<Iterator, bool> Emplace(Args&&... args)
		{
			if (m_count>= MaxSize() * LOAD_FACTOR)
			{
				Rehash(m_count == 0 ? 4 : MaxSize() * 2);
			}

			Slot newSlot(0, std::forward<Args>(args)...);
			size_t index = Hasher()(Selector()(newSlot.data)) % MaxSize();

			size_t firstIndex = MaxSize();
			bool firstEmplaced = false;

			while (true)
			{
				auto& oldSlot = m_slotList[index];
				if (oldSlot.psl == -1)
				{
					oldSlot = std::move(newSlot);
					m_count++;

					firstIndex = firstIndex == MaxSize() ? index : firstIndex;
					firstEmplaced = true;

					break;
				}

				if (!firstEmplaced && Selector()(newSlot.data) == Selector()(oldSlot.data))
				{
					firstIndex = index;

					break;
				}

				if (newSlot.psl > oldSlot.psl)
				{
					std::swap(newSlot, oldSlot);

					firstIndex = firstIndex == MaxSize() ? index : firstIndex;
					firstEmplaced = true;
				}

				index = (index + 1) % MaxSize();
				newSlot.psl++;
			}

			return std::make_pair(Iterator(*this, firstIndex), firstEmplaced);
		}

		Iterator Erase(const Key& key)
		{
			auto itr = Find(key);
			if (itr != End())
			{
				return Erase(itr);
			}
			else
			{
				return End();
			}
		}

		Iterator Erase(ConstIterator pos)
		{
			auto first = pos;
			auto last = pos;
			++last;
			return Erase(first, last);
		}

		Iterator Erase(ConstIterator first, ConstIterator last)
		{
			if (this != first.m_table || this != last.m_table)
			{
				return End();
			}

			if (first == last)
			{
				return Iterator(*this, last.m_index);
			}

			const size_t maxIndex = MaxSize();
			size_t length = (maxIndex + last.m_index - first.m_index) % maxIndex;

			for (size_t offset = 0; offset < length; offset++)
			{
				const size_t index = (offset + first.m_index) % maxIndex;

				auto& oldSlot = m_slotList[index];
				if (oldSlot.psl != -1)
				{
					Slot garbageSlot = std::move(oldSlot);
					m_count--;
				}
			}

			size_t firstIndex = first.m_index % maxIndex;
			size_t lastIndex = last.m_index % maxIndex;

			while (true)
			{
				auto& oldSlot = m_slotList[lastIndex];
				if (oldSlot.psl <= 0)
				{
					break;
				}

				const size_t diffIndex = static_cast<size_t>(oldSlot.psl) > length ? length : static_cast<size_t>(oldSlot.psl);
				const size_t nextIndex = (maxIndex + lastIndex - diffIndex) % maxIndex;

				auto& newSlot = m_slotList[nextIndex];
				newSlot = std::move(oldSlot);
				newSlot.psl -= diffIndex;

				firstIndex = (nextIndex + 1) % maxIndex;
				lastIndex = (lastIndex + 1) % maxIndex;
				length = (maxIndex + lastIndex - firstIndex) % maxIndex;
			}

			return Iterator(*this, first.m_index, true);
		}

		Iterator Find(const Key& key)
		{
			const size_t index = FindIndex(key);
			if (index == MaxSize())
			{
				return End();
			}
			else
			{
				return Iterator(*this, index);
			}
		}

		ConstIterator Find(const Key& key) const
		{
			const size_t index = FindIndex(key);
			if (index == MaxSize())
			{
				return End();
			}
			else
			{
				return ConstIterator(*this, index);
			}
		}

	public :
		// Standard Range Iterator
		Iterator begin() { return Iterator(*this, 0, true); }
		Iterator end() { return Iterator(*this, MaxSize()); }
		ConstIterator begin() const { return ConstIterator(*this, 0, true); }
		ConstIterator end() const { return ConstIterator(*this, MaxSize()); }

		ReverseIterator rbegin() { return ReverseIterator(*this, MaxSize(), true); }
		ReverseIterator rend() { return ReverseIterator(*this, 0); }
		ConstReverseIterator rbegin() const { return ConstReverseIterator(*this, MaxSize(), true); }
		ConstReverseIterator rend() const { return ConstReverseIterator(*this, 0); }

	public:
		Iterator Begin() { return Iterator(*this, 0, true); }
		Iterator End() { return Iterator(*this, MaxSize()); }
		ConstIterator Begin() const { return ConstIterator(*this, 0, true); }
		ConstIterator End() const { return ConstIterator(*this, MaxSize()); }

		ReverseIterator rBegin() { return ReverseIterator(*this, MaxSize(), true); }
		ReverseIterator rEnd() { return ReverseIterator(*this, 0); }
		ConstReverseIterator rBegin() const { return ConstReverseIterator(*this, MaxSize(), true); }
		ConstReverseIterator rEnd() const { return ConstReverseIterator(*this, 0); }

	private :
		size_t FindIndex(const Key& key) const
		{
			const size_t endIndex = MaxSize();

			if (Empty())
			{
				return endIndex;
			}

			size_t index = Hasher()(key) % endIndex;
			size_t psl = 0;

			while (true)
			{
				auto& slot = m_slotList[index];
				if (slot.psl == -1 || psl > slot.psl)
				{
					return endIndex;
				}

				auto& slotKey = Selector()(slot.data);
				if (Comparer()(key, slotKey))
				{
					return index;
				}

				index = (index + 1) % endIndex;
				psl++;

				if (psl > endIndex)
				{
					return endIndex;
				}
			}

			return endIndex;
		}

	private :
		DynamicArray<Slot, Allocator> m_slotList;
		size_t m_count;

		inline static float LOAD_FACTOR = 0.7f;
	};
};
#endif // __WTR_HASHTABLE_H__