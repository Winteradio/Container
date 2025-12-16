#ifndef __WTR_DYNAMIC_ARRAY_H__
#define __WTR_DYNAMIC_ARRAY_H__

#include <cstddef>
#include <cassert>
#include <utility>
#include <initializer_list>
#include <type_traits>
#include <functional>
#include <queue>

#include "Arena.h"

namespace wtr
{
	template<typename T, typename Allocator = Arena>
	class DynamicArray
	{
	public :
		template<bool Const, bool Reverse>
		class BaseIterator
		{
		public :
			using ContainerType = std::conditional_t<Const, const DynamicArray, DynamicArray>;
			using ValueType = std::conditional_t<Const, const T, T>;

			BaseIterator(ContainerType& refArray, const size_t index)
				: m_array(&refArray)
				, m_index(index)
			{}

			template<bool ConstOther>
			BaseIterator(const BaseIterator<ConstOther, Reverse>& other)
				: m_array(other.m_array)
				, m_index(other.m_index)
			{}

			template<bool ReverseOther>
			BaseIterator(const BaseIterator<Const, ReverseOther>& other)
				: m_array(other.m_array)
				, m_index()
			{
				if constexpr (!Reverse && ReverseOther)
				{
					m_index = other.m_index - 1;
				}
				else if constexpr (Reverse && !ReverseOther)
				{
					m_index = other.m_index + 1;
				}
				else
				{
					m_index = other.m_index;
				}
			}

			~BaseIterator() = default;

			BaseIterator& operator++()
			{
				if constexpr (Reverse)
				{
					assert(m_index > 0 && "Invalid the dynamic array's reverse iterator's prefix increment");

					m_index--;
				}
				else
				{
					assert(m_index < m_array->Size() && "Invalid the dynamic array's iterator's prefix increment");

					m_index++;
				}

				return *this;
			}

			BaseIterator& operator--()
			{
				if constexpr (Reverse)
				{
					assert(m_index < m_array->Size() && "Invalid the dynamic array's reverse iterator's prefix decrement");

					m_index++;
				}
				else
				{
					assert(m_index > 0 && "Invalid the dynamic array's iterator's prefix decrement");

					m_index--;
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
				return m_array == other.m_array && m_index == other.m_index;
			}

			bool operator!=(const BaseIterator& other) const
			{
				return !(*this == other);
			}

			ValueType* operator->() const
			{
				return &(**this);
			}

			ValueType& operator*() const
			{
				if constexpr (Reverse)
				{
					assert(m_index > 0 && "Invalid the static array's reverse iterator's index is end");
					return (*m_array)[m_index - 1];
				}
				else
				{
					assert(m_index < m_array->Size() && "Invalid the static array's iterator's index is end");
					return (*m_array)[m_index];
				}
			}

		private:
			template<bool ConstOther, bool ReverseOther>
			friend class BaseIterator;

			friend class DynamicArray;

			ContainerType* m_array;
			size_t m_index;
		};

		using Iterator = BaseIterator<false, false>;
		using ConstIterator = BaseIterator<true, false>;

		using ReverseIterator = BaseIterator<false, true>;
		using ConstReverseIterator = BaseIterator<true, true>;

	public :
		using ValueType = T;
		using AllocatorType = Allocator;

		public :
			DynamicArray()
				: m_data(nullptr)
				, m_size(0)
				, m_capacity(0)
				, m_allocator()
			{}

			DynamicArray(const std::initializer_list<T>& initList)
				: DynamicArray()
			{
				const size_t newCapacity = initList.size();
				Reserve(newCapacity);

				for (auto& element : initList)
				{
					EmplaceBack(element);
				}
			}

			DynamicArray(const DynamicArray& other)
			{
				m_size = other.m_size;
				m_capacity = other.m_size;

				m_data = static_cast<T*>(m_allocator.Allocate(sizeof(T) * m_capacity));

				for (size_t index = 0; index < m_size; index++)
				{
					new (m_data + index) T(other[index]);
				}
			}

