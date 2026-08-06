#include "Variant.h"
#include "List.h"
#include "DynamicArray.h"
#include "StaticArray.h"
#include "HashSet.h"
#include "HashMap.h"
#include "Arena.h"
#include "LinearArena.h"
#include "TLSFArena.h"

#include <Log/include/Log.h>
#include <Log/include/LogPlatform.h>

#include <vector>
#include <string>
#include <ostream>
#include <algorithm>
#include <cstring>
#include <cstdint>

// Shared lifecycle-tracking type used by List/DynamicArray/StaticArray tests -
// logs every constructor/destructor/copy/move so the exact sequence of calls
// a container makes internally is visible, not just the end result.
struct TrackedObject
{
	int m_id;

	TrackedObject() : m_id(0) { LOGINFO() << "TrackedObject Default Constructor"; }
	TrackedObject(int id) : m_id(id) { LOGINFO() << "TrackedObject Constructor : " << m_id; }
	~TrackedObject() { LOGINFO() << "TrackedObject Destructor : " << m_id; }

	TrackedObject(const TrackedObject& other) : m_id(other.m_id)
	{
		LOGINFO() << "TrackedObject Copy Constructor : " << m_id;
	}

	TrackedObject(TrackedObject&& other) noexcept : m_id(other.m_id)
	{
		other.m_id = -1;
		LOGINFO() << "TrackedObject Move Constructor : " << m_id;
	}

	TrackedObject& operator=(const TrackedObject& other)
	{
		m_id = other.m_id;
		LOGINFO() << "TrackedObject Copy Assign : " << m_id;
		return *this;
	}

	TrackedObject& operator=(TrackedObject&& other) noexcept
	{
		m_id = other.m_id;
		other.m_id = -1;
		LOGINFO() << "TrackedObject Move Assign : " << m_id;
		return *this;
	}

	bool operator==(const TrackedObject& other) const { return m_id == other.m_id; }
};

void ListTest()
{
	LOGINFO() << "========== List Test Start ==========";

	// 1. Basic Push/Pop & Front/Back
	{
		LOGINFO() << "[Test 1] Basic Push/Pop & Front/Back";

		wtr::List<int> list;
		list.PushBack(1);
		list.PushBack(2);
		list.PushFront(0);

		LOGINFO() << "Front : " << list.Front() << ", Back : " << list.Back() << ", Size : " << list.Size();

		if (list.Front() == 0 && list.Back() == 2 && list.Size() == 3)
		{
			LOGINFO() << "Basic Push/Pop & Front/Back Passed";
		}

		list.PopFront();
		list.PopBack();

		LOGINFO() << "After Pop, Size (expect 1) : " << list.Size();
	}

	// 2. Initializer List & Forward/Reverse Iteration
	{
		LOGINFO() << "[Test 2] Initializer List & Forward/Reverse Iteration";

		wtr::List<int> list = { 1, 2, 3, 4, 5 };

		LOGINFO() << "Forward :";
		for (const auto& value : list)
		{
			LOGINFO() << "  " << value;
		}

		LOGINFO() << "Reverse :";
		auto itr = list.rBegin();
		while (itr != list.rEnd())
		{
			LOGINFO() << "  " << *itr;
			++itr;
		}
	}

	// 3. Insert / Find / Erase at Position
	{
		LOGINFO() << "[Test 3] Insert / Find / Erase at Position";

		wtr::List<int> list = { 10, 20, 30 };

		auto itr = list.Begin();
		++itr; // points to 20

		list.Insert(itr, 99); // 10, 99, 20, 30
		for (const auto& value : list) { LOGINFO() << "Val : " << value; }

		itr = list.Find(20);
		list.Erase(itr); // 10, 99, 30

		LOGINFO() << "After Erase(20) :";
		for (const auto& value : list) { LOGINFO() << "Val : " << value; }

		if (list.Size() == 3 && list.Find(20) == list.End())
		{
			LOGINFO() << "Insert / Find / Erase at Position Passed";
		}
	}

	// 4. Remove by Value (removes every matching element, not just the first)
	{
		LOGINFO() << "[Test 4] Remove by Value";

		wtr::List<int> list = { 1, 2, 1, 3, 1 };
		list.Remove(1);

		LOGINFO() << "Size after Remove(1) (expect 2) : " << list.Size();
		for (const auto& value : list) { LOGINFO() << "Val : " << value; }
	}

	// 5. Splice - whole list, single element, and range
	{
		LOGINFO() << "[Test 5] Splice (whole list / single element / range)";

		// 5a. Whole-list splice: every node of 'donor' moves to the end of 'target' in one call.
		wtr::List<int> target = { 1, 2, 3 };
		wtr::List<int> donor = { 4, 5, 6 };

		target.Splice(target.End(), donor);
		LOGINFO() << "After whole-list splice, target.Size() : " << target.Size() << ", donor.Size() : " << donor.Size();

		// 5b. Single-element splice: only '100' moves from 'single' to the front of 'target'.
		wtr::List<int> single = { 100, 200 };
		auto singleItr = single.Begin(); // 100

		target.Splice(target.Begin(), single, singleItr);
		LOGINFO() << "After single-element splice, target.Size() : " << target.Size() << ", single.Size() : " << single.Size();

		// 5c. Range splice: [rangeFirst, rangeLast) == just '7' moves into the middle of 'target'.
		wtr::List<int> range = { 7, 8, 9, 10 };
		auto rangeFirst = range.Begin();  // 7
		auto rangeLast = range.Begin();
		++rangeLast;                      // 8, so [rangeFirst, rangeLast) == { 7 }

		auto middle = target.Begin();
		++middle;

		target.Splice(middle, range, rangeFirst, rangeLast);
		LOGINFO() << "After range splice, target.Size() : " << target.Size() << ", range.Size() : " << range.Size();

		for (const auto& value : target) { LOGINFO() << "target Val : " << value; }
	}

	// 6. Move Construction & Assignment (copy is intentionally deleted on List)
	{
		LOGINFO() << "[Test 6] Move Construction & Assignment";

		wtr::List<int> source = { 1, 2, 3 };

		wtr::List<int> moved = std::move(source);
		LOGINFO() << "Moved Size : " << moved.Size() << ", Source Size (After Move) : " << source.Size();

		wtr::List<int> assigned;
		assigned = std::move(moved);
		LOGINFO() << "Assigned Size : " << assigned.Size() << ", Moved Size (After Move) : " << moved.Size();

		if (assigned.Size() == 3 && source.Empty() && moved.Empty())
		{
			LOGINFO() << "Move Construction & Assignment Passed";
		}
	}

	// 7. Object Lifecycle via Insert/Erase
	//
	// Note: List::Insert now placement-constructs the Node's item directly from the forwarded
	// argument (Node's variadic constructor), so PushBack(TrackedObject(id)) only logs a single
	// construction - no default-construct-then-assign step, matching emplace-style construction.
	{
		LOGINFO() << "[Test 7] Object Lifecycle via Insert/Erase";

		wtr::List<TrackedObject> list;

		LOGINFO() << "[PushBack]";
		list.PushBack(TrackedObject(1));

		LOGINFO() << "[PushBack]";
		list.PushBack(TrackedObject(2));

		LOGINFO() << "[PopFront]";
		list.PopFront(); // TrackedObject(1) destructed here
	} // TrackedObject(2) destructed here via ~List -> Clear()

	LOGINFO() << "========== List Test End ==========";
}

