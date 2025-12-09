#include "Variant.h"
#include "DynamicArray.h"
#include "StaticArray.h"
#include "HashSet.h"
#include "HashMap.h"

#include <Log/include/Log.h>
#include <Log/include/LogPlatform.h>

#include <vector>
#include <string>
#include <ostream>

void HashMapTest()
{
	LOGINFO() << "========== HashMap Test Start ==========";

	// 1. Basic Operation Test (int -> std::string)
	// Verification of insertion/modification functions of operator[]
	{
		LOGINFO() << "[Test 1] Int-String Map (Basic Ops)";

		wtr::HashMap<int, std::string> map;

		// A. New insertion using operator[]
		map[1] = "One";
		map[2] = "Two";
		map[10] = "Ten";

		// B. Insertion using Emplace
		map.Emplace(5, "Five");

		// C. Value modification using operator[] (Update)
		map[1] = "Uno"; // "One" -> "Uno"

		// Verification
		if (map.Size() == 4)
		{
			LOGINFO() << "Size Check Passed: 4";
		}

		if (map[1] == "Uno" && map[2] == "Two")
		{
			LOGINFO() << "Value Update Check Passed: 1 -> " << map[1];
		}

		// Iteration Test (Iterator -> pair<Key, Value>)
		for (auto& pair : map)
		{
			// pair.first is Key, pair.second is Value
			LOGINFO() << "Key: " << pair.first << ", Value: " << pair.second;
		}
	}

	// 2. Const Correctness and At() Test
	// Verify read operations work correctly on const objects
	{
		LOGINFO() << "[Test 2] Const Map Access (At)";

		wtr::HashMap<std::string, int> map;
		map["HP"] = 100;
		map["MP"] = 50;

		// Access via Const Reference
		const auto& constMap = map;

		// Note: Standard operator[] works only on non-const to create items,
		// but your implementation might attempt Emplace even on const versions, potentially causing compile errors.
		// Therefore, testing read-only At() is the safest and most accurate approach.

		// constMap["HP"] = 200; // Expected Compile Error (Cannot modify Const Object)

		if (constMap.At("HP") == 100)
		{
			LOGINFO() << "Const At() Read Passed: HP = " << constMap.At("HP");
		}

		// Attempt to access non-existent key (Commented out as it triggers Assert, uncomment to test if needed)
		// constMap.At("Stamina"); 
	}

	// 3. Custom Struct Key/Value Test
	// verify hash function behavior and value modification when using structs as Keys
	{
		LOGINFO() << "[Test 3] Custom Struct Key/Value";

		struct PlayerID
		{
			int uid;
			int serverId;

			PlayerID() = default;
			PlayerID(int _uid, int _serverId) : uid(_uid), serverId(_serverId) {}

			bool operator==(const PlayerID& other) const
			{
				return uid == other.uid && serverId == other.serverId;
			}
		};

		struct PlayerStats
		{
			int hp;
			float speed;

			PlayerStats() = default;
			PlayerStats(int _hp, float _speed) : hp(_hp), speed(_speed) {}
		};

		struct PlayerIDHasher
		{
			size_t operator()(const PlayerID& id) const
			{
				// Simple bitwise operation for hash combination
				return std::hash<int>()(id.uid) ^ (std::hash<int>()(id.serverId) << 1);
			}
		};

		// HashMap Declaration
		wtr::HashMap<PlayerID, PlayerStats, PlayerIDHasher> playerMap;

		PlayerID p1 = { 1001, 1 };
		PlayerID p2 = { 1002, 1 };

		// Insert Data
		playerMap[p1] = { 100, 5.0f };
		playerMap[p2] = { 200, 4.5f };

		// Modify Data (Modify internal member of Value)
		playerMap[p1].hp -= 10; // Take damage

		// Verification
		LOGINFO() << "Player1 Stats: " << playerMap[p1].hp << " " << playerMap[p1].speed; // HP should be 90

		// Erase Test
		playerMap.Erase(p2);

		if (playerMap.Find(p2) == playerMap.End())
		{
			LOGINFO() << "Erase Struct Key Passed";
		}
	}

	// 4. Collision and Load Factor Test (New)
	{
		LOGINFO() << "[Test 4] Collision Handling & Rehash";

		// Force small size to trigger collisions/rehash
		wtr::HashMap<int, int> map;

		// Insert enough elements to likely trigger a rehash
		int count = 100;
		for (int i = 0; i < count; ++i)
		{
			map[i] = i * 10;
		}

		LOGINFO() << "Map Size after 100 insertions: " << map.Size();

		// Check if all elements are present and correct
		bool allFound = true;
		for (int i = 0; i < count; ++i)
		{
			if (map.Find(i) == map.End() || map[i] != i * 10)
			{
				allFound = false;
				break;
			}
		}

		if (allFound)
		{
			LOGINFO() << "All 100 elements found correctly after potential rehash.";
		}
		else
		{
			LOGINFO() << "[Error] Data loss or corruption during insertion/rehash.";
		}
	}

	LOGINFO() << "========== HashMap Test End ==========";
}

