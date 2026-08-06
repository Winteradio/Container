#ifndef __WTR_TLSF_ARENA_H__
#define __WTR_TLSF_ARENA_H__

#include "StaticArray.h"
#include "DynamicArray.h"
#include <cmath>

namespace wtr
{
	// TLSF Arena
	//
	// Physical chain (Block::prev/next) - always valid, free or allocated:
	//
	//  offset 0                                                                    totalSize
	//  +-----------+-----------+-----------+-----------+-----------+-----------+------+
	//  |  Block A  |  Block B  |  Block C  |  Block D  |  Block E  |  Block F  | ...  |
	//  |  [ALLOC]  |  [FREE]   |  [ALLOC]  |  [FREE]   |  [FREE]   |  [ALLOC]  |      |
	//  +-----------+-----------+-----------+-----------+-----------+-----------+------+
	//        --next-->   --next-->   --next-->   --next-->   --next-->
	//        <--prev--   <--prev--   <--prev--   <--prev--   <--prev--
	//
	// Size-class free-list (m_bitMap -> m_freeIndices -> Block::prevFree/nextFree):
	//
	//  m_bitMap[10] :  sli->  0   1   2   3   4 ...
	//                        [0] [1] [0] [1] [0] ...
	//                             |        |
	//                             |        +---> bucket(10,3) -> m_freeIndices[..] = 2
	//                             +------------> bucket(10,1) -> m_freeIndices[..] = 7
	//
	//      m_freeIndices[bucket(10,3)] = 2
	//                                     |
	//                                     v
	//                          +------------------+   nextFree   +------------------+
	//                          |  Block B (idx 2) | -----------> |  Block K (idx 9) |
	//                          |  prevFree = NONE | <----------- |  prevFree = 2    |
	//                          +------------------+   prevFree   +------------------+
	class TLSFArena
	{
	public :
		struct Range
		{
			size_t offset;
			size_t size;

			Range()
				: offset(0)
				, size(0)
			{}
		};

		struct Block : Range
		{
			uint64_t prev;       // physical chain - see class diagram
			uint64_t next;

			uint64_t prevFree;   // size-class chain (free only) - see class diagram
			uint64_t nextFree;

			bool free;

			Block()
				: Range()
				, prev(NULL_INDEX)
				, next(NULL_INDEX)
				, prevFree(NULL_INDEX)
				, nextFree(NULL_INDEX)
				, free(true)
			{}
		};

		struct Allocation : Range
		{
			uint64_t index;

			Allocation()
				: Range()
				, index(NULL_INDEX)
			{}
		};

	public :
		TLSFArena()
			: m_bitMap()
			, m_freeIndices{NULL_INDEX}
			, m_blocks()
			, m_unusedIndices()
			, m_totalSize(0)
			, m_remainedSize(0)
		{}

		virtual ~TLSFArena() = default;

	public :
		void Init(const size_t totalSize)
		{
			m_totalSize = GetNeededSize(totalSize);
			m_remainedSize = m_totalSize;

			Block block;
			block.free = true;
			block.offset = 0;
			block.size = m_totalSize;

			const size_t fli = GetFLI(m_totalSize);
			const size_t sli = GetSLI(m_totalSize, fli);
			const size_t bucketIndex = (fli * GetNumSli()) + sli;

			m_bitMap[fli] |= (1ull << sli);
			m_freeIndices[bucketIndex] = 0;
			m_blocks.PushBack(block);
		}

		Allocation Allocate(const size_t size, const size_t alignSize = ALIGN_SIZE)
		{
			Allocation allocation;

			const size_t neededSize = GetNeededSize(size);

			const size_t beginFLI = GetFLI(neededSize);
			const size_t endFLI = GetFLI(m_totalSize);
			for (size_t fli = beginFLI; fli <= endFLI; fli++)
			{
				const size_t requiredSize = (1ull << fli) > neededSize ? (1ull << fli) : neededSize;
				const size_t sli = GetSLI(requiredSize, fli);

				const size_t upperSLIBit = m_bitMap[fli] & (~0ull << sli);
				if (upperSLIBit == 0ull)
				{
					continue;
				}

				const size_t minSli = GetLSB(upperSLIBit);
				size_t bucketIndex = (fli * GetNumSli()) + minSli;
				if (bucketIndex >= m_freeIndices.Size())
				{
					continue;
				}

				const size_t blockIndex = m_freeIndices[bucketIndex];
				if (m_blocks[blockIndex].size > neededSize)
				{
					Split(blockIndex, neededSize);
				}
				else
				{
					Remove(blockIndex);
				}

				Block& block = m_blocks[blockIndex];
				block.free = false;

				allocation.offset = block.offset;
				allocation.size = block.size;
				allocation.index = blockIndex;

				break;
			}

			m_remainedSize -= allocation.size;

			return allocation;
		}