			DynamicArray(DynamicArray&& other) noexcept
				: m_data(std::move(other.m_data))
				, m_size(std::move(other.m_size))
				, m_capacity(std::move(other.m_capacity))
			{
				other.m_data = nullptr;
				other.m_size = 0;
				other.m_capacity = 0;
			}

			~DynamicArray()
			{
				Clear();
				m_allocator.Deallocate(m_data);
			}

			DynamicArray& operator=(const DynamicArray& other)
			{
				if (this != &other)
				{
					Clear();

					if (m_capacity < other.m_size)
					{
						m_allocator.Deallocate(m_data);
						m_data = static_cast<T*>(m_allocator.Allocate(sizeof(T) * other.m_size));
					}

					m_size = other.m_size;
					m_capacity = other.m_size;

					for (size_t index = 0; index < other.m_size; index++)
					{
						new (m_data + index) T(other[index]);
					}
				}

				return *this;
			}

			DynamicArray& operator=(DynamicArray&& other)
			{
				if (this != &other)
				{
					Clear();

					if (nullptr != m_data)
					{
						m_allocator.Deallocate(m_data);
					}

					m_data = std::move(other.m_data);
					m_size = std::move(other.m_size);
					m_capacity = std::move(other.m_capacity);

					other.m_data = nullptr;
					other.m_size = 0;
					other.m_capacity = 0;
				}

				return *this;
			}

			bool operator==(const DynamicArray& other) const
			{
				if (m_size != other.m_size)
				{
					return false;
				}

				for (size_t index = 0; index < m_size; index++)
				{
					if (m_data[index] != other.m_data[index])
					{
						return false;
					}
				}

				return true;
			}

			bool operator!=(const DynamicArray& other) const
			{
				return !(*this == other);
			}

			T& operator[](const size_t index)
			{
				assert(index < m_size && "Index out of bounds");

				return m_data[index];
			}

			const T& operator[](const size_t index) const
			{
				assert(index < m_size && "Index out of bounds");

				return m_data[index];
			}

	public :
		T& At(const size_t index)
		{
			assert(index < m_size && "The array is empty, failed to get data");
			
			return m_data[index];
		}

		const T& At(const size_t index) const
		{
			assert(index < m_size && "The array is empty, failed to get data");

			return m_data[index];
		}

		T* Data()
		{
			return m_data;
		}

		const T* Data() const
		{
			return m_data;
		}

		T& Front()
		{
			assert(0 < m_size && "The array is empty, failed to get the front data");

			return m_data[0];
		}

		T& Back()
		{
			assert(0 < m_size && "The array is empty, failed to get the back data");

			return m_data[m_size - 1];
		}

		const T& Front() const
		{
			assert(0 < m_size && "The array is empty, failed to get the front data");

			return m_data[0];
		}

		const T& Back() const
		{
			assert(0 < m_size && "The array is empty, failed to get the back data");

			return m_data[m_size - 1];
		}

	public :
		void PushBack(const T& data)
		{
			EmplaceBack(data);
		}

		void PopBack()
		{
			if (0 != m_size && nullptr != m_data)
			{
				T* instance = m_data + (m_size - 1);
				instance->~T();

				m_size--;
			}
		}

		template<typename... Args>
		void EmplaceBack(Args&&... args)
		{
			if (m_size == m_capacity)
			{
				const size_t newCapacity = (0 == m_capacity) ? 4 : m_capacity * 2;
				Reserve(newCapacity);
			}

			new (m_data + m_size) T(std::forward<Args>(args)...);
			m_size++;
		}

		bool Empty() const
		{
			return m_size == 0;
		}
		
		size_t Size() const
		{
			return m_size;
		}

		size_t MaxSize() const
		{
			return m_capacity;
		}

		size_t Capacity() const
		{
			return m_capacity;
		}