void HashSetTest()
{
	wtr::HashMap<std::string, int> map{ {"Hello", 1},{"Bye", 2},{"Good", 0},{"What", 3} };

	LOGINFO() << "========== HashSet Test Start ==========";

	// 1. Basic Operation and Duplicate Check Test (Primitive Type)
	{
		LOGINFO() << "[Test 1] Integer Set (Duplicate Check)";

		// Initializer list creation: Only 3, 1, 2, 4 should be inserted (Duplicates removed)
		wtr::HashSet<int> set{ 3, 1, 2, 3, 4, 2, 2, 4 };

		set.Insert(10);
		set.Insert(15);
		auto result = set.Insert(10); // Attempt duplicate insertion

		// Verification: Since 10 is already present, second should be false
		if (result.second == false)
		{
			LOGINFO() << "Duplicate insertion prevented correctly. Value: " << *result.first;
		}
		else
		{
			LOGINFO() << "[Error] Duplicate insertion allowed!";
		}

		LOGINFO() << "Set Size: " << set.Size(); // Expected: 3,1,2,4,10,15 -> 6 elements
		for (const auto& val : set)
		{
			LOGINFO() << "Val : " << val;
		}
	}

	// 2. String and Range Erase Test (String Type)
	{
		LOGINFO() << "[Test 2] String Set & Range Erase";

		wtr::HashSet<std::string> set;
		set.Insert("Apple");
		set.Insert("Banana");
		set.Insert("Cherry");
		set.Insert("Durian");
		set.Insert("Elderberry");
		set.Insert("Fig");

		// Attempt to erase "Banana" (inclusive) ~ "Elderberry" (exclusive) logic simulation
		// Note: HashSets are unordered, so Range Erase is typically done with iterators.
		// Here, we simulate a range using Find results, but be aware iterator order isn't guaranteed alphabetically.

		LOGINFO() << "Before Erase: " << set.Size();

		set.Erase("Apple");
		LOGINFO() << "After Erase 'Apple': " << set.Size();

		// Attempt to erase non-existent key
		set.Erase("Ghost");

		// Full traversal erasure (Similar behavior to Clear)
		auto it = set.Begin();
		while (it != set.End())
		{
			// LOGINFO() << "Erasing: " << *it;
			it = set.Erase(it);
		}

		if (set.Empty())
		{
			LOGINFO() << "Set is explicitly cleared via Erase loop.";
		}
	}

	// 3. Custom Struct Test (Custom Struct)
	{
		LOGINFO() << "[Test 3] Custom Struct Set";

		struct Vector2
		{
			int x, y;

			Vector2() = default;
			Vector2(int _x, int _y) : x(_x), y(_y) {};
			bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
		};

		struct Vector2Hasher
		{
			size_t operator()(const Vector2& v) const
			{
				// Simple hash combination
				return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
			}
		};

		wtr::HashSet<Vector2, Vector2Hasher> vecSet;

		vecSet.Emplace(1, 1);
		vecSet.Emplace(2, 2);
		vecSet.Emplace(1, 1); // Duplicate (Should be ignored)
		vecSet.Insert({ 3, 3 });

		for (const auto& v : vecSet)
		{
			LOGINFO() << "Vector: " << v.x << " " << v.y;
		}

		if (vecSet.Size() == 3)
		{
			LOGINFO() << "Custom Struct Duplicate Check Passed.";
		}
	}

	// 4. Iterator Safety Test (New)
	{
		LOGINFO() << "[Test 4] Iterator Safety Check";
		wtr::HashSet<int> set = { 1, 2, 3 };

		// Set iterators should be const_iterators effectively.
		// Uncommenting the line below should cause a compile error.
		// *set.begin() = 10; 

		LOGINFO() << "Iterator read access: " << *set.begin();
	}

	LOGINFO() << "========== HashSet Test End ==========";
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

void DynamicArrayTest()
{
	LOGINFO() << "[ DynamicArray Test ]";
	LOGINFO() << " ";

	{
		LOGINFO() << "------ Basic PushBack & Access Test ------";

		wtr::DynamicArray<int> arr;
		arr.Reserve(4);

		LOGINFO() << "Initial Capacity : " << arr.Capacity();

		arr.PushBack(10);
		arr.PushBack(20);
		arr.PushBack(30);

		LOGINFO() << "Size : " << arr.Size();
		LOGINFO() << "Index 0 : " << arr[0];
		LOGINFO() << "Index 1 : " << arr[1];
		LOGINFO() << "Index 2 : " << arr[2];
		LOGINFO() << "Front : " << arr.Front();
		LOGINFO() << "Back : " << arr.Back();
	}

	{
		LOGINFO() << "------ Initializer List & Iterator Test ------";

		wtr::DynamicArray<std::string> arr = { "Apple", "Banana", "Cherry" };

		LOGINFO() << "Size : " << arr.Size();

		int index = 0;
		for (const auto& item : arr)
		{
			LOGINFO() << "Item " << index++ << " : " << item;
		}
	}

	{
		LOGINFO() << "------ Object Lifecycle & Emplace Test ------";

		struct Object
		{
			int m_id;

			Object() : m_id(0) { LOGINFO() << "Object Default Constructor"; }
			Object(int id) : m_id(id) { LOGINFO() << "Object Constructor : " << m_id; }
			~Object() { LOGINFO() << "Object Destructor : " << m_id; }

			Object(const Object& other) : m_id(other.m_id)
			{
				LOGINFO() << "Object Copy Constructor : " << m_id;
			}

			Object(Object&& other) noexcept : m_id(other.m_id)
			{
				other.m_id = -1;
				LOGINFO() << "Object Move Constructor : " << m_id;
			}

			Object& operator=(const Object& other)
			{
				m_id = other.m_id;
				LOGINFO() << "Object Copy Assign : " << m_id;
				return *this;
			}

			Object& operator=(Object&& other) noexcept
			{
				m_id = other.m_id;
				other.m_id = -1;
				LOGINFO() << "Object Move Assign : " << m_id;
				return *this;
			}
		};

		wtr::DynamicArray<Object> arr;

		LOGINFO() << "[PushBack R-Value]";
		arr.PushBack(Object(1)); // Move Constructor Expected

		LOGINFO() << "[EmplaceBack]";
		arr.EmplaceBack(2); // Constructor Expected directly in place

		LOGINFO() << "[PopBack]";
		arr.PopBack(); // Destructor(2) Expected
	}

	{
		LOGINFO() << "------ Copy & Move Semantics Test ------";

		wtr::DynamicArray<int> original = { 1, 2, 3 };

		// Copy Constructor
		wtr::DynamicArray<int> copyArr = original;
		LOGINFO() << "Copy Size : " << copyArr.Size();
		LOGINFO() << "Copy Value[0] : " << copyArr[0];

		// Copy Assignment
		wtr::DynamicArray<int> assignArr;
		assignArr = original;
		LOGINFO() << "Assign Size : " << assignArr.Size();

		// Move Constructor
		wtr::DynamicArray<int> moveArr = std::move(original);
		LOGINFO() << "Move Size : " << moveArr.Size();
		LOGINFO() << "Original Size (After Move) : " << original.Size(); // Should be 0
	}

	{
		LOGINFO() << "------ Insert & Erase Test ------";

		wtr::DynamicArray<int> arr = { 10, 20, 30, 40, 50 };

		// Insert 99 at index 2 (between 20 and 30)
		// Expected: 10, 20, 99, 30, 40, 50
		auto it = arr.begin();
		it++;
		it++; // it points to 30

		LOGINFO() << "Insert 99 before 30";
		arr.Insert(it, 99);

		for (auto val : arr) LOGINFO() << "Val : " << val;

		// Erase 20 (index 1)
		// Expected: 10, 99, 30, 40, 50
		it = arr.begin();
		it++; // points to 20

		LOGINFO() << "Erase 20";
		arr.Erase(it);

		for (auto val : arr) LOGINFO() << "Val : " << val;
	}

	{
		LOGINFO() << "------ Resize & Clear Test ------";

		wtr::DynamicArray<int> arr = { 1, 2, 3 };

		LOGINFO() << "Resize 5";
		arr.Resize(5); // 1, 2, 3, 0, 0
		LOGINFO() << "Size : " << arr.Size() << " | Capacity : " << arr.Capacity();

		LOGINFO() << "Resize 2";
		arr.Resize(2); // 1, 2
		LOGINFO() << "Size : " << arr.Size();

		LOGINFO() << "Clear";
		arr.Clear();
		LOGINFO() << "Size : " << arr.Size() << " | Empty : " << (arr.Empty() ? "True" : "False");
	}
}

void StaticArrayTest()
{
	LOGINFO() << "[ StaticArray Test ]";

	{
		LOGINFO() << "------ Basic Access & Fill Test ------";

		// Create static array of size 5
		Memory::StaticArray<int, 5> arr;

		LOGINFO() << "Size : " << arr.Size(); // Always 5

		// Fill values
		arr.Fill(10);
		LOGINFO() << "After Fill(10), Index 0 : " << arr[0];
		LOGINFO() << "After Fill(10), Index 4 : " << arr[4];

		// Modify individually
		arr[0] = 99;
		arr.Back() = 77;

		LOGINFO() << "Index 0 : " << arr[0];
		LOGINFO() << "Front : " << arr.Front();
		LOGINFO() << "Back : " << arr.Back();
		LOGINFO() << "At(2) : " << arr.At(2);
	}

	{
		LOGINFO() << "------ Initializer List & Iterator Test ------";

		// Initializer list creation (less than size fills remainder with default)
		Memory::StaticArray<std::string, 3> arr = { "Apple", "Banana" };

		LOGINFO() << "Size : " << arr.Size();

		int index = 0;
		for (const auto& item : arr)
		{
			// Should output "Apple", "Banana", "" (empty string)
			LOGINFO() << "Item " << index++ << " : " << (item.empty() ? "[Empty]" : item);
		}
	}

	{
		LOGINFO() << "------ Object Lifecycle & Move Test ------";

		struct Object
		{
			int m_id;

			Object() : m_id(0) { LOGINFO() << "Object Default Constructor"; }
			Object(int id) : m_id(id) { LOGINFO() << "Object Constructor : " << m_id; }
			~Object() { LOGINFO() << "Object Destructor : " << m_id; }

			Object(const Object& other) : m_id(other.m_id)
			{
				LOGINFO() << "Object Copy Constructor : " << m_id;
			}

			Object(Object&& other) noexcept : m_id(other.m_id)
			{
				other.m_id = -1; // Trace moved state
				LOGINFO() << "Object Move Constructor : " << m_id;
			}

			Object& operator=(const Object& other)
			{
				m_id = other.m_id;
				LOGINFO() << "Object Copy Assign : " << m_id;
				return *this;
			}

			Object& operator=(Object&& other) noexcept
			{
				m_id = other.m_id;
				other.m_id = -1;
				LOGINFO() << "Object Move Assign : " << m_id;
				return *this;
			}
		};

		LOGINFO() << "[Create Array with Default Constructors]";
		Memory::StaticArray<Object, 2> arr; // Default Constructor x 2

		LOGINFO() << "[Assign R-Value]";
		arr[0] = Object(10); // Constructor(10) -> Move Assign -> Destructor(10)

		LOGINFO() << "[Assign L-Value]";
		Object obj(20);
		arr[1] = obj; // Copy Assign
	} // Array Destructor x 2, obj Destructor Call

	{
		LOGINFO() << "------ Copy & Move Semantics Test ------";

		Memory::StaticArray<int, 3> original = { 1, 2, 3 };

		// Copy Constructor
		Memory::StaticArray<int, 3> copyArr = original;
		LOGINFO() << "Copy Index 0 : " << copyArr[0];

		// Modify Copy
		copyArr[0] = 999;
		LOGINFO() << "Modified Copy[0] : " << copyArr[0];
		LOGINFO() << "Original[0] (Should be 1) : " << original[0];

		// Move Constructor
		// StaticArray Move is element-wise move, not pointer swap.
		Memory::StaticArray<int, 3> moveArr = std::move(original);
		LOGINFO() << "Move Index 0 : " << moveArr[0];

		// If objects were used, they would be in a moved-from state.
		// Size remains constant for StaticArray.
		LOGINFO() << "Original Size (Always 3) : " << original.Size();
	}

	{
		LOGINFO() << "------ Const Access Test ------";

		const Memory::StaticArray<int, 3> constArr = { 100, 200, 300 };

		LOGINFO() << "Const Front : " << constArr.Front();
		LOGINFO() << "Const Back : " << constArr.Back();
		LOGINFO() << "Const At(1) : " << constArr.At(1);

		// constArr[0] = 500; // Compile Error Check
	}
}

int MAIN()
{
	Log::Init(1024, Log::Enum::eMode_Print | Log::Enum::eMode_Save, Log::Enum::eLevel_Type);

	HashMapTest();
	HashSetTest();
	VariantTest();
	DynamicArrayTest();
	StaticArrayTest();

	system("pause");

	return 0;
}