		void Free(const Allocation& allocation)
		{
			if (allocation.index >= m_blocks.Size())
			{
				return;
			}

			m_remainedSize += allocation.size;

			Block& block = m_blocks[allocation.index];
			block.free = true;
			const size_t leftIndex = block.prev;
			const size_t rightIndex = block.next;

			bool merged = false;
			merged |= Merge(allocation.index, leftIndex);
			merged |= Merge(allocation.index, rightIndex);

			if (!merged)
			{
				Insert(allocation.index);
			}
		}

		bool IsEmpty() const
		{
			return m_totalSize == m_remainedSize;
		}

	private :
		// Split(blockIndex, size):
		//
		//   before : [ --------------- blockIndex, FREE --------------- ]
		//   after  : [ blockIndex (size), ALLOC ][ rightIndex, FREE ]
		void Split(const size_t blockIndex, const size_t size)
		{
			if (blockIndex >= m_blocks.Size())
			{
				return;
			}

			const size_t neededSize = GetNeededSize(size);

			Block origin = m_blocks[blockIndex];
			if (neededSize >= origin.size)
			{
				return;
			}

			const size_t rightIndex = GetUnusedIndex();

			Block& right = m_blocks[rightIndex];
			right.offset = origin.offset + neededSize;
			right.size = origin.size - neededSize;
			right.free = true;

			right.prev = blockIndex;
			right.next = origin.next;

			Remove(blockIndex);

			Block& block = m_blocks[blockIndex];
			block.size = neededSize;
			block.free = false;
			block.prevFree = NULL_INDEX;
			block.nextFree = NULL_INDEX;

			Update(rightIndex);
			Insert(rightIndex);
		}

		// Merge(baseIndex, mergedIndex) - direction read from offsets, never blockIndex:
		//
		//   isLeft=true  : [ base ][ merged ]  -> [ base (grown) ]
		//   isLeft=false : [ merged ][ base ]  -> [ base (grown) ]
		bool Merge(const size_t baseIndex, const size_t mergedIndex)
		{
			if (baseIndex == mergedIndex || baseIndex >= m_blocks.Size() || mergedIndex >= m_blocks.Size())
			{
				return false;
			}

			Block& base = m_blocks[baseIndex];
			Block& merged = m_blocks[mergedIndex];
			if (!base.free || !merged.free)
			{
				return false;
			}

			const bool isLeft = (base.offset < merged.offset);
			const bool isNear = isLeft ? ((base.offset + base.size) == merged.offset) : ((merged.offset + merged.size) == base.offset);
			if (!isNear)
			{
				return false;
			}

			Remove(baseIndex);
			Remove(mergedIndex);

			base.size = base.size + merged.size;
			base.offset = isLeft ? base.offset : merged.offset;
			base.prev = isLeft ? base.prev : merged.prev;
			base.next = isLeft ? merged.next : base.next;

			Update(baseIndex);
			Insert(baseIndex);

			m_unusedIndices.PushBack(mergedIndex);

			return true;
		}

		// Update(blockIndex) - closes both halves of the physical link:
		//
		//   [ prev.next = blockIndex ] <---- [ blockIndex ] ----> [ next.prev = blockIndex ]
		void Update(const size_t blockIndex)
		{
			if (blockIndex >= m_blocks.Size())
			{
				return;
			}

			Block& block = m_blocks[blockIndex];
			if (block.prev != NULL_INDEX && block.prev < m_blocks.Size())
			{
				Block& prev = m_blocks[block.prev];
				prev.next = blockIndex;
			}

			if (block.next != NULL_INDEX && block.next < m_blocks.Size())
			{
				Block& next = m_blocks[block.next];
				next.prev = blockIndex;
			}
		}

		// Insert(blockIndex) - push to head of the bucket's chain:
		//
		//   before : m_freeIndices[bucket] -> [ H ]
		//   after  : m_freeIndices[bucket] -> [ blockIndex ] --nextFree--> [ H ]
		//                                                    <-prevFree--
		void Insert(const size_t blockIndex)
		{
			if (blockIndex >= m_blocks.Size())
			{
				return;
			}

			Block& block = m_blocks[blockIndex];
			const size_t fli = GetFLI(block.size);
			const size_t sli = GetSLI(block.size, fli);
			const size_t bucketIndex = fli * GetNumSli() + sli;

			if (m_freeIndices[bucketIndex] == NULL_INDEX)
			{
				m_freeIndices[bucketIndex] = blockIndex;

				block.prevFree = NULL_INDEX;
				block.nextFree = NULL_INDEX;
			}
			else
			{
				const size_t headIndex = m_freeIndices[bucketIndex];
				if (headIndex >= m_blocks.Size())
				{
					return;
				}

				Block& head = m_blocks[headIndex];
				head.prevFree = blockIndex;

				block.nextFree = headIndex;
				block.prevFree = NULL_INDEX;

				m_freeIndices[bucketIndex] = blockIndex;
			}

			block.free = true;
			m_bitMap[fli] |= (1ull << sli);
		}

