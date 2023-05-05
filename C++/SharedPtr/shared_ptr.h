#pragma once
#include <memory>

#include <iostream>
#include <cassert>

//This structures are helpers for class SharedPtr and class WeakPtr
//They does not depend of T than they defined without class SharedPtr and class WeakPtr

//Main idea:
//Memory allocated by allocator which can be default
//Memory allocated as char array
//Memory is used by RealAllocator, Count, ControlBlock, T (can be or not be)
//Deleter is responsible for deleting T, if Deleter does not exist we make our own Deleter
//ControlBlock must be destructed
//RealAllocator destructed itself

//Allocator can have any type as parameter

struct Count {
	size_t shared_ptr_count;
	size_t weak_ptr_count;
};

struct DestructorDeleter {
	//if T is allocated in char array
	bool destroy_object = true;

	template <typename U>
	void operator()(U* u) {
		if (destroy_object) {
			u->~U();
		}
	}
};

template <class Allocator>
struct DestructorAllocatorDeleter {
	//if T is allocated in char array
	Allocator allocator;
	bool destroy_object = true;
	
	DestructorAllocatorDeleter(const Allocator& allocator_tmp) {
		allocator = allocator_tmp;
	}
	
	template <typename U>
	void operator()(U* u) {
		using AllocatorU = typename std::allocator_traits<Allocator>::template rebind_alloc<U>;
		AllocatorU allocator_u(allocator);
		if (destroy_object) {
			using AllocatorTraits = typename std::allocator_traits<AllocatorU>;
			AllocatorTraits::destroy(allocator_u, u);
		}
	}
};

template <class Allocator>
struct AllocatorDeleter {
	//if T is allocated as T by allocator
	Allocator allocator;

	AllocatorDeleter(const Allocator& allocator_tmp) {
		allocator = allocator_tmp;
	}
	
	template <typename U>
	void operator()(U* u) {
		using AllocatorU = typename std::allocator_traits<Allocator>::template rebind_alloc<U>;
		AllocatorU allocator_u(allocator);
		if (u != nullptr) {
			using AllocatorTraits = typename std::allocator_traits<AllocatorU>;
			AllocatorTraits::destroy(allocator_u, u);
		}
		std::allocator_traits<AllocatorU>::deallocate(allocator_u, u, 1);
	}
};

struct BaseControlBlock {
	BaseControlBlock() {}

	virtual void change_condition_of_destroy_object(bool flag) = 0;

	virtual void delete_object_but_not_control_block() = 0;

	virtual bool object_valid() = 0;

	virtual ~BaseControlBlock() = default;
};

template <class Deleter, typename RealT>
struct ControlBlock : public BaseControlBlock {
	RealT* real_pointer;
	//for example, heir, if we have any cast
	Deleter deleter;
	bool is_object_valid = true;

	ControlBlock() = delete;

	ControlBlock(RealT* real_pointer_tmp, const Deleter& deleter_tmp) :
		real_pointer(real_pointer_tmp),
		deleter(deleter_tmp)
	{}

	void change_condition_of_destroy_object(bool flag) override {
		if (flag) {
			assert(flag);
		}
	}

	void delete_object_but_not_control_block() override {
		deleter(real_pointer);
		real_pointer = nullptr;
		is_object_valid = false;
	}

	bool object_valid() override {
		return is_object_valid;
	}
	
	~ControlBlock() override {
		if (is_object_valid) {
			deleter(real_pointer);
		}
		real_pointer = nullptr;
	}
};

template <typename RealT>
struct ControlBlock<DestructorDeleter, RealT> : public BaseControlBlock {
	RealT* real_pointer;
	//for example, heir, if we have any cast
	DestructorDeleter deleter;
	bool is_object_valid = true;

	ControlBlock() = delete;

	ControlBlock(RealT* real_pointer_tmp, const DestructorDeleter& deleter_tmp) :
		real_pointer(real_pointer_tmp),
		deleter(deleter_tmp)
	{}

	void change_condition_of_destroy_object(bool flag) override {
		deleter.destroy_object = flag;
	}

	void delete_object_but_not_control_block() override {
		deleter(real_pointer);
		real_pointer = nullptr;
		is_object_valid = false;
	}

	bool object_valid() override {
		return is_object_valid;
	}
	
	~ControlBlock() override {
		if (is_object_valid) {
			deleter(real_pointer);
		}
		real_pointer = nullptr;
	}
};