void StaticArrayTest()
{
	LOGINFO() << "========== StaticArray Test Start ==========";

	// 1. Basic Access & Fill
	{
		LOGINFO() << "[Test 1] Basic Access & Fill";

		wtr::StaticArray<int, 5> arr;
		LOGINFO() << "Size (always 5) : " << arr.Size();

		arr.Fill(10);
		arr[0] = 99;
		arr.Back() = 77;

		LOGINFO() << "Index 0 : " << arr[0] << ", Front : " << arr.Front() << ", Back : " << arr.Back() << ", At(2) : " << arr.At(2);

		if (arr[0] == 99 && arr.Back() == 77 && arr.At(2) == 10)
		{
			LOGINFO() << "Basic Access & Fill Passed";
		}
	}

	// 2. Initializer List & Iteration (short list pads the remainder with the last given value)
	{
		LOGINFO() << "[Test 2] Initializer List & Iteration";

		wtr::StaticArray<std::string, 3> arr = { "Apple", "Banana" };
		LOGINFO() << "Size : " << arr.Size();

		int index = 0;
		for (const auto& item : arr)
		{
			LOGINFO() << "Item " << index++ << " : " << item;
		}
	}

	// 3. Object Lifecycle via Assignment
	{
		LOGINFO() << "[Test 3] Object Lifecycle via Assignment";

		LOGINFO() << "[Create Array with Default Constructors]";
		wtr::StaticArray<TrackedObject, 2> arr; // 2x Default Constructor

		LOGINFO() << "[Assign R-Value]";
		arr[0] = TrackedObject(10); // Constructor(10) -> Move Assign -> Destructor(10)

		LOGINFO() << "[Assign L-Value]";
		TrackedObject obj(20);
		arr[1] = obj; // Copy Assign
	} // Locals destruct in reverse declaration order: obj(20) first, then arr's slots (20, then 10)

	// 4. Copy & Move Semantics (StaticArray move is element-wise, not a pointer swap)
	{
		LOGINFO() << "[Test 4] Copy & Move Semantics";

		wtr::StaticArray<int, 3> original = { 1, 2, 3 };

		wtr::StaticArray<int, 3> copyArr = original;
		copyArr[0] = 999;
		LOGINFO() << "Copy[0] modified : " << copyArr[0] << ", Original[0] unaffected : " << original[0];

		wtr::StaticArray<int, 3> moveArr = std::move(original);
		LOGINFO() << "Move[0] : " << moveArr[0] << ", Original Size (always 3, StaticArray can't shrink) : " << original.Size();
	}

	// 5. Const Access
	{
		LOGINFO() << "[Test 5] Const Access";

		const wtr::StaticArray<int, 3> constArr = { 100, 200, 300 };
		LOGINFO() << "Const Front : " << constArr.Front() << ", Back : " << constArr.Back() << ", At(1) : " << constArr.At(1);

		// constArr[0] = 500; // Expected compile error - const StaticArray is read-only
	}

	LOGINFO() << "========== StaticArray Test End ==========";
}

