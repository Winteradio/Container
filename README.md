# WTR Container Library

A high-performance, custom C++ container library designed for game engine development and performance-critical applications. This library provides STL-like containers with control over memory allocation via a custom `Arena` allocator and robust implementations of common data structures.

## 📦 Features

  * **Namespace**: All classes are encapsulated within the `wtr` namespace.
  * **Custom Memory Management**: Most containers support a custom `Allocator` template argument (defaulting to `wtr::Arena`).
  * **STL-Compatible Iterators**: Supports range-based for loops, forward/reverse iterators, and const iterators.
  * **Header-Only (mostly)**: Template-heavy design for easy integration.

## 🛠 Containers & Utilities

| Component | Description | Implementation Details |
| :--- | :--- | :--- |
| **`DynamicArray`** | Resizable array (like `std::vector`). | Supports `EmplaceBack`, `Reserve`, and move semantics. |
| **`StaticArray`** | Fixed-size array (like `std::array`). | Compile-time size check, stack-allocated storage. |
| **`List`** | Doubly Linked List (like `std::list`). | Supports `Splice`, `Remove`, and efficient insertions. |
| **`HashMap`** | Key-Value store (like `std::unordered_map`). | **Robin Hood Hashing** (Open Addressing) for cache locality. |
| **`HashSet`** | Unique key set (like `std::unordered_set`). | **Robin Hood Hashing** (Open Addressing). |
| **`Variant`** | Type-safe union (like `std::variant`). | Supports types with non-trivial destructors and deep copying. |
| **`Arena`** | Memory Allocator. | Wrapper for allocation strategies. |

## 🚀 Getting Started

### Prerequisites

  * C++17 or higher (Required for `if constexpr`, `std::conditional_t`, etc.).

### Installation

1.  Copy the `include/` directory to your project.
2.  Include the necessary headers.
3.  Ensure `Arena.cpp` is compiled with your project implementation.

-----

## 📖 Usage Examples

### 1\. HashMap

A high-performance hash map using Robin Hood hashing to minimize variance in probe lengths.

```cpp
#include "HashMap.h"
#include <iostream>

void ExampleHashMap() {
    // Key: int, Value: string
    wtr::HashMap<int, std::string> map;

    // Insertion
    map[1] = "Apple";
    map.Emplace(2, "Banana");

    // Modification
    map[1] = "Apricot";

    // Iteration
    for (const auto& pair : map) {
        std::cout << "ID: " << pair.first << ", Name: " << pair.second << std::endl;
    }

    // Safe Access
    if (map.Find(2) != map.End()) {
        std::cout << "Found: " << map.At(2) << std::endl;
    }
}
```

#### Custom Struct Keys

To use custom structs as keys, define a **Hasher** and a **Comparer** (optional if `operator==` is defined).

```cpp
struct Point { 
    int x, y; 
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

struct PointHasher {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

wtr::HashMap<Point, int, PointHasher> pointMap;
```

### 2\. HashSet

A set implementation sharing the same underlying Robin Hood Hash Table logic as `HashMap`.

```cpp
#include "HashSet.h"

void ExampleHashSet() {
    wtr::HashSet<int> set;
    
    set.Insert(10);
    set.Insert(20);
    set.Insert(10); // Duplicate, will be ignored

    // Removal
    set.Erase(20);

    for (const auto& val : set) {
        // ...
    }
}
```

### 3\. DynamicArray

A contiguous dynamic array similar to `std::vector`.

```cpp
#include "DynamicArray.h"

void ExampleArray() {
    wtr::DynamicArray<int> arr;
    arr.Reserve(10);

    arr.PushBack(1);
    arr.EmplaceBack(2);

    // Range-based for loop
    for (int val : arr) {
        // ...
    }

    // Random Access
    arr[0] = 5;
}
```

### 4\. Variant

A type-safe union that can store one of several specified types. It handles object lifecycles (constructors/destructors) automatically.

```cpp
#include "Variant.h"
#include <vector>

void ExampleVariant() {
    // Can hold an int, a float, or a vector of ints
    wtr::Variant<int, float, std::vector<int>> var;

    var.Set(10);
    bool isInt = var.Is<int>(); // true

    var.Set(3.14f);
    float val = var.Get<float>();

    // Supports complex types
    var.Set(std::vector<int>{1, 2, 3});
}
```

### 5\. StaticArray

A wrapper around a raw array providing bounds checking (via assert) and STL-compatible iterators.

```cpp
#include "StaticArray.h"

void ExampleStaticArray() {
    // Fixed size of 5
    Memory::StaticArray<int, 5> arr = {1, 2, 3}; // Remaining elements zero-initialized

    arr[0] = 10;
    
    // Bounds check asserts in debug mode
    // arr[10] = 5; // Crashes
}
```

-----

## ⚙️ Architecture Details

### Robin Hood Hashing (`HashTable.h`)

The `HashMap` and `HashSet` utilize **Open Addressing** with **Robin Hood Hashing**.

  * **Slot Structure**: Stores `Data` and `psl` (Probe Sequence Length).
  * **Insertion Logic**: If a new element has a higher probe count (PSL) than the element currently occupying a slot, they are swapped. The displaced element continues to look for a spot.
  * **Benefit**: Reduces the variance of search times and improves cache hits compared to standard chaining or linear probing.

### Variant (`Variant.h`)

Implemented using `AlignedStorage` and variadic templates. It uses a recursive `TypeMatcher` struct to handle Copy, Move, and Destroy operations for the active type in the storage union.

### Custom Allocation (`Arena`)

The containers allow injecting an `Allocator` type.

  * Default: `wtr::Arena`
  * Functions: `Allocate(size)`, `Deallocate(ptr)`
  * This structure allows for easy replacement with Pool Allocators or Stack Allocators for engine integration.

-----

## 📄 License
This project is open-source and licensed under the MIT License.