		// Remove(blockIndex) - splice out of the bucket's chain:
		//
		//   before : [ prevFree ] <-> [ blockIndex ] <-> [ nextFree ]
		//   after  : [ prevFree ] <-----------------------> [ nextFree ]
		void Remove(const size_t blockIndex)
		{
			if (blockIndex >= m_blocks.Size())
			{
				return;
			}

			Block& block = m_blocks[blockIndex];

			const size_t fli = GetFLI(block.size);
			const size_t sli = GetSLI(block.size, fli);
			const size_t bucketIndex = fli * GetNumSli() + sli;

			if (m_freeIndices[bucketIndex] == blockIndex)
			{
				if (block.prevFree != NULL_INDEX)
				{
					m_freeIndices[bucketIndex] = block.prevFree;
				}
				else if (block.nextFree != NULL_INDEX)
				{
					m_freeIndices[bucketIndex] = block.nextFree;
				}
				else
				{
					m_freeIndices[bucketIndex] = NULL_INDEX;

					m_bitMap[fli] &= ~(1ull << sli);
				}
			}

			if (block.prevFree != NULL_INDEX && block.prevFree < m_blocks.Size())
			{
				Block& prev = m_blocks[block.prevFree];
				prev.nextFree = block.nextFree;
			}

			if (block.nextFree != NULL_INDEX && block.nextFree < m_blocks.Size())
			{
				Block& next = m_blocks[block.nextFree];
				next.prevFree = block.prevFree;
			}

			block.prevFree = NULL_INDEX;
			block.nextFree = NULL_INDEX;
			block.free = false;
		}

		size_t GetUnusedIndex()
		{
			if (m_unusedIndices.Empty())
			{
				m_blocks.EmplaceBack();
				return m_blocks.Size() - 1;
			}
			else
			{
				const size_t unusedIndex = m_unusedIndices.Back();
				m_unusedIndices.PopBack();

				return unusedIndex;
			}
		}

		size_t GetNeededSize(const size_t size) const
		{
			return (size + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1);
		}

		size_t GetFLI(const size_t size) const
		{
			return GetMSB(size);
		}

		size_t GetSLI(const size_t size, const size_t firstLevel) const
		{
			return (size >> (firstLevel - SL_COUNT)) & ((1 << SL_COUNT) - 1);
		}

		size_t GetMSB(const size_t value) const
		{
			size_t copyValue = value;
			if (copyValue == 0)
			{
				return 0;
			}

			size_t bit = 0;
			if (copyValue >= 0x100000000ull)	{ copyValue >>= 32; bit += 32; }
			if (copyValue >= 0x10000)			{ copyValue >>= 16; bit += 16; }
			if (copyValue >= 0x100)				{ copyValue >>= 8;  bit += 8; }
			if (copyValue >= 0x10)				{ copyValue >>= 4;  bit += 4; }
			if (copyValue >= 0x4)				{ copyValue >>= 2;  bit += 2; }
			if (copyValue >= 0x2)				{ copyValue >>= 1;  bit += 1; }

			return bit;
		}

		size_t GetLSB(const size_t value) const
		{
			size_t copyValue = value;
			if (copyValue == 0)
			{
				return 64;
			}

			size_t bit = 0;
			if ((copyValue & 0xFFFFFFFFull) == 0) { bit += 32; copyValue >>= 32; }
			if ((copyValue & 0xFFFFull) == 0) { bit += 16; copyValue >>= 16; }
			if ((copyValue & 0xFFull) == 0) { bit += 8;  copyValue >>= 8; }
			if ((copyValue & 0xFull) == 0) { bit += 4;  copyValue >>= 4; }
			if ((copyValue & 0x3ull) == 0) { bit += 2;  copyValue >>= 2; }
			if ((copyValue & 0x1ull) == 0) { bit += 1; }

			return bit;
		}

		size_t GetNumSli() const
		{
			return (1ull << SL_COUNT);
		}

	public :
		static constexpr uint64_t NULL_INDEX = ~0ull;
		static constexpr size_t FL_COUNT = 64;
		static constexpr size_t SL_COUNT = 4;
		static constexpr size_t	ALIGN_SIZE = 256;

	private :
		StaticArray<uint64_t, FL_COUNT> m_bitMap;
		StaticArray<uint64_t, FL_COUNT* (1ull << SL_COUNT)> m_freeIndices;

		DynamicArray<Block>		m_blocks;
		DynamicArray<size_t>	m_unusedIndices;

		size_t m_totalSize;
		size_t m_remainedSize;
	};
};

#endif // __WTR_TLSF_ARENA_H__