void DynamicArrayTest()
{
	LOGINFO() << "========== DynamicArray Test Start ==========";

	// 1. Basic PushBack & Access
	{
		LOGINFO() << "[Test 1] Basic PushBack & Access";

		wtr::DynamicArray<int> arr;
		arr.Reserve(4);
		LOGINFO() << "Initial Capacity : " << arr.Capacity();

		arr.PushBack(10);
		arr.PushBack(20);
		arr.PushBack(30);

		LOGINFO() << "Size : " << arr.Size() << ", Front : " << arr.Front() << ", Back : " << arr.Back();
	}

	// 2. Initializer List & Iterator
	{
		LOGINFO() << "[Test 2] Initializer List & Iterator";

		wtr::DynamicArray<std::string> arr = { "Apple", "Banana", "Cherry" };

		int index = 0;
		for (const auto& item : arr)
		{
			LOGINFO() << "Item " << index++ << " : " << item;
		}
	}

	// 3. Object Lifecycle - PushBack vs EmplaceBack
	//
	// Note: PushBack(T&&) now exists and moves via EmplaceBack(std::move(data)), matching
	// std::vector. EmplaceBack still wins for a fresh object though: it constructs T directly
	// in place from the given args, so there's no temporary to move from at all.
	{
		LOGINFO() << "[Test 3] Object Lifecycle - PushBack vs EmplaceBack";

		wtr::DynamicArray<TrackedObject> arr;

		LOGINFO() << "[PushBack R-Value] (Move Constructor expected)";
		arr.PushBack(TrackedObject(1));

		LOGINFO() << "[EmplaceBack] (Constructor directly in place, no copy/move at all)";
		arr.EmplaceBack(2);

		LOGINFO() << "[PopBack]";
		arr.PopBack(); // Destructor(2) expected
	} // Destructor(1) expected here via ~DynamicArray -> Clear()

	// 4. Copy & Move Semantics
	{
		LOGINFO() << "[Test 4] Copy & Move Semantics";

		wtr::DynamicArray<int> original = { 1, 2, 3 };

		wtr::DynamicArray<int> copyArr = original;
		LOGINFO() << "Copy Size : " << copyArr.Size() << ", Copy[0] : " << copyArr[0];

		wtr::DynamicArray<int> assignArr;
		assignArr = original;
		LOGINFO() << "Assign Size : " << assignArr.Size();

		wtr::DynamicArray<int> moveArr = std::move(original);
		LOGINFO() << "Move Size : " << moveArr.Size() << ", Original Size (After Move, expect 0) : " << original.Size();
	}

	// 5. Insert & Erase
	{
		LOGINFO() << "[Test 5] Insert & Erase";

		wtr::DynamicArray<int> arr = { 10, 20, 30, 40, 50 };

		auto it = arr.begin();
		++it; ++it; // points to 30

		arr.Insert(it, 99); // 10, 20, 99, 30, 40, 50
		for (auto val : arr) { LOGINFO() << "Val : " << val; }

		it = arr.begin();
		++it; // points to 20

		arr.Erase(it); // 10, 99, 30, 40, 50
		for (auto val : arr) { LOGINFO() << "Val : " << val; }
	}

	// 6. Resize & Clear
	{
		LOGINFO() << "[Test 6] Resize & Clear";

		wtr::DynamicArray<int> arr = { 1, 2, 3 };

		arr.Resize(5); // 1, 2, 3, 0, 0
		LOGINFO() << "Resize(5), Size : " << arr.Size() << ", Capacity : " << arr.Capacity();

		arr.Resize(2); // 1, 2
		LOGINFO() << "Resize(2), Size : " << arr.Size();

		arr.Clear();
		LOGINFO() << "Clear, Size : " << arr.Size() << ", Empty : " << (arr.Empty() ? "True" : "False");
	}

	LOGINFO() << "========== DynamicArray Test End ==========";
}