template <class Allocator, typename RealT>
struct ControlBlock<DestructorAllocatorDeleter<Allocator>, RealT> : public BaseControlBlock {
	RealT* real_pointer;
	//for example, heir, if we have any cast
	DestructorAllocatorDeleter<Allocator> deleter;
	bool is_object_valid = true;

	ControlBlock() = delete;

	ControlBlock(RealT* real_pointer_tmp, const DestructorAllocatorDeleter<Allocator>& deleter_tmp) :
		real_pointer(real_pointer_tmp),
		deleter(deleter_tmp)
	{}

	void change_condition_of_destroy_object(bool flag) override {
		deleter.destroy_object = flag;
	}

	void delete_object_but_not_control_block() override {
		deleter(real_pointer);
		real_pointer = nullptr;
		is_object_valid = false;
	}

	bool object_valid() override {
		return is_object_valid;
	}
	
	~ControlBlock() override {
		if (is_object_valid) {
			deleter(real_pointer);
		}
		real_pointer = nullptr;
	}
};

struct BaseRealAllocator {
	BaseRealAllocator() {}
	
	virtual void free_memory(BaseRealAllocator* this_pointer) = 0;

	virtual ~BaseRealAllocator() = default;
};

template <class Allocator>
struct RealAllocator : public BaseRealAllocator {
	Allocator allocator;
	size_t number_of_char;

	RealAllocator() = delete;

	RealAllocator(const Allocator& allocator_tmp, size_t number_of_char_tmp) {
		allocator = allocator_tmp;
		number_of_char = number_of_char_tmp;
	}

	void free_memory(BaseRealAllocator* this_pointer) override {
		using AllocatorChar = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
		AllocatorChar allocator_of_char_tmp(allocator);
		size_t number_of_char_tmp = number_of_char;

		RealAllocator* this_real_pointer = static_cast<RealAllocator*>(this_pointer);
		this_real_pointer->~RealAllocator();

		char* array_of_char_tmp = reinterpret_cast<char*>(this_real_pointer);
		std::allocator_traits<AllocatorChar>::deallocate(allocator_of_char_tmp, array_of_char_tmp, number_of_char_tmp);
	}

	~RealAllocator() override {}
};

//Simple help function
template <typename Y>
void help_swap(Y* y1, Y* y2) {
	Y* tmp = y1;
	y1 = y2;
	y2 = tmp;
}

