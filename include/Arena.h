#ifndef __WTR_ARENA_H__
#define __WTR_ARENA_H__

#include <cstddef>
#include <new>

namespace wtr
{
	class Arena
	{
		public :
			Arena() = default;
			~Arena() = default;

		public :
			void* Allocate(const size_t memroySize)
			{
				void* memory = ::operator new(memroySize, std::nothrow);

				return memory;
			}

			void Deallocate(void* pointer)
			{
				if (nullptr == pointer)
				{
					return;
				}

				::operator delete(pointer);
			}
	};
};

#endif // __WTR_ARENA_H__