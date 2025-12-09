#ifndef __WTR_LIST_H__
#define __WTR_LIST_H__

#include <cassert>
#include <utility>
#include <new>
#include <cstddef>

#include <Reflection/include/Utils.h>

namespace wtr
{
	template<typename T>
	class List
	{
	private:
		struct NodeBase
		{
			NodeBase* prev;
			NodeBase* next;

			NodeBase()
				: prev(nullptr)
				, next(nullptr)
			{}

			NodeBase(const NodeBase& other)
				: prev(other.prev)
				, next(other.next)
			{}
		};

		struct Node : NodeBase
		{
			T item;

			Node()
				: NodeBase()
				, item()
			{}

			Node(const Node& other)
				: NodeBase(other)
				, item(other.item)
			{}
		};

	public:
		template<bool Const, bool Reverse>
		class BaseIterator
		{
		public :
			using BaseType = std::conditional_t<Const, const NodeBase, NodeBase>;
			using NodeType = std::conditional_t<Const, const Node, Node>;

		public:
			BaseIterator(BaseType* node)
				: m_node(node)
			{}

			~BaseIterator() = default;

			BaseIterator& operator++()
			{
				if constexpr (Reverse)
				{
					m_node = m_node->prev;
				}
				else
				{
					m_node = m_node->next;
				}

				return *this;
			}

			BaseIterator& operator--()
			{
				if constexpr (Reverse)
				{
					m_node = m_node->next;
				}
				else
				{
					m_node = m_node->prev;
				}

				return *this;
			}

			BaseIterator operator++(int)
			{
				BaseIterator itr = *this;
				++(*this);
				return itr;
			}

			BaseIterator operator--(int)
			{
				BaseIterator itr = *this;
				--(*this);
				return itr;
			}

			bool operator==(const BaseIterator& other) const
			{
				return m_node == other.m_node;
			}

			bool operator!=(const BaseIterator& other) const
			{
				return m_node != other.m_node;
			}

			T* operator->() const
			{
				assert(nullptr != m_node && "The list iterator's node is invalid");

				NodeType* node = static_cast<NodeType*>(m_node);
				return &(node->item);
			}

			T& operator*() const
			{
				assert(nullptr != m_node && "The list iterator's node is invalid");

				NodeType* node = static_cast<NodeType*>(m_node);
				return node->item;
			}

		private:
			friend class List;

			BaseType* m_node;
		};

		using Iterator = BaseIterator<false, false>;
		using ConstIterator = BaseIterator<true, false>;

		using ReverseIterator = BaseIterator<false, true>;
		using ConstReverseIterator = BaseIterator<true, true>;

	public:
		List()
			: m_count(0)
			, m_end()
		{
			m_end.prev = &m_end;
			m_end.next = &m_end;
		}

		List(const std::initializer_list<T>& initList) : List()
		{
			for (auto& element : initList)
			{
				PushBack(element);
			}
		}

		List(const List<T>& other) = delete;

		explicit List(List<T>&& other) noexcept
			: List()
		{
			this->Splice(Begin(), other);
		}

		~List()
		{
			Clear();
		}

		List& operator=(const List<T>& other) = delete;

		List& operator=(List<T>&& other) noexcept
		{
			if (this != &other)
			{
				Clear();
				this->Splice(End(), other);
			}

			return *this;
		}

	public:
		T& Front()
		{
			Node* head = static_cast<Node*>(m_end.next);
			assert(nullptr != head && "List is empty.");
			return head->item;
		}

		T& Back()
		{
			Node* tail = static_cast<Node*>(m_end.prev);
			assert(nullptr != tail && "List is empty.");
			return tail->item;
		}

		const T& Front() const
		{
			Node* head = static_cast<Node*>(m_end.next);
			assert(nullptr != head && "List is empty.");
			return head->item;
		}

		const T& Back() const
		{
			Node* tail = static_cast<Node*>(m_end.prev);
			assert(nullptr != tail && "List is empty.");
			return tail->item;
		}

	public :
		void PushFront(const T& item)
		{
			Iterator begin = Begin();

			Insert(begin, item);
		}

		void PushBack(const T& item)
		{
			Iterator end = End();

			Insert(end, item);
		}

		void PopFront()
		{
			Iterator begin = Begin();

			T beginValue = *begin;

			Erase(begin);
		}

		void PopBack()
		{
			Iterator back = --End();

			T backValue = *back;

			Erase(back);
		}

		void Splice(const Iterator pos, List<T>& other)
		{
			if (this == &other)
			{
				return;
			}

			NodeBase* node = pos.m_node;
			if (nullptr == node || nullptr == node->prev || nullptr == node->next)
			{
				return;
			}

			NodeBase* prev = node->prev;
			NodeBase* head = other.m_end.next;
			NodeBase* tail = other.m_end.prev;

			prev->next = head;
			head->prev = prev;

			node->prev = tail;
			tail->next = node;

			other.m_end.next = &other.m_end;
			other.m_end.prev = &other.m_end;
			m_count += other.m_count;
			other.m_count = 0;
		}