//Class SharedPtr

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {

private:
	template <typename U>
	friend class SharedPtr;
	
	template <typename U>
	friend class WeakPtr;

	template <typename U, class... Args>
	friend SharedPtr<U> makeShared(Args&&...);

	template <typename U, class Allocator, class... Args>
	friend SharedPtr<U> allocateShared(Allocator&, Args&&...);

private:
	T* pointer_quick_work;
	Count* count;
	BaseControlBlock* base_control_block;
	BaseRealAllocator* base_real_allocator;

	template <typename RealT, class Allocator, class Deleter> 
	void construct_shared_ptr_with_ready_pointer(RealT* object, const Deleter& object_deleter, const Allocator& object_allocator) {
		//have only objects in parametres
		//construct all shared pointer
		
		size_t all_size = sizeof(RealAllocator<Allocator>) + sizeof(Count) + sizeof(ControlBlock<Deleter, RealT>);
		
		using AllocatorChar = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
		AllocatorChar allocator_of_char_tmp(object_allocator);
		char* all_array = std::allocator_traits<AllocatorChar>::allocate(allocator_of_char_tmp, all_size);
		//if throw all is OK

		RealAllocator<Allocator>* real_allocator_tmp = reinterpret_cast<RealAllocator<Allocator>*>(all_array);
		Count* count_tmp = reinterpret_cast<Count*>(all_array + sizeof(RealAllocator<Allocator>));
		ControlBlock<Deleter, RealT>* control_block_tmp = reinterpret_cast<ControlBlock<Deleter, RealT>*>(
				all_array + sizeof(RealAllocator<Allocator>) + sizeof(Count));

		count_tmp->shared_ptr_count = 1;
		count_tmp->weak_ptr_count = 0;

		new(control_block_tmp) ControlBlock<Deleter, RealT>(object, object_deleter);

		new(real_allocator_tmp) RealAllocator<Allocator>(object_allocator, all_size);

		pointer_quick_work = static_cast<T*>(control_block_tmp->real_pointer);
		count = count_tmp;
		base_control_block = static_cast<BaseControlBlock*>(control_block_tmp);
		base_real_allocator = static_cast<BaseRealAllocator*>(real_allocator_tmp);
	}
	
	template <class Allocator, class Deleter>
	void contruct_shared_ptr_without_pointer(const Deleter& object_deleter, const Allocator& object_allocator) {
		//have only objects in parametres
		//construct all shared pointer
		//only thing that must be do: construct object of T on pointer_quick_work
		//pointer_quick_work will have memory for this action

		size_t all_size = sizeof(RealAllocator<Allocator>) + sizeof(Count) + sizeof(ControlBlock<Deleter, T>) + sizeof(T);

		using AllocatorChar = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
		AllocatorChar allocator_of_char_tmp(object_allocator);
		char* all_array = std::allocator_traits<AllocatorChar>::allocate(allocator_of_char_tmp, all_size);
		//if throw all is OK

		RealAllocator<Allocator>* real_allocator_tmp = reinterpret_cast<RealAllocator<Allocator>*>(all_array);
		Count* count_tmp = reinterpret_cast<Count*>(all_array + sizeof(RealAllocator<Allocator>));
		ControlBlock<Deleter, T>* control_block_tmp = reinterpret_cast<ControlBlock<Deleter, T>*>(
				all_array + sizeof(RealAllocator<Allocator>) + sizeof(Count));
		T* object = reinterpret_cast<T*>(all_array + sizeof(RealAllocator<Allocator>) + sizeof(Count) + sizeof(ControlBlock<Deleter, T>));
		//in this function we think that object is real object of type T

		count_tmp->shared_ptr_count = 1;
		count_tmp->weak_ptr_count = 0;

		new(control_block_tmp) ControlBlock<Deleter, T>(object, object_deleter);

		new(real_allocator_tmp) RealAllocator<Allocator>(object_allocator, all_size);

		pointer_quick_work = static_cast<T*>(control_block_tmp->real_pointer);
		count = count_tmp;
		base_control_block = static_cast<BaseControlBlock*>(control_block_tmp);
		base_real_allocator = static_cast<BaseRealAllocator*>(real_allocator_tmp);
	}

	void destructor() {
		//real destructor of all object
		//called if this is last shared ptr

		base_control_block->~BaseControlBlock();

		base_real_allocator->free_memory(base_real_allocator);
		//it is destructor for this object with memory for pointer

		pointer_quick_work = nullptr;
		count = nullptr;
		base_control_block = nullptr;
		base_real_allocator = nullptr;
	}

	//four pointers and three methods of construct and destroy contains all internal logic of shared ptr
	//other functions will be work with them
	
private:
	void nullptr_constructor() {
		pointer_quick_work = nullptr;
		count = nullptr;
		base_control_block = nullptr;
		base_real_allocator = nullptr;
	}

public:
	SharedPtr() {
		nullptr_constructor();
	}

	template <typename Y>
	explicit SharedPtr(Y* ptr) {
		std::allocator<T> alloc;
		AllocatorDeleter<std::allocator<T>> del(alloc);
		construct_shared_ptr_with_ready_pointer(ptr, del, alloc);
	}

private:
	SharedPtr(int i) {
		//a small crutch - constructor that put nullptr in the fields
		if (i != 0) {
			assert(i != 0);
		}
		pointer_quick_work = nullptr;
		count = nullptr;
		base_control_block = nullptr;
		base_real_allocator = nullptr;
	}

private:
	template <typename Y>
	void copy_all_pointers_from_other_shared_ptr(const SharedPtr<Y>&& y) {
		pointer_quick_work = static_cast<T*>(y.pointer_quick_work);
		count = y.count;
		base_control_block = y.base_control_block;
		base_real_allocator = y.base_real_allocator;
	}
	
	template <typename Y>
	void make_nullptr_all_pointers_from_other_shared_ptr(SharedPtr<Y>&& y) {
		y.pointer_quick_work = nullptr;
		y.count = nullptr;
		y.base_control_block = nullptr;
		y.base_real_allocator = nullptr;
	}
	
	void destroy_this_shared_ptr() {
		if (count != nullptr) {
			assert(count->shared_ptr_count > 0);
			--count->shared_ptr_count;
			if (count->shared_ptr_count == 0 &&
					count->weak_ptr_count != 0) {
				base_control_block->delete_object_but_not_control_block();
			}
			if (count->shared_ptr_count == 0 &&
					count->weak_ptr_count == 0) {
				destructor();
			}
		}
	}
	
public:
	template <typename Y>
	SharedPtr(const SharedPtr<Y>& y) {
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->shared_ptr_count;
		}
	}
	
	SharedPtr(const SharedPtr& y) {
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->shared_ptr_count;
		}
	}

	template <typename Y>
	SharedPtr(SharedPtr<Y>&& y) {
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_shared_ptr(std::move(y));
	}
	
	SharedPtr(SharedPtr&& y) {
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_shared_ptr(std::move(y));
	}

	template <typename Y>
	SharedPtr& operator=(const SharedPtr<Y>& y) {
		destroy_this_shared_ptr();
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->shared_ptr_count;
		}
		return *this;
	}
	
	SharedPtr& operator=(const SharedPtr& y) {
		destroy_this_shared_ptr();
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->shared_ptr_count;
		}
		return *this;
	}

	template <typename Y>
	SharedPtr& operator=(SharedPtr<Y>&& y) {
		destroy_this_shared_ptr();
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_shared_ptr(std::move(y));
		return *this;
	}
	
	SharedPtr& operator=(SharedPtr&& y) {
		destroy_this_shared_ptr();
		copy_all_pointers_from_other_shared_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_shared_ptr(std::move(y));
		return *this;
	}

	~SharedPtr() {
		destroy_this_shared_ptr();
	}

