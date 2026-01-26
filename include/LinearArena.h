#ifndef __WTR_LINEARARENA_H__
#define __WTR_LINEARARENA_H__

#include <cstddef>
#include <new>

#define MAX(x, y) (x >= y) ? x : y;
#define MIN(x, y) (x <= y) ? x : y;

namespace wtr
{
	class LinearArena
	{
	private:
		struct Page
		{
			Page* next;
			Page* prev;
			uint8_t* data;

			size_t offset;
			size_t size;

			Page()
				: next(nullptr)
				, prev(nullptr)
				, data(nullptr)
				, offset(0)
				, size(0)
			{
			}

			~Page()
			{
				if (nullptr != data)
				{
					delete data;
				}
			}

			bool isFull() const
			{
				return offset == size;
			}

			static constexpr size_t MIN_SIZE = 64 * 1024;
		};

	public:
		LinearArena()
			: m_end()
		{
			m_end.prev = &m_end;
			m_end.next = &m_end;
		}

		LinearArena(const LinearArena& other) = delete;
		LinearArena(LinearArena&& other) noexcept
			: LinearArena()
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

		~LinearArena()
		{
			Release();
		}

		LinearArena& operator=(const LinearArena& other) = delete;
		LinearArena& operator=(LinearArena&& other) noexcept
		{
			if (this != &other && other.m_end.next != &other.m_end)
			{
				Release();

				m_end.next = other.m_end.next;
				m_end.prev = other.m_end.prev;
				m_end.next->prev = &m_end;
				m_end.prev->next = &m_end;

				other.m_end.next = &other.m_end;
				other.m_end.prev = &other.m_end;
			}

			return *this;
		}

	public:
		template<typename T>
		void* Allocate()
		{
			const size_t objectSize = sizeof(T);
			const size_t alignSize = alignof(T);

			return Allocate(objectSize, alignSize);
		}

		void* Allocate(const size_t objectSize, const size_t alignSize)
		{
			Page* page = m_end.prev;

			size_t paddingSize = (alignSize - (page->offset % alignSize)) % alignSize;
			size_t totalSize = objectSize + paddingSize;

			if (page == &m_end || page->isFull() || page->offset + totalSize >= page->size)
			{
				const size_t pageSize = MAX(objectSize, Page::MIN_SIZE);
				const size_t pageAlignSize = alignof(Page);
				const size_t pagePaddingSize = (pageAlignSize - (pageSize % pageAlignSize)) % pageAlignSize;
				const size_t pageOffsetSize = pageSize + pagePaddingSize;
				const size_t totalPageSize = pageOffsetSize + sizeof(Page);

				void* memory = ::operator new(totalPageSize, std::nothrow);
				if (nullptr == memory)
				{
					return nullptr;
				}

				void* pageStart = static_cast<void*>(static_cast<uint8_t*>(memory) + pageOffsetSize);

				Page* newPage = new (pageStart) Page();
				newPage->data = static_cast<uint8_t*>(memory);
				newPage->offset = 0;
				newPage->size = pageSize + pagePaddingSize;

				m_end.prev->next = newPage;
				newPage->prev = m_end.prev;
				newPage->next = &m_end;
				m_end.prev = newPage;

				page = newPage;
			}

			paddingSize = (alignSize - (page->offset % alignSize)) % alignSize;
			totalSize = objectSize + paddingSize;

			void* memoryStart = static_cast<void*>(static_cast<uint8_t*>(page->data) + page->offset + paddingSize);

			page->offset += totalSize;

			return memoryStart;
		}

		void Reset()
		{
			Page* now = m_end.next;
			while (nullptr != now && &m_end != now)
			{
				Page* next = now->next;
				now->offset = 0;
				now = next;
			}
		}

	private :
		void Release()
		{
			Page* now = m_end.next;
			while (nullptr != now && &m_end != now)
			{
				Page* next = now->next;

				if (nullptr != now->data)
				{
					now->~Page();
				}

				now = next;
			}
		}

	private:
		Page m_end;
	};
};

#endif // __WTR_LINEARARENA_H__