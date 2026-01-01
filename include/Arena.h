#ifndef __WTR_ARENA_H__
#define __WTR_ARENA_H__

#include <cstddef>
#include <new>

namespace wtr
{
	class Arena
	{
		private :
			struct Page
			{
				Page* next;
				Page* prev;
				void* data;

				Page()
					: next(nullptr)
					, prev(nullptr)
					, data(nullptr)
				{}

				~Page() = default;
			};

		public :
			Arena()
				: m_end()
			{
				m_end.prev = &m_end;
				m_end.next = &m_end;
			}

			Arena(const Arena& other) = delete;
			Arena(Arena&& other) noexcept
				: Arena()
			{
				if (other.m_end.next != &other.m_end)
				{
					m_end.next = other.m_end.next;
					m_end.prev = other.m_end.prev;
					m_end.next->prev = &m_end;
					m_end.prev->next = &m_end;

					other.m_end.next = &other.m_end;
					other.m_end.prev = &other.m_end;
				}
			}

			~Arena()
			{
				Reset();
			}

			Arena& operator=(const Arena& other) = delete;
			Arena& operator=(Arena&& other) noexcept
			{
				if (this != &other && other.m_end.next != &other.m_end)
				{
					Reset();

					m_end.next = other.m_end.next;
					m_end.prev = other.m_end.prev;
					m_end.next->prev = &m_end;
					m_end.prev->next = &m_end;

					other.m_end.next = &other.m_end;
					other.m_end.prev = &other.m_end;
				}

				return *this;
			}

		public :
			void* Allocate(const size_t memorySize)
			{
				const size_t alignSize = alignof(Page);
				const size_t paddingSize = (alignSize - (memorySize % alignSize)) % alignSize;
				const size_t offsetSize = memorySize + paddingSize;
				const size_t totalSize = offsetSize + sizeof(Page);

				void* memory = ::operator new(totalSize, std::nothrow);
				if (nullptr == memory)
				{
					return nullptr;
				}

				void* pageOffset = static_cast<void*>(static_cast<uint8_t*>(memory) + offsetSize);
				
				Page* newPage = new (pageOffset) Page();
				newPage->data = memory;

				m_end.prev->next = newPage;
				newPage->prev = m_end.prev;
				newPage->next = &m_end;
				m_end.prev = newPage;

				return memory;
			}

			void Deallocate(void* pointer)
			{
				if (nullptr == pointer)
				{
					return;
				}

				Page* now = m_end.next;
				while (nullptr != now && &m_end != now)
				{
					Page* prev = now->prev;
					Page* next = now->next;

					if (pointer == now->data)
					{
						now->~Page();

						::operator delete(pointer);

						prev->next = next;
						next->prev = prev;

						return;
					}

					now = next;
				}
			}

		private :
			void Reset()
			{
				Page* now = m_end.next;
				while (nullptr != now && &m_end != now)
				{
					Page* next = now->next;

					if (nullptr != now->data)
					{
						now->~Page();
						::operator delete(now->data);
					}

					now = next;
				}
			}

		private :
			Page m_end;
	};
};

#endif // __WTR_ARENA_H__