public:
	size_t use_count() const {
		if (pointer_quick_work != nullptr) {
			return count->shared_ptr_count;
		}
		return 1;
	}
	
	void reset() {
		destroy_this_shared_ptr();

		nullptr_constructor();
	}
	
	template <typename Y>
	void reset(Y* ptr) {
		destroy_this_shared_ptr();

		std::allocator<T> alloc;
		AllocatorDeleter<std::allocator<T>> del(alloc);
		construct_shared_ptr_with_ready_pointer(ptr, del, alloc);
	}
	
	void swap(SharedPtr& smart_pointer) {
		help_swap<T>(pointer_quick_work, smart_pointer.pointer_quick_work);
		help_swap<Count>(count, smart_pointer.count);
		help_swap<BaseControlBlock>(base_control_block, smart_pointer.base_control_block);
		help_swap<BaseRealAllocator>(base_real_allocator, smart_pointer.base_real_allocator);
	}
	
	T* get() const {
		return pointer_quick_work;
	}
	
	T& operator*() const {
		return *pointer_quick_work;
	}
	
	T* operator->() const {
		return pointer_quick_work;
	}

public:
	template <typename Y, class Deleter>
	SharedPtr(Y* ptr, const Deleter& del) {
		std::allocator<T> alloc;
		construct_shared_ptr_with_ready_pointer(ptr, del, alloc);
	}

	template <typename Y, class Deleter, class Allocator>
	SharedPtr(Y* ptr, Deleter del, Allocator alloc) {
		construct_shared_ptr_with_ready_pointer(ptr, del, alloc);
	}

};

template <typename T, class... Args>
SharedPtr<T> makeShared(Args&&... args) {
	std::allocator<T> alloc;
	DestructorDeleter del;

	SharedPtr<T> smart_ptr(0);
	smart_ptr.contruct_shared_ptr_without_pointer(del, alloc);
	//if throw all is OK
	
	try {
		new(smart_ptr.pointer_quick_work) T(std::forward<Args>(args)...);
	} catch(...) {
		smart_ptr.base_control_block->change_condition_of_destroy_object(false);
		throw;
	}

	return smart_ptr;
}

template <typename T, class Allocator, class... Args>
SharedPtr<T> allocateShared(Allocator& alloc, Args&&... args) {
	DestructorAllocatorDeleter<Allocator> del(alloc);

	SharedPtr<T> smart_ptr(0);
	smart_ptr.contruct_shared_ptr_without_pointer(del, alloc);
	//if throw all is OK

	try {
		using AllocatorTraits = typename std::allocator_traits<Allocator>;
		AllocatorTraits::construct(alloc, smart_ptr.pointer_quick_work, std::forward<Args>(args)...);
	} catch(...) {
		smart_ptr.base_control_block->change_condition_of_destroy_object(false);
		throw;
	}

	return smart_ptr;
}