void HashSetTest()
{
	LOGINFO() << "========== HashSet Test Start ==========";

	// 1. Duplicate Check (Primitive Type)
	{
		LOGINFO() << "[Test 1] Integer Set (Duplicate Check)";

		wtr::HashSet<int> set{ 3, 1, 2, 3, 4, 2, 2, 4 }; // dedup to {1,2,3,4}

		set.Insert(10);
		set.Insert(15);
		auto result = set.Insert(10); // duplicate

		LOGINFO() << "Duplicate insertion result.second (expect false) : " << (result.second ? "true" : "false");
		LOGINFO() << "Set Size (expect 6) : " << set.Size();

		for (const auto& val : set) { LOGINFO() << "Val : " << val; }
	}

	// 2. String Set & Full Erase
	{
		LOGINFO() << "[Test 2] String Set & Erase";

		wtr::HashSet<std::string> set;
		set.Insert("Apple");
		set.Insert("Banana");
		set.Insert("Cherry");

		LOGINFO() << "Before Erase : " << set.Size();
		set.Erase("Apple");
		LOGINFO() << "After Erase('Apple') : " << set.Size();

		set.Erase("Ghost"); // non-existent key, must be a safe no-op

		auto it = set.Begin();
		while (it != set.End())
		{
			it = set.Erase(it);
		}

		LOGINFO() << "Set Empty after full-erase loop : " << (set.Empty() ? "True" : "False");
	}

	// 3. Custom Struct with Custom Hasher
	{
		LOGINFO() << "[Test 3] Custom Struct Set";

		struct Vector2
		{
			int x, y;

			Vector2() = default;
			Vector2(int _x, int _y) : x(_x), y(_y) {}
			bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
		};

		struct Vector2Hasher
		{
			size_t operator()(const Vector2& v) const
			{
				return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
			}
		};

		wtr::HashSet<Vector2, Vector2Hasher> vecSet;
		vecSet.Emplace(1, 1);
		vecSet.Emplace(2, 2);
		vecSet.Emplace(1, 1); // duplicate, ignored
		vecSet.Insert({ 3, 3 });

		LOGINFO() << "Custom Struct Set Size (expect 3) : " << vecSet.Size();
		for (const auto& v : vecSet) { LOGINFO() << "Vector : " << v.x << " " << v.y; }
	}

	// 4. Rehash Stress Test - large insert count forces multiple rehashes
	{
		LOGINFO() << "[Test 4] Rehash Stress Test";

		wtr::HashSet<int> set;
		const int count = 200;
		for (int i = 0; i < count; ++i)
		{
			set.Insert(i);
		}

		bool allFound = true;
		for (int i = 0; i < count; ++i)
		{
			if (!set.Contains(i))
			{
				allFound = false;
				break;
			}
		}

		LOGINFO() << "Size after " << count << " insertions : " << set.Size();
		LOGINFO() << (allFound ? "All elements survived rehash" : "[Error] Data loss during rehash");
	}

	LOGINFO() << "========== HashSet Test End ==========";
}

void HashMapTest()
{
	LOGINFO() << "========== HashMap Test Start ==========";

	// 1. Basic operator[] Insert/Update + Emplace
	{
		LOGINFO() << "[Test 1] Int-String Map (Basic Ops)";

		wtr::HashMap<int, std::string> map;
		map[1] = "One";
		map[2] = "Two";
		map[10] = "Ten";
		map.Emplace(5, "Five");
		map[1] = "Uno"; // update existing key

		LOGINFO() << "Size (expect 4) : " << map.Size();
		LOGINFO() << "map[1] (expect Uno) : " << map[1];

		for (auto& pair : map) { LOGINFO() << "Key: " << pair.first << ", Value: " << pair.second; }
	}

	// 2. Const Access via At()
	{
		LOGINFO() << "[Test 2] Const Map Access (At)";

		wtr::HashMap<std::string, int> map;
		map["HP"] = 100;
		map["MP"] = 50;

		const auto& constMap = map;
		LOGINFO() << "constMap.At(\"HP\") : " << constMap.At("HP");
	}

	// 3. TryEmplace - only inserts when the key is missing
	{
		LOGINFO() << "[Test 3] TryEmplace";

		wtr::HashMap<int, int> map;
		map[1] = 100;

		auto first = map.TryEmplace(1, 999);  // key exists -> ignored, returns (itr, false)
		auto second = map.TryEmplace(2, 200); // key missing -> inserted, returns (itr, true)

		LOGINFO() << "TryEmplace existing key -> inserted : " << (first.second ? "true" : "false") << ", value unchanged : " << map[1];
		LOGINFO() << "TryEmplace new key -> inserted : " << (second.second ? "true" : "false") << ", value : " << map[2];
	}

	// 4. Custom Struct Key/Value
	{
		LOGINFO() << "[Test 4] Custom Struct Key/Value";

		struct PlayerID
		{
			int uid;
			int serverId;

			PlayerID() = default;
			PlayerID(int _uid, int _serverId) : uid(_uid), serverId(_serverId) {}
			bool operator==(const PlayerID& other) const { return uid == other.uid && serverId == other.serverId; }
		};

		struct PlayerIDHasher
		{
			size_t operator()(const PlayerID& id) const
			{
				return std::hash<int>()(id.uid) ^ (std::hash<int>()(id.serverId) << 1);
			}
		};

		wtr::HashMap<PlayerID, int, PlayerIDHasher> playerMap;

		PlayerID p1 = { 1001, 1 };
		PlayerID p2 = { 1002, 1 };

		playerMap[p1] = 100;
		playerMap[p2] = 200;
		playerMap[p1] -= 10; // take damage, HP: 100 -> 90

		LOGINFO() << "Player1 HP (expect 90) : " << playerMap[p1];

		playerMap.Erase(p2);
		LOGINFO() << "Player2 erased : " << (playerMap.Find(p2) == playerMap.End() ? "true" : "false");
	}

	// 5. Rehash Stress Test
	{
		LOGINFO() << "[Test 5] Rehash Stress Test";

		wtr::HashMap<int, int> map;
		const int count = 200;
		for (int i = 0; i < count; ++i)
		{
			map[i] = i * 10;
		}

		bool allFound = true;
		for (int i = 0; i < count; ++i)
		{
			if (map.Find(i) == map.End() || map[i] != i * 10)
			{
				allFound = false;
				break;
			}
		}

		LOGINFO() << "Size after " << count << " insertions : " << map.Size();
		LOGINFO() << (allFound ? "All elements survived rehash" : "[Error] Data loss during rehash");
	}

	LOGINFO() << "========== HashMap Test End ==========";
}

