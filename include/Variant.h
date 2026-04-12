#ifndef __WTR_VARIANT_H__
#define __WTR_VARIANT_H__

#ifdef _DEBUG
	#include <cassert>
	#define _VARIANT_ASSERT(x) assert(x)
#else
	#define _VARIANT_ASSERT(x)
#endif

#include "TypeTraits.h"

namespace std
{
	template<typename T>
	struct hash;
};

namespace wtr
{
	namespace util
	{
		template<typename... Args>
		struct SizeCalculator 
		{
			static constexpr size_t value = 0;
		};
	
		template<typename T>
		struct SizeCalculator<T>
		{
			static constexpr size_t value = sizeof(T);
		};
	
		template<typename T, typename... Args>
		struct SizeCalculator<T, Args...>
		{
			static constexpr size_t value = sizeof(T) + SizeCalculator<Args...>::value;
		};
	
		template<typename... Args>
		constexpr size_t SizeOf()
		{
			return SizeCalculator<Args...>::value;
		}
	
		template<typename... Args>
		struct AlignCalculator 
		{
			static constexpr size_t value = 0;
		};
	
		template<typename T>
		struct AlignCalculator<T>
		{
			static constexpr size_t value = alignof(T);
		};
	
		template<typename T, typename... Args>
		struct AlignCalculator<T, Args...>
		{
			static constexpr size_t value = (alignof(T) > AlignCalculator<Args...>::value) ? alignof(T) : AlignCalculator<Args...>::value;
		};
	
		template<typename... Args>
		constexpr size_t AlignOf() 
		{
			return AlignCalculator<Args...>::value;
		};
	
		template<size_t Size, size_t Align>
		struct AlignedStorage
		{
			bool valid;
			alignas(Align) char data[Size];

			AlignedStorage()
				: valid(false)
			{}
		};
	
		template<typename... Args>
		struct IndexCalculator
		{
			static constexpr bool found = false;
			static constexpr int value = -1;
		};
	
		template<typename T, typename U, typename... Args>
		struct IndexCalculator<T, U, Args...>
		{
			static constexpr bool found = IsSame<T, U>::Value || IndexCalculator<T, Args...>::found;
			static constexpr int value = (found) ? 1 + IndexCalculator<T, Args...>::value : -1;
		};
	
		template<typename T, typename... Args>
		constexpr int IndexOf()
		{
			return IndexCalculator<T, Args...>::value;
		}

		template<typename Storage, typename TypeList>
		struct TypeMatcher
		{
			static void Move(Storage& _lhs, const Storage& _rhs, const int _index)
			{
				_VARIANT_ASSERT(false && "Failed to move matcher");				
			}

			static void Copy(Storage& _lhs, const Storage& _rhs, const int _index)
			{
				_VARIANT_ASSERT(false && "Failed to copy matcher");				
			}

			static void Destroy(Storage& _storage, const int _index) 
			{
				_VARIANT_ASSERT(false && "Failed to destroy matcher");
			};
		};

		template<typename Storage, typename T, typename... Args>
		struct TypeMatcher<Storage, TypeList<T, Args...>>
		{
			static void Move(Storage& _lhs, Storage&& _rhs, const int _index)
			{
				if (_index == 0)
				{
					T& lhsValue = *reinterpret_cast<T*>(&_lhs.data);
					T&& rhsValue = std::move(*reinterpret_cast<T*>(&_rhs.data));

					if (!_lhs.valid)
					{
						new (&_lhs.data) T(rhsValue);
					}
					else
					{
						lhsValue = rhsValue;
						_lhs.valid = _rhs.valid;
					}
				}
				else
				{
					TypeMatcher<Storage, TypeList<Args...>>::Move(_lhs, std::move(_rhs), _index - 1);
				}
			}

			static void Copy(Storage& _lhs, const Storage& _rhs, const int _index)
			{
				if (_index == 0)
				{
					T& lhsValue = *reinterpret_cast<T*>(&_lhs.data);
					const T& rhsValue = *reinterpret_cast<const T*>(&_rhs.data);

					if (!_lhs.valid)
					{
						new (&_lhs.data) T(rhsValue);
					}
					else
					{
						lhsValue = rhsValue;
						_lhs.valid = _rhs.valid;
					}
				}
				else
				{
					TypeMatcher<Storage, TypeList<Args...>>::Copy(_lhs, _rhs, _index - 1);
				}
			}

			static void Destroy(Storage& _storage, const int _index)
			{
				if (_index == 0)
				{
					const T& refValue = *reinterpret_cast<T*>(&_storage.data);
					refValue.~T();

					_storage.valid = false;
				}
				else
				{
					TypeMatcher<Storage, TypeList<Args...>>::Destroy(_storage, _index - 1);
				}
			}
		};
		
		template<typename T, typename = void>
		struct HashMatcher {
			static size_t GetHash(const T&) {
				return 0;
			}
		};
		
		template<typename T>
		struct HashMatcher<T, Void_t<decltype(std::hash<T>{}(Declval<T>()))>> {
			static size_t GetHash(const T& _value) {
				return std::hash<T>{}(_value);
			}
		};
	};

	template<typename... Args>
	class Variant
	{
		private :
			static constexpr size_t g_MaxSize = util::SizeOf<Args...>();
			static constexpr size_t g_MaxAlign = util::AlignOf<Args...>();

