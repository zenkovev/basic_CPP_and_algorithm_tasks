#include <iterator>
#include <memory>
#include <iostream>
#include <assert.h>

template<size_t N>
class StackStorage {

private:
	char stack_memory[N] alignas(16);
	//with maximal alignment
	size_t capacity;
	size_t first_free_byte;
	//first_free_byte show first free number of element in array
	//previous elements don't used after free
	//if no memory, first_free_byte == capacity

public:
	StackStorage() {
		capacity = N;
		first_free_byte = 0;
	}

	StackStorage(const StackStorage& ss) = delete;

	char* reserve(size_t bytes, size_t alignment = 1) {
		//if there no memory, don't change something, return nullptr
		assert(bytes != 0);
		assert(alignment != 0);
		size_t first_free_byte_tmp = first_free_byte;
		size_t shift = first_free_byte_tmp % alignment;
		if (shift != 0)
			first_free_byte_tmp += alignment - shift;
		size_t pointer = first_free_byte_tmp;
		//return as pointer on memory
		first_free_byte_tmp += bytes;
		if (first_free_byte_tmp >= capacity)
			return nullptr;
		first_free_byte = first_free_byte_tmp;
		return stack_memory+pointer;
	}

};

template<typename T, size_t N>
class StackAllocator {

private:
	StackStorage<N>* source_of_memory;
	
	template<typename U, size_t M>
	friend class StackAllocator;
	
public:
	StackAllocator() = delete;
	
	StackAllocator(StackStorage<N>& ss) {
		source_of_memory = &ss;
	}

	T* allocate(size_t n) {
		char* pointer = source_of_memory->reserve(n*sizeof(T), alignof(T));
		if (pointer == nullptr)
			throw "StackAllocator doesn't have memory";
		return reinterpret_cast<T*>(pointer);
	}

	void deallocate(T* pointer, size_t n) {
		//fighting the stupid warnings about unused parameters
		if (pointer != nullptr)
			assert(pointer != nullptr);
		assert(n >= 0);
		//fighting the stupid warnings about unused parameters
	}

	using value_type = T;

	template<typename U>
	struct rebind {
		using other = StackAllocator<U, N>;
	};

	template<typename U>
	StackAllocator(const StackAllocator<U, N>& sa) {
		source_of_memory = sa.source_of_memory;
	}

	StackAllocator& operator=(const StackAllocator& sa) {
		source_of_memory = sa.source_of_memory;
		return *this;
	}
	
	bool operator==(const StackAllocator& sa) const {
		return source_of_memory == sa.source_of_memory;
	}
	
	bool operator!=(const StackAllocator& sa) const {
		return !(*this == sa);
	}

};

template<typename T, typename Allocator = std::allocator<T>>
class List {

private:

	struct BaseNode { 
		BaseNode* previous;
		BaseNode* next;
		
		BaseNode() {
			previous = this;
			next = this;
		}
	};
	//keep in stack
	
	struct Node : BaseNode {
		T value;
	};
	//keep in allocator
	
	using AllocTraits = typename std::allocator_traits<Allocator>;
	using AllocNode = typename Allocator::template rebind<Node>::other;
	using AllocNodeTraits = typename std::allocator_traits<AllocNode>;

	BaseNode root_node;
	Allocator alloc_original_type;
	//allocator of T
	AllocNode alloc_node;
	//allocator of Node
	size_t list_size;
	
	inline Node ItIsNode(BaseNode node_as_base_node) {
		return dynamic_cast<Node>(node_as_base_node);
	}
	
	inline Node* ItIsNode(BaseNode* node_as_base_node) {
		return static_cast<Node*>(node_as_base_node);
	}
	
	template<class... Args>
	BaseNode* push_element_after_here(BaseNode* after_here, Args&&... args) {
		//return pointer on new element
		//work with correct list
		Node* tmp_place_of_element = AllocNodeTraits::allocate(alloc_node, 1);
		//allocate memory for new element
		//if throw all is OK
		try {
			//new(&(tmp_place_of_element->value)) T(value);
			AllocTraits::construct(alloc_original_type, &(tmp_place_of_element->value), args...);
		} catch(...) {
			AllocNodeTraits::deallocate(alloc_node, tmp_place_of_element, 1);
			throw;
		}
		//all throw are avoiding
		//now have new object: tmp_place_of_element
		tmp_place_of_element->previous = after_here;
		tmp_place_of_element->next = after_here->next;
		//all new objects is correct
		after_here->next->previous = tmp_place_of_element;
		//element after new is correct
		after_here->next = tmp_place_of_element;
		//element before new is correct
		++list_size;
		//all positions is correct
		return tmp_place_of_element;
	}