		void Resize(const size_t newSize)
		{
			if (newSize > m_size)
			{
				if (newSize > m_capacity)
				{
					Reserve(newSize);
				}

				for (size_t index = m_size; index < newSize; index++)
				{
					T* instance = m_data + index;
					if (nullptr != instance)
					{
						new (m_data + index) T{};
					}
				}
			}
			else
			{
				for (size_t index = newSize; index < m_size; index++)
				{
					T* instance = m_data + index;
					if (nullptr != instance)
					{
						instance->~T();
					}
				}
			}


			m_size = newSize;
		}

		void Reserve(const size_t newCapacity)
		{
			if (newCapacity > m_capacity)
			{
				T* newData = static_cast<T*>(m_allocator.Allocate(sizeof(T) * newCapacity));

				for (size_t index = 0; index < m_size; index++)
				{
					new (newData + index) T(std::move(m_data[index]));

					m_data[index].~T();
				}

				m_allocator.Deallocate(m_data);
				m_capacity = newCapacity;
				m_data = newData;
			}
		}

		void Clear()
		{
			for (size_t index = 0; index < m_size; index++)
			{
				T* instance = m_data + index;

				if (nullptr != instance)
				{
					instance->~T();
				}
			}

			m_size = 0;
		}

		void Assign(const std::initializer_list<T>& initList)
		{
			Clear();

			const size_t newCapacity = initList.size();
			Reserve(newCapacity);

			for (auto& element : initList)
			{
				EmplaceBack(element);
			}
		}
		
	public :
		Iterator Insert(ConstIterator pos, T&& value)
		{
			const size_t newSize = m_size + 1;
			Resize(newSize);

			for (size_t index = m_size - 1; index > pos.m_index; index--)
			{
				m_data[index] = std::move(m_data[index - 1]);
			}

			m_data[pos.m_index] = std::move(value);

			return Iterator(*this, pos.m_index);
		}

		Iterator Insert(ConstIterator pos, Iterator first, Iterator last)
		{
			if (first == last)
			{
				return Iterator(*this, pos.m_index);
			}

			const size_t diffIndex = (last.m_index - first.m_index);
			const size_t newSize = diffIndex + m_size;
			Resize(newSize);

			for (size_t index = m_size - 1; index > pos.m_index + diffIndex; index--)
			{
				m_data[index] = std::move(m_data[index - diffIndex]);
			}

			for (auto itr = first; itr != last; itr++)
			{
				const size_t index = pos.m_index + itr.m_index - first.m_index;
				m_data[index] = *itr;
			}

			return Iterator(*this, pos.m_index);
		}

		Iterator Insert(ConstIterator pos, const std::initializer_list<T>& initList)
		{
			const size_t offset = initList.size();
			const size_t newSize = m_size + offset;
			Resize(newSize);

			for (size_t index = m_size - 1; index > pos.m_index + offset - 1; index--)
			{
				m_data[index] = std::move(m_data[index - offset]);
			}

			for (size_t index = 0; index < offset; index++)
			{
				m_data[index + pos.m_index] = initList[index];
			}

			return Iterator(*this, pos.m_index);
		}

		Iterator Erase(Iterator pos)
		{
			auto first = pos;
			auto last = pos;
			++last;
			return Erase(first, last);
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
			if (first == End() || first == last)
			{
				return End();
			}

			if (this != first.m_array || this != last.m_array)
			{
				return End();
			}

			const size_t length = last.m_index - first.m_index;
			for (size_t index = first.m_index; index < m_size - length; index++)
			{
				m_data[index] = std::move(m_data[index + length]);
			}

			for (size_t index = m_size - length; index < m_size; index++)
			{
				m_data[index].~T();
			}

			m_size -= length;

			return Iterator(*this, first.m_index);
		}

		Iterator Find(const ValueType& other)
		{
			auto itr = Begin();
			while (itr != End())
			{
				const auto& value = *itr;

				if (value == other)
				{
					return itr;
				}
				else
				{
					itr++;
				}
			}

			return End();
		}