		void Splice(const Iterator pos, List<T>& other, const Iterator itr)
		{
			if (pos == itr)
			{
				return;
			}

			NodeBase* node = pos.m_node;
			if (nullptr == node || nullptr == node->prev)
			{
				return;
			}

			NodeBase* otherNode = itr.m_node;
			if (nullptr == otherNode || nullptr == otherNode->prev || nullptr == otherNode->next)
			{
				return;
			}

			NodeBase* otherPrev = otherNode->prev;
			NodeBase* otherNext = otherNode->next;

			otherPrev->next = otherNext;
			otherNext->prev = otherPrev;

			NodeBase* prev = node->prev;
			prev->next = otherNode;
			otherNode->prev = prev;

			node->prev = otherNode;
			otherNode->next = node;

			other.m_count--;
			m_count++;
		}

		void Splice(const Iterator pos, List<T>& other, const Iterator first, const Iterator last)
		{
			if (pos == last || first == last)
			{
				return;
			}

			NodeBase* node = pos.m_node;
			if (nullptr == node || nullptr == node->prev)
			{
				return;
			}

			NodeBase* firstNode = first.m_node;
			NodeBase* endNode = last.m_node->prev;
			if (nullptr == firstNode || nullptr == firstNode->prev ||
				nullptr == endNode || nullptr == endNode->next)
			{
				return;
			}

			NodeBase* firstPrev = firstNode->prev;
			NodeBase* lastNext = endNode->next;

			firstPrev->next = lastNext;
			lastNext->prev = firstPrev;

			Node* prev = node->prev;

			prev->next = firstNode;
			firstNode->prev = prev;

			node->prev = endNode;
			endNode->next = node;

			if (this != &other)
			{
				NodeBase* current = firstNode;
				while (current != node)
				{
					other.m_count--;
					m_count++;
					current = current->next;
				}
			}
		}

		void Remove(const T& item)
		{
			Iterator itr = Begin();

			while (itr != End())
			{
				if (*itr == item)
				{
					itr = Erase(itr);
				}
				else
				{
					itr++;
				}
			}
		}

		void Clear()
		{
			while (!Empty())
			{
				this->PopFront();
			}
		}

		bool Empty() const
		{
			return &m_end == m_end.prev && &m_end == m_end.next;
		}

		size_t Size() const
		{
			return m_count;
		}

	public :
		Iterator Insert(Iterator pos, const T& item)
		{
			NodeBase* node = pos.m_node;
			if (nullptr == node || nullptr == node->prev || nullptr == node->next)
			{
				return End();
			}

			NodeBase* prev = node->prev;
			Node* newNode = new (std::nothrow) Node;
			if (nullptr == newNode)
			{
				return End();
			}

			newNode->item = item;
			newNode->prev = prev;
			newNode->next = node;

			prev->next = static_cast<NodeBase*>(newNode);
			node->prev = static_cast<NodeBase*>(newNode);

			m_count++;

			return Iterator(newNode);
		}

		Iterator Erase(const Iterator itr)
		{
			if (itr == End())
			{
				return End();
			}

			NodeBase* node = itr.m_node;
			if (nullptr == node || nullptr == node->prev || nullptr == node->next)
			{
				return End();
			}

			NodeBase* prev = node->prev;
			NodeBase* next = node->next;

			prev->next = next;
			next->prev = prev;

			m_count--;

			delete node;

			return Iterator(next);
		}

		Iterator Find(const T& item)
		{
			NodeBase* node = m_end.next;
			while (node != m_end)
			{
				const T& nodeItem = static_cast<Node*>(node)->item;
				if (nodeItem == item)
				{
					return Iterator(node);
				}

				node = node->next;
			}

			return End();
		}

	public :
		// Standard Range Iterator
		Iterator begin() { return Iterator(m_end.next); }
		Iterator end() { return Iterator(&m_end); }
		ConstIterator begin() const { return ConstIterator(m_end.next); }
		ConstIterator end() const {	return Iterator(&m_end); }

		ReverseIterator rbegin() { return ReverseIterator(m_end.prev); }
		ReverseIterator rend() { return ReverseIterator(&m_end); }
		ConstReverseIterator rbegin() const { return ConstReverseIterator(m_end.prev); }
		ConstReverseIterator rend() const { return ConstReverseIterator(&m_end); }

	public :
		Iterator Begin() { return Iterator(m_end.next);	}
		Iterator End() { return Iterator(&m_end); }
		ConstIterator Begin() const { return ConstIterator(m_end.next);	}
		ConstIterator End() const {	return ConstIterator(&m_end); }

		ReverseIterator rBegin() { return ReverseIterator(m_end.prev); }
		ReverseIterator rEnd() { return ReverseIterator(&m_end); }
		ConstReverseIterator rBegin() const { return ConstReverseIterator(m_end.prev); }
		ConstReverseIterator rEnd() const {	return ConstReverseIterator(&m_end); }

	private:
		NodeBase m_end;
		size_t m_count;
	};
};

#endif // __WTR_LIST_H__