	void pop_element_here(BaseNode* element_here) {
		element_here->previous->next = element_here->next;
		//element before this is correct
		BaseNode* pointer_for_delete = element_here->next->previous;
		//this pointer will be deallocated
		element_here->next->previous = element_here->previous;
		//element after this is correct
		AllocTraits::destroy(alloc_original_type, &(ItIsNode(pointer_for_delete)->value));
		AllocNodeTraits::deallocate(alloc_node, ItIsNode(pointer_for_delete), 1);
		//all is cleaned
		--list_size;
		//all positions is correct
	}

	void make_void_part_of_list() {
		//create a correct node and list_size for void list
		//does not work with allocator fields
		//think that allocator fields is correct
		//think that list does not have nodes except basenode
		root_node.previous = &root_node;
		root_node.next = &root_node;
		list_size = 0;
	}

	void make_clean_list() {
		//create a list of zero elements from current condition
		//work with correct list
		if (list_size == 0)
			return;
		//list_size >= 1
		BaseNode* element_after_deleted = (&root_node)->next->next;
		while (element_after_deleted != &root_node) {
			AllocTraits::destroy(alloc_original_type, &(ItIsNode(element_after_deleted->previous)->value));
			AllocNodeTraits::deallocate(alloc_node, ItIsNode(element_after_deleted->previous), 1);
			element_after_deleted = element_after_deleted->next;
		}
		AllocTraits::destroy(alloc_original_type, &(ItIsNode(root_node.previous)->value));
		AllocNodeTraits::deallocate(alloc_node, ItIsNode(root_node.previous), 1);
		//here we have only root_node with non_correct pointers
		make_void_part_of_list();
		return;
	}

	template<class... Args>
	void create_all_list(size_t number, Args... args) {
		//called in constructor after creating a void list
		BaseNode* last_element = &root_node;
		for (size_t i = 0; i < number; ++i) {
			try {
				last_element = push_element_after_here(last_element, args...);
			} catch(...) {
				make_clean_list();
				throw;
			}
		}
		//we used correct list methods than have a correct list!!!
	}
	
	BaseNode* copy_chain_of_node(
			BaseNode* push_chain_after_this_element,
			//push_chain_after_this_element is in this list
			BaseNode* first_non_void_element_in_chain,
			BaseNode* last_non_void_element_in_chain
			//this two nodes is in one list, usually it is other list than this list
			//chain does not contain basenode
			//if chain does not const in coping process undefined behaviour
			//if throw list can be changed but list will correct
			) {
		//return pointer on last element of created chain
		while (first_non_void_element_in_chain != last_non_void_element_in_chain) {
			push_chain_after_this_element = push_element_after_here(push_chain_after_this_element, ItIsNode(first_non_void_element_in_chain)->value);
			first_non_void_element_in_chain = first_non_void_element_in_chain->next;
		}
		push_chain_after_this_element = push_element_after_here(push_chain_after_this_element, ItIsNode(first_non_void_element_in_chain)->value);
		return push_chain_after_this_element;
	}

	template <bool flag_of_const, typename type_of_const>
	class all_iterator;

public:

	using iterator = all_iterator<false, T>;
	using const_iterator = all_iterator<true, const T>;

private:
	
	template <bool flag_of_const, typename type_of_const>
	class all_iterator {
	
	private:
		const List* main_list;
		BaseNode* index_node;
		size_t index_number;

		template <bool flag_of_const_tmp, typename type_of_const_tmp>
		friend class all_iterator;
		
		friend void List<T, Allocator>::insert(List<T, Allocator>::const_iterator, const T&);
		friend void List<T, Allocator>::erase(List<T, Allocator>::const_iterator);
		
	public:
		all_iterator() = delete; 
		
		all_iterator(const List& list, const BaseNode* node, size_t number) {
			main_list = &list;
			index_node = const_cast<BaseNode*>(node);
			index_number = number;
		}

		all_iterator(const all_iterator<false, T>& it) {
			main_list = it.main_list;
			index_node = it.index_node;
			index_number = it.index_number;
		}
		
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = type_of_const;
		using difference_type = int;
		using pointer = type_of_const*;
		using reference = type_of_const&;