void ArenaTest()
{
	LOGINFO() << "========== Arena Test Start ==========";

	// 1. Basic Allocate & Deallocate - each call is backed by its own operator new block
	{
		LOGINFO() << "[Test 1] Basic Allocate & Deallocate";

		wtr::Arena arena;

		void* memory = arena.Allocate(sizeof(int));
		if (nullptr == memory)
		{
			LOGINFO() << "[Error] Allocate returned nullptr";
		}
		else
		{
			int* value = new (memory) int(42);
			LOGINFO() << "Allocated int value : " << *value;

			arena.Deallocate(memory);
			LOGINFO() << "Basic Allocate & Deallocate Passed";
		}
	}

	// 2. Multiple Independent Allocations - Arena has no shared buffer, so writing
	// through one block must never disturb another, and freeing out of order (b, then a, then c)
	// must not corrupt the page list.
	{
		LOGINFO() << "[Test 2] Multiple Independent Allocations";

		wtr::Arena arena;

		void* a = arena.Allocate(64);
		void* b = arena.Allocate(128);
		void* c = arena.Allocate(256);

		LOGINFO() << "Distinct Addresses : " << (a != b && b != c && a != c ? "true" : "false");

		std::memset(a, 0xAA, 64);
		std::memset(c, 0xCC, 256);

		arena.Deallocate(b);
		arena.Deallocate(a);
		arena.Deallocate(c);

		LOGINFO() << "Multiple Independent Allocations Passed";
	}

	// 3. Alignment - note: alignSize here only controls where the internal Page header
	// is placed right after the payload, not the returned pointer itself. 'memory' is
	// always the raw base of the ::operator new block, so its alignment is whatever
	// operator new's default guarantee is (__STDCPP_DEFAULT_NEW_ALIGNMENT__), regardless
	// of what alignSize is passed in.
	{
		LOGINFO() << "[Test 3] Default operator new Alignment";

		wtr::Arena arena;

		void* memory = arena.Allocate(sizeof(double), alignof(double));
		const bool isAligned = (reinterpret_cast<uintptr_t>(memory) % alignof(double)) == 0;

		LOGINFO() << "Requested align : " << alignof(double) << ", is aligned : " << (isAligned ? "true" : "false");

		arena.Deallocate(memory);

		if (isAligned)
		{
			LOGINFO() << "Default operator new Alignment Passed";
		}
	}

	// 4. Move Constructor Transfers Ownership
	{
		LOGINFO() << "[Test 4] Move Constructor Transfers Ownership";

		wtr::Arena source;
		void* memory = source.Allocate(32);

		wtr::Arena moved = std::move(source);

		// The moved-from arena no longer owns any page, so only 'moved' may Deallocate it safely.
		moved.Deallocate(memory);

		LOGINFO() << "Move Constructor Passed";
	}

	LOGINFO() << "========== Arena Test End ==========";
}