template <typename T>
class WeakPtr {

private:
	template <typename U>
	friend class WeakPtr;

private:
	T* pointer_quick_work;
	Count* count;
	BaseControlBlock* base_control_block;
	BaseRealAllocator* base_real_allocator;

public:
	WeakPtr() {
		pointer_quick_work = nullptr;
		count = nullptr;
		base_control_block = nullptr;
		base_real_allocator = nullptr;
	}

	template <typename U>
	WeakPtr(const SharedPtr<U>& smart_ptr_u) {
		SharedPtr<T> smart_ptr_t(smart_ptr_u);
		
		pointer_quick_work = smart_ptr_t.pointer_quick_work;
		count = smart_ptr_t.count;
		base_control_block = smart_ptr_t.base_control_block;
		base_real_allocator = smart_ptr_t.base_real_allocator;
		
		smart_ptr_t.pointer_quick_work = nullptr;
		smart_ptr_t.count = nullptr;
		smart_ptr_t.base_control_block = nullptr;
		smart_ptr_t.base_real_allocator = nullptr;

		if (pointer_quick_work != nullptr) {
			--count->shared_ptr_count;
			++count->weak_ptr_count;
		}
	}

private:
	void destructor() {
		//real destructor of all object
		//called if this is last of shared and weak ptr

		base_control_block->~BaseControlBlock();

		base_real_allocator->free_memory(base_real_allocator);
		//it is destructor for this object with memory for pointer

		pointer_quick_work = nullptr;
		count = nullptr;
		base_control_block = nullptr;
		base_real_allocator = nullptr;
	}

	template <typename Y>
	void copy_all_pointers_from_other_weak_ptr(const WeakPtr<Y>&& y) {
		pointer_quick_work = static_cast<T*>(y.pointer_quick_work);
		count = y.count;
		base_control_block = y.base_control_block;
		base_real_allocator = y.base_real_allocator;
	}

	template <typename Y>
	void make_nullptr_all_pointers_from_other_weak_ptr(WeakPtr<Y>&& y) {
		y.pointer_quick_work = nullptr;
		y.count = nullptr;
		y.base_control_block = nullptr;
		y.base_real_allocator = nullptr;
	}

	void destroy_this_weak_ptr() {
		if (count != nullptr) {
			assert(count->weak_ptr_count > 0);
			--count->weak_ptr_count;
			if (count->shared_ptr_count == 0 &&
					count->weak_ptr_count == 0) {
				destructor();
			}
		}
	}

public:
	template <typename Y>
	WeakPtr(const WeakPtr<Y>& y) {
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->weak_ptr_count;
		}
	}
	
	WeakPtr(const WeakPtr& y) {
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->weak_ptr_count;
		}
	}

	template <typename Y>
	WeakPtr(WeakPtr<Y>&& y) {
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_weak_ptr(std::move(y));
	}
	
	WeakPtr(WeakPtr&& y) {
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_weak_ptr(std::move(y));
	}

	template <typename Y>
	WeakPtr& operator=(const WeakPtr<Y>& y) {
		destroy_this_weak_ptr();
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->weak_ptr_count;
		}
		return *this;
	}
	
	WeakPtr& operator=(const WeakPtr& y) {
		destroy_this_weak_ptr();
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		if (pointer_quick_work != nullptr) {
			++count->weak_ptr_count;
		}
		return *this;
	}

	template <typename Y>
	WeakPtr& operator=(WeakPtr<Y>&& y) {
		destroy_this_weak_ptr();
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_weak_ptr(std::move(y));
		return *this;
	}
	
	WeakPtr& operator=(WeakPtr&& y) {
		destroy_this_weak_ptr();
		copy_all_pointers_from_other_weak_ptr(std::move(y));
		make_nullptr_all_pointers_from_other_weak_ptr(std::move(y));
		return *this;
	}

	~WeakPtr() {
		destroy_this_weak_ptr();
	}
	
	size_t use_count() const {
		if (pointer_quick_work != nullptr) {
			return count->shared_ptr_count;
		}
		return 0;
	}

	bool expired() const {
		return !base_control_block->object_valid();
	}

	SharedPtr<T> lock() const {
		SharedPtr<T> smart_ptr(0);
		smart_ptr.pointer_quick_work = pointer_quick_work;
		smart_ptr.count = count;
		smart_ptr.base_control_block = base_control_block;
		smart_ptr.base_real_allocator = base_real_allocator;

		++smart_ptr.count->shared_ptr_count;
		return smart_ptr;
	}

};