		void Sort(ConstIterator first, ConstIterator last, const std::function<bool(const ValueType&, const ValueType)>& func)
		{
			if (first == last || Empty())
			{
				return;
			}

			const size_t front = first.m_index < last.m_index ? first.m_index : last.m_index;
			const size_t back = first.m_index < last.m_index ? last.m_index - 1 : first.m_index;
			if (front == back)
			{
				return;
			}

			struct Pivot
			{
				size_t pivot;
				size_t lower;
				size_t upper;
			};

			Pivot start;
			start.lower = front;
			start.upper = back;
			start.pivot = (start.lower + start.upper) / 2;

			std::queue<Pivot> pivotQueue;
			pivotQueue.push(start);

			while (!pivotQueue.empty())
			{
				Pivot pivot = pivotQueue.front();
				pivotQueue.pop();

				const size_t lower = pivot.lower;
				const size_t upper = pivot.upper;

				while (pivot.lower <= pivot.upper)
				{
					bool lowerBigger = !func(m_data[pivot.lower], m_data[pivot.pivot]);
					bool upperSmaller = !func(m_data[pivot.pivot], m_data[pivot.upper]);

					if (lowerBigger && upperSmaller)
					{
						ValueType value = m_data[pivot.lower];
						m_data[pivot.lower] = m_data[pivot.upper];
						m_data[pivot.upper] = value;
					}

					if (pivot.lower == pivot.upper)
					{
						pivot.lower = (pivot.lower < upper) ? pivot.lower + 1 : pivot.lower;
					}
					else
					{
						pivot.lower = (!lowerBigger || upperSmaller) && (pivot.lower < upper) ? pivot.lower + 1 : pivot.lower;
						pivot.upper = (!upperSmaller || lowerBigger) && (pivot.upper > lower) ? pivot.upper - 1 : pivot.upper;
					}
				}

				if (lower < pivot.upper)
				{
					Pivot left;
					left.lower = lower;
					left.upper = pivot.upper;
					left.pivot = (left.lower + left.upper) / 2;

					pivotQueue.push(left);
				}

				if (upper > pivot.lower)
				{
					Pivot right;
					right.lower = pivot.lower;
					right.upper = upper;
					right.pivot = (right.lower + right.upper) / 2;

					pivotQueue.push(right);
				}
			}
		}

		ConstIterator Find(const ValueType& other) const
		{
			auto itr = Begin();
			while (itr != End())
			{
				const auto& value = *itr;

				if (value == other)
				{
					return itr;
				}
				else
				{
					itr++;
				}
			}

			return End();
		}

		size_t Distance(ConstIterator first, ConstIterator last) const
		{
			return last.m_index - first.m_index;
		}

	public :
		// Standard Range Iterator
		Iterator begin() { return Iterator(*this, 0); }
		Iterator end() { return Iterator(*this, m_size); }
		ConstIterator begin() const	{ return ConstIterator(*this, 0); }
		ConstIterator end() const {	return ConstIterator(*this, m_size); }

		ReverseIterator rbegin() { return ReverseIterator(*this, m_size); }
		ReverseIterator rend() { return ReverseIterator(*this, 0); }
		ConstReverseIterator rbegin() const { return ConstReverseIterator(*this, m_size); }
		ConstReverseIterator rend() const {	return ConstReverseIterator(*this, 0); }

	public:
		Iterator Begin() { return Iterator(*this, 0); }
		Iterator End() { return Iterator(*this, m_size); }
		ConstIterator Begin() const { return ConstIterator(*this, 0); }
		ConstIterator End() const { return ConstIterator(*this, m_size); }

		ReverseIterator rBegin() { return ReverseIterator(*this, m_size); }
		ReverseIterator rEnd() { return ReverseIterator(*this, 0); }
		ConstReverseIterator rBegin() const { return ConstReverseIterator(*this, m_size); }
		ConstReverseIterator rEnd() const { return ConstReverseIterator(*this, 0); }

	private :
		T* m_data;
		size_t m_size;
		size_t m_capacity;
		AllocatorType m_allocator;
	};
};

#endif // __WTR_DYNAMIC_ARRAY_H__