void LinearArenaTest()
{
	LOGINFO() << "========== LinearArena Test Start ==========";

	// 1. Basic Bump Allocation - consecutive Allocate<T>() calls land in the same page
	// at increasing addresses, since this is a pure bump pointer, not a free-list allocator.
	{
		LOGINFO() << "[Test 1] Basic Bump Allocation";

		wtr::LinearArena arena;

		int* a = new (arena.Allocate<int>()) int(1);
		int* b = new (arena.Allocate<int>()) int(2);
		int* c = new (arena.Allocate<int>()) int(3);

		LOGINFO() << "A : " << *a << ", B : " << *b << ", C : " << *c;
		LOGINFO() << "Bump Allocation Order (a < b < c) : " << (a < b && b < c ? "true" : "false");
	}

	// 2. Alignment Within a Page - mixed-size allocations must still land on aligned addresses
	{
		LOGINFO() << "[Test 2] Alignment Within a Page";

		wtr::LinearArena arena;

		arena.Allocate(1, alignof(char)); // misalign the bump offset first
		double* aligned = static_cast<double*>(arena.Allocate(sizeof(double), alignof(double)));

		const bool isAligned = (reinterpret_cast<uintptr_t>(aligned) % alignof(double)) == 0;
		LOGINFO() << "Double is aligned despite preceding 1-byte allocation : " << (isAligned ? "true" : "false");
	}

	// 3. Oversized Allocation Forces a Dedicated Page (Page::MIN_SIZE is 64KB)
	{
		LOGINFO() << "[Test 3] Oversized Allocation Forces a New Page";

		wtr::LinearArena arena;

		const size_t bigSize = 128 * 1024; // bigger than Page::MIN_SIZE
		void* big = arena.Allocate(bigSize, alignof(std::max_align_t));

		if (nullptr != big)
		{
			std::memset(big, 0xEE, bigSize);
			LOGINFO() << "Oversized Allocation Passed";
		}
		else
		{
			LOGINFO() << "[Error] Oversized Allocation Failed";
		}
	}

	// 4. Multiple Pages Chain Together - once one page can no longer fit the next
	// request, GetPage() falls through and a second page is created automatically.
	{
		LOGINFO() << "[Test 4] Multiple Pages Chain Together";

		wtr::LinearArena arena;

		const size_t blockSize = 20 * 1024; // 4 of these exceed one 64KB page
		void* first = arena.Allocate(blockSize, alignof(std::max_align_t));
		void* second = arena.Allocate(blockSize, alignof(std::max_align_t));
		void* third = arena.Allocate(blockSize, alignof(std::max_align_t));
		void* fourth = arena.Allocate(blockSize, alignof(std::max_align_t)); // spills into a new page

		const bool allValid = nullptr != first && nullptr != second && nullptr != third && nullptr != fourth;
		LOGINFO() << "Multiple Pages Chain Together Passed : " << (allValid ? "true" : "false");
	}

	// 5. Reset Reuses Page Memory - Reset() rewinds every page's bump offset back to 0
	// without releasing the underlying memory, so the next Allocate() reuses the same
	// address as before. There is no per-object Deallocate() here - LinearArena only
	// supports resetting the whole arena at once.
	{
		LOGINFO() << "[Test 5] Reset Reuses Page Memory";

		wtr::LinearArena arena;

		int* before = new (arena.Allocate<int>()) int(10);
		void* beforeAddr = before;

		arena.Reset();

		int* after = new (arena.Allocate<int>()) int(20);
		void* afterAddr = after;

		LOGINFO() << "Address reused after Reset() : " << (beforeAddr == afterAddr ? "true" : "false");
	}

	LOGINFO() << "========== LinearArena Test End ==========";
}

