#ifndef __WTR_TYPETRAITS_H__
#define __WTR_TYPETRAITS_H__

#include <cstddef>
#include <new>

namespace wtr
{
	template<typename...> 
	using Void_t = void;

	template<bool Condition, typename T>
	struct EnableIf;

	template<typename T>
	struct EnableIf<true, T>
	{
		using Type = T;
	};

	template<typename T, typename U>
	struct IsSame
	{
		static const bool Value = false;
	};

	template<typename T>
	struct IsSame<T, T>
	{
		static const bool Value = true;
	};

	template<typename T>
	T&& Declval() noexcept
	{
		return static_cast<T&&>(*(T*)nullptr);
	}

	template<typename... Ts>
	struct TypeList {};
};

#endif // __WTR_TYPETRAITS_H__