		all_iterator& operator++() {
			index_node = index_node->next;
			++index_number;
			return *this;
		}

		all_iterator& operator--() {
			index_node = index_node->previous;
			--index_number;
			return *this;
		}

		all_iterator operator++(int) {
			all_iterator a(*this);
			++(*this);
			return a;
		}

		all_iterator operator--(int) {
			all_iterator a(*this);
			--(*this);
			return a;
		}

		all_iterator& operator+=(int right) {
			if (right < 0) {
				right *= -1;
				for (int i = 0; i < right; ++i) {
					--(*this);
				}
			} else {
				//right >= 0
				for (int i = 0; i < right; ++i) {
					++(*this);
				}
			}
			return *this;
		}
		
		all_iterator& operator-=(int right) {
			return (*this)+=(-right);
		}

		all_iterator operator+(int right) const {
			all_iterator centre(*this);
			return centre += right;
		}

		all_iterator operator-(int right) const {
			all_iterator centre(*this);
			return centre -= right;
		}

		bool operator==(const all_iterator& r) const {
			return index_number == r.index_number;
		}

		bool operator!=(const all_iterator& r) const {
			return !(*this == r);
		}

		bool operator<(const all_iterator& r) const {
			return index_number < r.index_number;
		}

		bool operator>(const all_iterator& r) const {
			return !((*this < r) || (*this == r));
		}

		bool operator<=(const all_iterator& r) const {
			return (*this < r) || (*this == r);
		}

		bool operator>=(const all_iterator& r) const {
			return (*this > r) || (*this == r);
		}

		int operator-(const all_iterator& r) const {
			return static_cast<int>(index_number) - r.index_number;
		}

		type_of_const* operator->() const {
			return &((static_cast<Node*>(index_node))->value);
		}

		type_of_const& operator*() const {
			return (static_cast<Node*>(index_node))->value;
		}
	
	};

public:

	List() :
		root_node(),
		alloc_original_type(Allocator()),
		alloc_node(Allocator()),
		list_size(0)
	{}
	
	List(size_t number) :
		root_node(),
		alloc_original_type(Allocator()),
		alloc_node(Allocator()),
		list_size(0)
	{
		create_all_list(number);
	}
	
	List(size_t number, const T& value) :
		root_node(),
		alloc_original_type(Allocator()),
		alloc_node(Allocator()),
		list_size(0)
	{
		create_all_list(number, value);
	}
	
	List(const Allocator& tmp_alloc) :
		root_node(),
		alloc_original_type(tmp_alloc),
		alloc_node(tmp_alloc),
		list_size(0)
       	{}
	
	List(size_t number, const Allocator& tmp_alloc) :
		root_node(),
		alloc_original_type(tmp_alloc),
		alloc_node(tmp_alloc),
		list_size(0)
	{
		create_all_list(number);
	}
	
	List(size_t number, const T& value, const Allocator& tmp_alloc) :
		root_node(),
		alloc_original_type(tmp_alloc),
		alloc_node(tmp_alloc),
		list_size(0)
	{
		create_all_list(number, value);
	}
	
	Allocator get_allocator() const {
		return alloc_original_type;
	}

	size_t size() const {
		return list_size;
	}

	~List() {
		make_clean_list();
	}

	List(const List& list_for_copy) :
		root_node(),
		alloc_original_type(AllocTraits::select_on_container_copy_construction(list_for_copy.alloc_original_type)),
		alloc_node(AllocTraits::select_on_container_copy_construction(list_for_copy.alloc_original_type)),
		list_size(0)
	{
		if (list_for_copy.list_size != 0) {
			try {
				copy_chain_of_node(&root_node, list_for_copy.root_node.next, list_for_copy.root_node.previous);
			} catch(...) {
				make_clean_list();
				throw;
			}
		}
	}

	List(const List& list_for_copy, const Allocator& allocator_for_copy) :
                root_node(),
                alloc_original_type(allocator_for_copy),
                alloc_node(allocator_for_copy),
                list_size(0)
        {
                if (list_for_copy.list_size != 0) {
                        try {
                                copy_chain_of_node(&root_node, list_for_copy.root_node.next, list_for_copy.root_node.previous);
                        } catch(...) {
                                make_clean_list();
                                throw;
                        }
                }
        }