void TLSFArenaTest()
{
	LOGINFO() << "========== TLSFArena Test Start ==========";

	// 1. Init & Single Allocate
	{
		LOGINFO() << "[Test 1] Init & Single Allocate";

		wtr::TLSFArena arena;
		arena.Init(1024 * 1024); // 1MB

		auto alloc = arena.Allocate(1000);

		LOGINFO() << "Offset : " << alloc.offset << ", Size : " << alloc.size << ", Index : " << alloc.index;

		if (alloc.offset == 0 && alloc.size >= 1000)
		{
			LOGINFO() << "Basic Allocate Passed";
		}
		else
		{
			LOGINFO() << "[Error] Basic Allocate Failed";
		}
	}

	// 2. Sequential Allocate - offsets should be contiguous, no overlap
	{
		LOGINFO() << "[Test 2] Sequential Allocate (No Overlap)";

		wtr::TLSFArena arena;
		arena.Init(1024 * 1024);

		auto a = arena.Allocate(1024);
		auto b = arena.Allocate(2048);
		auto c = arena.Allocate(512);

		LOGINFO() << "A : offset=" << a.offset << " size=" << a.size;
		LOGINFO() << "B : offset=" << b.offset << " size=" << b.size;
		LOGINFO() << "C : offset=" << c.offset << " size=" << c.size;

		bool noOverlap = (b.offset == a.offset + a.size) && (c.offset == b.offset + b.size);

		if (noOverlap)
		{
			LOGINFO() << "Sequential Allocate Passed (contiguous, no overlap)";
		}
		else
		{
			LOGINFO() << "[Error] Sequential Allocate Failed";
		}
	}

	// 3. Free & Reallocate - freed space should be reusable
	{
		LOGINFO() << "[Test 3] Free & Reallocate";

		wtr::TLSFArena arena;
		arena.Init(1024 * 1024);

		auto a = arena.Allocate(1024);
		arena.Free(a);

		auto b = arena.Allocate(1024);

		LOGINFO() << "Freed A : offset=" << a.offset;
		LOGINFO() << "Reallocated B : offset=" << b.offset;

		if (b.offset == a.offset)
		{
			LOGINFO() << "Free & Reallocate Passed (space reused)";
		}
		else
		{
			LOGINFO() << "[Error] Free & Reallocate Failed";
		}
	}

	// 4. Merge on Free - freeing neighbors should merge back into one block
	{
		LOGINFO() << "[Test 4] Merge on Free (Left/Right Neighbor)";

		wtr::TLSFArena arena;
		arena.Init(1024 * 1024);

		auto a = arena.Allocate(1024);
		auto b = arena.Allocate(1024);
		auto c = arena.Allocate(1024);

		// Free B first : both neighbors still allocated -> no merge
		arena.Free(b);

		// Free A : right neighbor(B) is free -> should merge into A
		arena.Free(a);

		// Free C : left neighbor(A+B merged) is free -> should merge into one block
		arena.Free(c);

		size_t mergedTarget = a.size + b.size + c.size;
		auto merged = arena.Allocate(mergedTarget);

		LOGINFO() << "Merged Allocate : offset=" << merged.offset << " size=" << merged.size;

		if (merged.offset == a.offset && merged.size >= mergedTarget)
		{
			LOGINFO() << "Merge on Free Passed";
		}
		else
		{
			LOGINFO() << "[Error] Merge on Free Failed";
		}
	}

	// 5. Full Cycle - allocate several blocks, free all, whole arena should be reclaimable
	{
		LOGINFO() << "[Test 5] Full Cycle (Alloc All -> Free All -> Reclaim Whole Arena)";

		const size_t totalSize = 1024 * 1024;
		wtr::TLSFArena arena;
		arena.Init(totalSize);

		auto x = arena.Allocate(10000);
		auto y = arena.Allocate(20000);
		auto z = arena.Allocate(5000);

		// Free in a mixed order (not purely sequential) to exercise all merge directions
		arena.Free(y);
		arena.Free(x);
		arena.Free(z);

		auto full = arena.Allocate(totalSize);

		LOGINFO() << "Full Reclaim : offset=" << full.offset << " size=" << full.size;

		if (full.offset == 0 && full.size == totalSize)
		{
			LOGINFO() << "Full Cycle Passed (no leaks, no fragmentation)";
		}
		else
		{
			LOGINFO() << "[Error] Full Cycle Failed - possible leak or fragmentation";
		}
	}

	// 6. Exact-Size Match Allocate - reusing an isolated free block that exactly fits
	{
		LOGINFO() << "[Test 6] Exact-Size Match Allocate & Double-Allocation Guard";

		wtr::TLSFArena arena;
		arena.Init(1024 * 1024);

		auto a = arena.Allocate(2048);
		auto b = arena.Allocate(2048);
		auto c = arena.Allocate(2048);

		// B is sandwiched between two still-allocated blocks, so freeing it
		// cannot merge with either neighbor - it stays an isolated, exact-size free block.
		arena.Free(b);

		auto d = arena.Allocate(2048);

		LOGINFO() << "B (freed) : offset=" << b.offset << " size=" << b.size;
		LOGINFO() << "D (reallocated) : offset=" << d.offset << " size=" << d.size;

		if (d.offset == b.offset && d.size == b.size)
		{
			LOGINFO() << "Exact-Size Match Passed";
		}
		else
		{
			LOGINFO() << "[Error] Exact-Size Match Failed";
		}

		// D is now allocated again (not free). Requesting the same size again must NOT
		// hand back the same block - this guards against the exact-match double-allocation bug.
		auto e = arena.Allocate(2048);
		LOGINFO() << "E (should differ from D) : offset=" << e.offset << " size=" << e.size;

		if (e.offset != d.offset)
		{
			LOGINFO() << "Double-Allocation Guard Passed";
		}
		else
		{
			LOGINFO() << "[Error] Double-Allocation Detected!";
		}
	}

	// 7. Multiple Simultaneous Free Blocks Sharing One Size Bucket
	{
		LOGINFO() << "[Test 7] Multiple Free Blocks Sharing One Size Bucket";

		wtr::TLSFArena arena;
		arena.Init(1024 * 1024);

		// Wall off each candidate with an allocated neighbor so freeing it
		// can never merge away - forces a,b,c to stay free at the exact same time.
		auto a = arena.Allocate(1024);      // offset 0
		auto wall1 = arena.Allocate(1024);  // offset 1024, stays allocated
		auto b = arena.Allocate(1024);      // offset 2048
		auto wall2 = arena.Allocate(1024);  // offset 3072, stays allocated
		auto c = arena.Allocate(1024);      // offset 4096
		auto wall3 = arena.Allocate(1024);  // offset 5120, stays allocated

		arena.Free(a);
		arena.Free(b);
		arena.Free(c);

		LOGINFO() << "Freed A : offset=" << a.offset;
		LOGINFO() << "Freed B : offset=" << b.offset;
		LOGINFO() << "Freed C : offset=" << c.offset;

		// All three now sit free, at the same time, in the same size bucket.
		auto d = arena.Allocate(1024);
		auto e = arena.Allocate(1024);
		auto f = arena.Allocate(1024);

		LOGINFO() << "D : offset=" << d.offset;
		LOGINFO() << "E : offset=" << e.offset;
		LOGINFO() << "F : offset=" << f.offset;

		std::vector<size_t> expected = { a.offset, b.offset, c.offset };
		std::vector<size_t> actual = { d.offset, e.offset, f.offset };

		std::sort(expected.begin(), expected.end());
		std::sort(actual.begin(), actual.end());

		if (expected == actual)
		{
			LOGINFO() << "Multiple Free Blocks Passed (all three reused correctly, no loss/duplication)";
		}
		else
		{
			LOGINFO() << "[Error] Multiple Free Blocks Failed - mismatch between freed and reallocated offsets";
		}
	}

	LOGINFO() << "========== TLSFArena Test End ==========";
}

