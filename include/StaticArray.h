#ifndef __WTR_STATIC_ARRAY_H__
#define __WTR_STATIC_ARRAY_H__

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace wtr
{
	template<typename T, size_t Count>
	class StaticArray
	{
		static_assert(Count > 0, "The static array's size must be greater than 0");

	public :
		template<bool Const, bool Reverse>
		class BaseIterator
		{
		public :
			using ContainerType = std::conditional_t<Const, const StaticArray, StaticArray>;
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

			~BaseIterator() = default;

			BaseIterator& operator++()
			{
				if constexpr (Reverse)
				{
					assert(m_index > 0 && "Invalid the static array's reverse iterator's prefix increment");

					m_index--;
				}
				else
				{
					assert(m_index < m_array->Size() && "Invalid the static array's iterator's prefix increment");

					m_index++;
				}

				return *this;
			}
			
			BaseIterator& operator--()
			{
				if constexpr (Reverse)
				{
					assert(m_index < m_array->Size() && "Invalid the static array's reverse iterator's prefix decrement");

					m_index++;
				}
				else
				{
					assert(m_index > 0 && "Invalid the static array's iterator's prefix decrement");

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

		private :
			template<bool ConstOther, bool ReverseOther>
			friend class BaseIterator;

			ContainerType* m_array;
			size_t m_index;
		};

		using Iterator = BaseIterator<false, false>;
		using ConstIterator = BaseIterator<true, false>;

		using ReverseIterator = BaseIterator<false, true>;
		using ConstReverseIterator = BaseIterator<true, true>;

	public	:
		using ValueType = T;

		StaticArray() = default;
		StaticArray(const std::initializer_list<T>& initList)
		{
			assert(initList.size() <= Count && "The initializer list is over than max count");

			size_t index = 0;
			for (auto& element : initList)
			{
				m_data[index] = element;
				index++;
			}

			for (; index < Count; index++)
			{
				m_data[index] = T{};
			}

		}

		StaticArray(const StaticArray& other)
		{
			for (size_t index = 0; index < Count; index++)
			{
				m_data[index] = other[index];
			}
		}

		StaticArray(StaticArray&& other) noexcept
		{
			for (size_t index = 0; index < Count; index++)
			{
				m_data[index] = std::move(other[index]);
			}
		}

		~StaticArray() = default;

		StaticArray& operator=(const StaticArray& other)
		{
			if (this != &other)
			{
				for (size_t index = 0; index < Count; index++)
				{
					m_data[index] = other[index];
				}
			}

			return *this;
		}

		StaticArray& operator=(StaticArray&& other) noexcept
		{
			for (size_t index = 0; index < Count; index++)
			{
				m_data[index] = std::move(other[index]);
			}

			return *this;
		}

		bool operator==(const StaticArray& other) const
		{
			for (size_t index = 0; index < Count; index++)
			{
				if (m_data[index] != other.m_data[index])
				{
					return false;
				}
			}

			return true;
		}

		bool operator!=(const StaticArray& other) const
		{
			return !(*this == other);
		}

		T& operator[](const size_t index)
		{
			assert(index < Count && "Index out of bounds");

			return m_data[index];
		}

		const T& operator[](const size_t index) const
		{
			assert(index < Count && "Index out of bounds");

			return m_data[index];
		}

	public :
		T& Front()
		{
			return m_data[0];
		}

		T& Back()
		{
			assert(0 < Count && "The array is empty, failed to get the end data");

			return m_data[Count - 1];
		}

		const T& Front() const
		{
			return m_data[0];
		}

		const T& Back() const
		{
			assert(0 < Count && "The array is empty, failed to get the end data");

			return m_data[Count - 1];
		}

	public :
		T& At(const size_t index)
		{
			assert(index < Count && "The index is over than array's max count");

			return m_data[index];
		}

		const T& At(const size_t index) const
		{
			assert(index < Count && "The index is over than array's max count");

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

		constexpr size_t Size() const
		{
			return Count;
		}

		template<typename... Args>
		void Fill(Args&&... args)
		{
			for (size_t index = 0; index < Count; index++)
			{
				m_data[index] = T(std::forward<Args>(args)...);
			}
		}

	public :
		// Standard Range Iterator
		Iterator begin() { return Iterator(*this, 0); }
		Iterator end() { return Iterator(*this, Count);	}
		ConstIterator begin() const { return ConstIterator(*this, 0); }
		ConstIterator end() const {	return ConstIterator(*this, Count);	}

		ReverseIterator rbegin() { return ReverseIterator(*this, Count); }
		ReverseIterator rend() { return ReverseIterator(*this, 0); }
		ConstReverseIterator rbegin() const { return ConstReverseIterator(*this, Count); }
		ConstReverseIterator rend() const { return ConstReverseIterator(*this, 0); }

	public :
		Iterator Begin() { return Iterator(*this, 0); }
		Iterator End() { return Iterator(*this, Count); }
		ConstIterator Begin() const { return ConstIterator(*this, 0); }
		ConstIterator End() const { return ConstIterator(*this, Count);	}

		ReverseIterator rBegin() { return ReverseIterator(*this, Count); }
		ReverseIterator rEnd() { return ReverseIterator(*this, 0); }
		ConstReverseIterator rBegin() const { return ConstReverseIterator(*this, Count); }
		ConstReverseIterator rEnd() const { return ConstReverseIterator(*this, 0); }

	private :
		T m_data[Count];
	};
};

#endif // __WTR_STATIC_ARRAY_H__