	List& operator=(const List& list_for_copy) {
		const Allocator& allocator_for_copy = AllocTraits::propagate_on_container_copy_assignment::value ?
			list_for_copy.alloc_original_type : alloc_original_type;
		List list_tmp(list_for_copy, allocator_for_copy);
		//if throw all is OK
		make_clean_list();
		root_node = list_tmp.root_node;
		root_node.previous->next = &root_node;
		root_node.next->previous = &root_node;
		alloc_original_type = list_tmp.alloc_original_type;
		alloc_node = list_tmp.alloc_node;
		list_size = list_tmp.list_size;
		//list_tmp must not delete his elements
		list_tmp.make_void_part_of_list();
		return *this;
	}
	
	void print_all_information() {
		std::cout << "----------" << '\n';
		std::cout << "List" << '\n';
		std::cout << "Size" << ' ' << size() << '\n';
		std::cout << "-----" << '\n';
		std::cout << "BaseNode" << '\n';
		std::cout << "previous" << ' ' << root_node.previous << '\n';
		std::cout << "next" << ' ' << root_node.next << '\n';
		std::cout << "-----" << '\n';
		BaseNode* pointer = root_node.next;
		while (pointer != &root_node) {
			Node* real_pointer = static_cast<Node*>(pointer);
			std::cout << "-----" << '\n';
			std::cout << "Node" << '\n';
			std::cout << "&value" << ' ' << &real_pointer->value << '\n';
			std::cout << "previous" << ' ' << real_pointer->previous << '\n';
			std::cout << "next" << ' ' << real_pointer->next << '\n';
			std::cout << "-----" << '\n';
			pointer = pointer->next;
		}
		std::cout << "----------" << '\n';
	}
	
	void print_all_information_value() {
		std::cout << "----------" << '\n';
		std::cout << "List" << '\n';
		std::cout << "Size" << ' ' << size() << '\n';
		std::cout << "-----" << '\n';
		std::cout << "BaseNode" << '\n';
		std::cout << "previous" << ' ' << root_node.previous << '\n';
		std::cout << "next" << ' ' << root_node.next << '\n';
		std::cout << "-----" << '\n';
		BaseNode* pointer = root_node.next;
		while (pointer != &root_node) {
			Node* real_pointer = static_cast<Node*>(pointer);
			std::cout << "-----" << '\n';
			std::cout << "Node" << '\n';
			std::cout << "&value" << ' ' << &real_pointer->value << '\n';
			std::cout << "value" << ' ' << real_pointer->value << '\n';
			std::cout << "previous" << ' ' << real_pointer->previous << '\n';
			std::cout << "next" << ' ' << real_pointer->next << '\n';
			std::cout << "-----" << '\n';
			pointer = pointer->next;
		}
		std::cout << "----------" << '\n';
	}

	void push_back(const T& value) {
		push_element_after_here(root_node.previous, value);
	}

	void push_front(const T& value) {
		push_element_after_here(&root_node, value);
	}

	void pop_back() {
		if (list_size == 0)
			return;
		pop_element_here(root_node.previous);
		return;
	}

	void pop_front() {
		if (list_size == 0)
			return;
		pop_element_here(root_node.next);
		return;
	}

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	iterator begin() {
		iterator it(*this, root_node.next, 0);
		return it;
	}

	const_iterator begin() const {
		const_iterator it(*this, root_node.next, 0);
		return it;
	}

	const_iterator cbegin() const {
		const_iterator it(*this, root_node.next, 0);
		return it;
	}

	iterator end() {
		iterator it(*this, &root_node, list_size);
		return it;
	}

	const_iterator end() const {
		const_iterator it(*this, &root_node, list_size);
		return it;
	}

	const_iterator cend() const {
		const_iterator it(*this, &root_node, list_size);
		return it;
	}

	reverse_iterator rbegin() {
		return std::make_reverse_iterator(end());
	}

	const_reverse_iterator rbegin() const {
		return std::make_reverse_iterator(end());
	}

	const_reverse_iterator crbegin() const {
		return std::make_reverse_iterator(cend());
	}

	reverse_iterator rend() {
		return std::make_reverse_iterator(begin());
	}

	const_reverse_iterator rend() const {
		return std::make_reverse_iterator(begin());
	}

	const_reverse_iterator crend() const {
		return std::make_reverse_iterator(cbegin());
	}

	void insert(const_iterator it, const T& value) {
		push_element_after_here(it.index_node->previous, value);
	}

	void erase(const_iterator it) {
		pop_element_here(it.index_node);
	}

};