void VariantTest()
{
	LOGINFO() << "[ Variant Test ]";
	{
		LOGINFO() << "------ Base Type Test ------";

		wtr::Variant<int, float, double> var;

		var.Set(1.0f);

		LOGINFO() << "Float Check : " << var.Is<float>();
		LOGINFO() << "Float Value : " << var.Get<float>();
		LOGINFO() << "Float Index : " << var.GetIndex();

		var.Set(2.0);

		LOGINFO() << "Double Check : " << var.Is<double>();
		LOGINFO() << "Double Value : " << var.Get<double>();
		LOGINFO() << "Double Index : " << var.GetIndex();
	}

	{
		LOGINFO() << "------ Copy Test ------";

		wtr::Variant<int, float, double> var;

		var.Set(100);

		wtr::Variant<int, float, double> var1 = var;

		LOGINFO() << "Copy Check : " << var1.Is<int>();
		LOGINFO() << "Copy Value : " << var1.Get<int>();

	}

	{
		LOGINFO() << "------ Object Destruct Test ------";

		class Object
		{
		public:
			Object()
			{
				LOGINFO() << "Object Constructor!";
			}
			~Object()
			{
				LOGINFO() << "Object Destructor!";
			}
		};

		wtr::Variant<int, Object> var2;

		var2.Set(Object());
		LOGINFO() << "Object Check : " << var2.Is<Object>();

		var2.Set(1);
		LOGINFO() << "Int Check: " << var2.Is<int>();
	}

	{
		LOGINFO() << "------ STL Container Test ------";
		wtr::Variant<std::vector<int>, std::vector<float>, std::vector<double>> var3;
		var3.Set(std::vector<int>{ 1, 2, 3, 4, 5 });
		LOGINFO() << "Vector Check : " << var3.Is<std::vector<int>>();
		LOGINFO() << "Vector Value of Index 0 : " << var3.Get<std::vector<int>>()[0];
	}

	{
		LOGINFO() << "------ Deep Copy Test ------";
		struct Object
		{
			std::shared_ptr<Object> spChild;
			int Value;

			Object()
				: spChild(nullptr)
				, Value(0)
			{
			}

			Object(const Object& _other)
				: spChild(_other.spChild)
				, Value(_other.Value)
			{
			}
		};

		wtr::Variant<int, std::string, Object> var1;
		wtr::Variant<int, std::string, Object> var2;
		wtr::Variant<int, std::string, Object> var3;

		var1.Set<std::string>("Hello World!");

		var2 = var1;

		LOGINFO() << "Copy String : " << var2.Get<std::string>();

		var3 = std::move(var1);

		LOGINFO() << "Move String : " << var2.Get<std::string>();
	}

	{
		LOGINFO() << "------ Hash Value Test ------";
		wtr::Variant<int, float, double> var4, var5, var6, var7;
		var4.Set(1.0f);
		var5.Set(1.2f);
		var6.Set(0.0f);
		var7.Set(2.0f);

		LOGINFO() << "Hash Value : " << var4.GetHash();
		LOGINFO() << "Hash Value : " << var5.GetHash();
		LOGINFO() << "Hash Value : " << var6.GetHash();
		LOGINFO() << "Hash Value : " << var7.GetHash();

		wtr::HashSet<wtr::Variant<int, float, double>> set;

		set.Insert(var4);
		set.Insert(var5);
		set.Insert(var6);
		set.Insert(var7);

		for (auto& iter : set)
		{
			LOGINFO() << "Value : " << iter.Get<float>() << " | Hash Value : " << iter.GetHash();
		}

		struct Object
		{
			using ObjectList = std::vector<std::shared_ptr<Object>>;
			using Value = wtr::Variant<bool, int, float, double, std::string, ObjectList>;

			Object() {};

			Value m_Value;
		};

		wtr::Variant<std::string, int, Object, std::vector<float>> var8, var9;
		var8.Set(std::string("Hello World!"));
		var9.Set(Object());
		LOGINFO() << "Hash Value : " << var8.GetHash();
		LOGINFO() << "Hash Value : " << var9.GetHash();
	}
}

int MAIN()
{
	Log::Init(1024, Log::Enum::eMode_Print | Log::Enum::eMode_Save, Log::Enum::eLevel_Type);

	ListTest();
	StaticArrayTest();
	DynamicArrayTest();
	HashSetTest();
	HashMapTest();
	ArenaTest();
	LinearArenaTest();
	TLSFArenaTest();
	VariantTest();

	system("pause");

	return 0;
}