			using Types = TypeList<Args...>;
			using Storage = util::AlignedStorage<g_MaxSize, g_MaxAlign>;

		public :
			Variant() 
				: m_nCurrentIndex(-1)
				, m_nCurrentHash(0)
			{}

			virtual ~Variant()
			{
				Destroy();
			}

			Variant(const Variant& _other)
				: m_nCurrentIndex(-1)
			{
				Copy(_other);
			}

			Variant(Variant&& _other)
				: m_nCurrentIndex(-1)
			{
				Move(std::move(_other));
			}

			template<typename T, typename = typename EnableIf<!IsSame<T, Variant>::Value>>
			Variant(const T& _value)
				: m_nCurrentIndex(-1)
			{
				Set(_value);
			}

			template<typename T, typename = typename EnableIf<!IsSame<T, Variant>::Value>>
			Variant(T&& _value)
				: m_nCurrentIndex(-1)
			{
				Set(std::move(_value));
			}

			Variant& operator=(const Variant& _other)
			{
				if (this != &_other)
				{
					Copy(_other);
				}

				return *this;
			}

			Variant& operator=(Variant&& _other)
			{
				if (this != &_other)
				{
					Move(std::move(_other));
				}

				return *this;
			}

			template<typename T, typename = typename EnableIf<!IsSame<T, Variant>::Value>>
			Variant& operator=(const T& _value)
			{
				Set(_value);

				return *this;
			}

			template<typename T, typename = typename EnableIf<!IsSame<T, Variant>::Value>>
			Variant& operator=(T&& _value)
			{
				Set(std::move(_value));

				return *this;
			}

			bool operator==(const Variant& _other) const
			{
				if (this != &_other)
				{
					if (m_nCurrentIndex == _other.m_nCurrentIndex)
					{
						return m_nCurrentHash == _other.m_nCurrentHash;
					}
				}

				return false;
			}

		public:
			template<typename T>
			void Set(const T& _value)
			{
				constexpr int index = util::IndexOf<T, Args...>();
				if (index != -1)
				{
					Destroy();

					m_nCurrentIndex = index;
					m_nCurrentHash = util::HashMatcher<T>::GetHash(_value);
					
					new (&m_tStorage.data) T(_value);
				}
				else
				{
					_VARIANT_ASSERT(false && "Failed to set value in Variant");
				}
			}

			template<typename T>
			void Set(T&& _value)
			{
				constexpr int index = util::IndexOf<T, Args...>();
				if (index != -1)
				{
					Destroy();

					m_nCurrentIndex = index;
					m_nCurrentHash = util::HashMatcher<T>::GetHash(_value);

					new (&m_tStorage.data) T(std::move(_value));
				}
				else
				{
					_VARIANT_ASSERT(false && "Failed to set value in Variant");
				}
			}

			template<typename T>
			const T& Get() const
			{
				if (util::IndexOf<T, Args...>() == m_nCurrentIndex)
				{
					return *reinterpret_cast<const T*>(&m_tStorage.data);
				}
				else
				{
					_VARIANT_ASSERT(false && "Failed to get value from Variant");

					static T emptyValue;
					return emptyValue;
				}
			}

			const int GetIndex() const
			{
				return m_nCurrentIndex;
			}

			const size_t GetHash() const
			{
				return m_nCurrentHash;
			}

			template<typename T>
			const bool Is() const
			{
				return util::IndexOf<T, Args...>() == m_nCurrentIndex;
			}

			void Emplace(const Variant& _other)
			{
				if (this != &_other)
				{
					this->Copy(_other);
				}
			}

			void Emplace(Variant&& _other)
			{
				if (this != &_other)
				{
					this->Move(std::move(_other));
				}
			}

		private :
			void Destroy()
			{
				if (m_nCurrentIndex != -1)
				{
					util::TypeMatcher<Storage, Types>::Destroy(m_tStorage, m_nCurrentIndex);
					m_nCurrentIndex = -1;
					m_nCurrentHash = 0;
				}
			}

			void Copy(const Variant& _other)
			{
				if (_other.m_nCurrentIndex != -1)
				{
					this->m_nCurrentIndex = _other.m_nCurrentIndex;
					this->m_nCurrentHash = _other.m_nCurrentHash;
					util::TypeMatcher<Storage, Types>::Copy(m_tStorage, _other.m_tStorage, m_nCurrentIndex);
				}
			}

			void Move(Variant&& _other)
			{
				if (_other.m_nCurrentIndex != -1)
				{
					this->m_nCurrentIndex = _other.m_nCurrentIndex;
					this->m_nCurrentHash = _other.m_nCurrentHash;
					util::TypeMatcher<Storage, Types>::Move(m_tStorage, std::move(_other.m_tStorage), m_nCurrentIndex);
				}
			}

		private :
			int m_nCurrentIndex;
			size_t m_nCurrentHash;
			Storage m_tStorage;
	};
};

namespace std
{
	template<typename... Args>
	struct hash<wtr::Variant<Args...>>
	{
		size_t operator()(const wtr::Variant<Args...>& _variant) const
		{
			return _variant.GetHash();
		}
	};
};

#endif // __WTR_VARIANT_H__