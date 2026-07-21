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
| **`Arena`** | Tracked heap allocator. | Each `Allocate` is a separate `::operator new`, tracked in a `Page` linked list so it can be individually `Deallocate`d or bulk-released on destruction. |
| **`LinearArena`** | Bump (linear) allocator. | Grows fixed-size pages and hands out offsets from them; no per-object free, only a bulk `Reset()`. Good for scratch/frame-lifetime allocations. |
| **`TLSFArena`** | Two-Level Segregated Fit suballocator. | O(1) allocate/free/split/merge over a fixed-size byte range via a FL/SL size-class bitmap + free-list, independent of the underlying memory (see the diagrams in `TLSFArena.h`). |

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
    wtr::StaticArray<int, 5> arr = {1, 2, 3}; // Fewer elements than Count -> remaining
                                               // slots repeat the LAST given value (here: 3, 3)

    arr[0] = 10;
    
    // Bounds check asserts in debug mode
    // arr[10] = 5; // Crashes
}
```

### 6\. TLSFArena

A Two-Level Segregated Fit suballocator over an abstract `[0, totalSize)` byte range. It never touches real memory itself - callers map the returned `offset` onto whatever storage they own (a raw buffer, a `VkDeviceMemory`, etc.), which is what lets the same arena be reused for CPU or GPU-backed memory.

```cpp
#include "TLSFArena.h"

void ExampleTLSFArena() {
    wtr::TLSFArena arena;
    arena.Init(1024 * 1024); // 1MB

    auto a = arena.Allocate(4096);
    auto b = arena.Allocate(8192);

    arena.Free(a);

    // a's space is reusable immediately; adjacent free Blocks merge back
    // together automatically on Free() - see TLSFArena.h for the full
    // FL/SL bitmap + free-list design (diagrams included in the header).
    auto c = arena.Allocate(4096);
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

### Custom Allocation (`Arena` / `LinearArena`)

The containers allow injecting an `Allocator` type (default: `wtr::Arena`).

  * **`Arena`** - a concrete tracked allocator, not a strategy wrapper. Every `Allocate(size)` is its own `::operator new`, recorded in a doubly-linked list of `Page` records; `Deallocate(ptr)` walks that list to find and free the matching page, and any pages still outstanding are released together when the `Arena` itself is destroyed.
  * **`LinearArena`** - a separate, distinct allocator (not swapped in via the same `Allocator` slot as `Arena` - it has no per-pointer `Deallocate`). It carves memory out of large pages sequentially and only supports releasing everything at once via `Reset()`, making it a better fit for short-lived, frame-scoped allocations than for containers that need individual items freed.
  * Plugging in your own allocator (e.g. a pool allocator) just means matching whichever interface the container's `Allocator` template parameter expects.

### TLSF Suballocation (`TLSFArena.h`)

`TLSFArena` manages a `[0, totalSize)` size range using the **Two-Level Segregated Fit** algorithm - the same class of algorithm used by production GPU suballocators (e.g. AMD's VMA).

  * **First/Second-Level index (FL/SL)**: `size` is classified by its most-significant-bit position (FL) and a linear subdivision within that power-of-two range (SL), giving an O(1) size-class lookup instead of a linear scan.
  * **Bitmap search**: one bit per size class (`m_bitMap`) lets `Allocate()` find the nearest non-empty class via bit masking + lowest-set-bit, without touching `m_blocks` until a candidate is already known.
  * **Two independent link pairs per Block**: `prev/next` (physical neighbors, always valid - used to merge on `Free()`) and `prevFree/nextFree` (size-class neighbors, only valid while free - used by `Allocate()`'s free-list). They track two unrelated groupings of the same Block and cannot be collapsed into one pair.
  * **Storage-agnostic**: the arena only ever deals in integer offsets - it has no notion of CPU vs GPU memory, which is why it is reusable as the core of a GPU suballocator (e.g. wrapped by a `VkDeviceMemory`-owning block) without any GPU-specific code inside `TLSFArena` itself.
  * Full ASCII-art diagrams of the physical chain and the size-class free-list are kept as comments directly in `TLSFArena.h`, next to the code they describe.

-----

## 📄 License
This project is open-source and licensed under the MIT License.
