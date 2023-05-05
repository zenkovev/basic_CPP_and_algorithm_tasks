#pragma GCC optimize("O0")

#include <initializer_list>
#include <type_traits>

#include <iostream>
#include <cassert>

template <typename Head, typename... Tail>
union VariantUnion {
	Head head;
	VariantUnion<Tail...> tail;
	
	VariantUnion() {}
	~VariantUnion() {}
	
	template <typename T, size_t N = 0>
	constexpr size_t get_index_by_type() {
		//find type T in VariantUnion and return its index
		//static_assert if VariantUnion does not have T type
		if constexpr (std::is_same_v<T, Head>) {
			return N;
		} else {
			return tail.template get_index_by_type<T, N+1>();
		}
	}
	
	template <size_t N, class... Args>
	bool push(Args&&... args) {
		//construct element of type with index N
		//true if all is OK
		//false else
		if constexpr (N == 0) {
			try {
				using NonConstHead = typename std::remove_const_t<Head>;
				NonConstHead* non_const_pointer = const_cast<NonConstHead*>(&head);
				new(non_const_pointer) Head(std::forward<Args>(args)...);
			} catch(...) {
				return false;
			}
			return true;
		} else {
			return tail.template push<N-1, Args...>(args...);
		}
	}
	
	template <size_t N>
	auto& get() {
		//get element by index from union
		if constexpr (N == 0) {
			return head;
		} else {
			return tail.template get<N-1>();
		}
	}
	
	template <size_t N>
	const auto& get() const {
		//get element by index from union
		if constexpr (N == 0) {
			return head;
		} else {
			return tail.template get<N-1>();
		}
	}
	
	void destroy(size_t N) {
		//delete element by index from union
		if (N == 0) {
			head.~Head();
		} else {
			tail.destroy(N-1);
		}
	}
};

template <typename Head>
union VariantUnion<Head> {
	Head head;
	
	VariantUnion() {}
	~VariantUnion() {}
	
	template <typename T, size_t N = 0>
	constexpr size_t get_index_by_type() {
		//find type T in VariantUnion and return its index
		//static_assert if VariantUnion does not have T type
		static_assert(std::is_same_v<T, Head>);
		return N;
	}
	
	template <size_t N, class... Args>
	bool push(Args&&... args) {
		//construct element of type with index N
		//true if all is OK
		//false else
		static_assert(N == 0, "Union contains less number of elements");
		try {
			using NonConstHead = typename std::remove_const_t<Head>;
			NonConstHead* non_const_pointer = const_cast<NonConstHead*>(&head);
			new(non_const_pointer) Head(std::forward<Args>(args)...);
		} catch(...) {
			return false;
		}
		return true;
	}
	
	template <size_t N>
	auto& get() {
		//get element by index from union
		static_assert(N == 0, "Union contains less number of elements");
		return head;
	}
	
	template <size_t N>
	const auto& get() const {
		//get element by index from union
		static_assert(N == 0, "Union contains less number of elements");
		return head;
	}

	void destroy(size_t N) {
		//delete element by index from union
		assert(N == 0);
		head.~Head();
	}
};

template <typename... Types>
class Variant;

template <typename T, typename... Types>
class VariantParent {

public:
	using VariantHeir = Variant<Types...>;
	
	VariantParent() {}
	
	VariantParent(const VariantParent&) {}
	
	VariantParent(const T& value) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		ptr->used_index = index_of_type_of_variant_parent;
		ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(value);
	}

	VariantParent(const T&& value) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		ptr->used_index = index_of_type_of_variant_parent;
		ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(value);
	}
	
	template <typename MakeType>
	VariantParent(const std::initializer_list<MakeType>& list) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		ptr->used_index = index_of_type_of_variant_parent;
		ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(list);
	}
	
	template <typename... Args, typename = std::enable_if_t<
		std::conjunction_v<
			std::is_constructible<T, Args...>,
			std::disjunction<
				std::negation<std::is_constructible<Types, Args...>>,
				std::is_same<Types, T>
			>...
		>
	>>
	VariantParent(Args&&... args) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		ptr->used_index = index_of_type_of_variant_parent;
		ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(args...);
	}
	
	VariantHeir& operator=(const T& value) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		if (!ptr->non_empty) {
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(value);
			return *ptr;
		} else if (ptr->used_index == index_of_type_of_variant_parent) {
			ptr->storage.template get<index_of_type_of_variant_parent>() = value;
			//if throw all is OK
			return *ptr;
		} else {
			ptr->storage.destroy(ptr->used_index);
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(value);
			return *ptr;
		}
	}
	
	VariantHeir& operator=(const T&& value) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		if (!ptr->non_empty) {
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(value);
			return *ptr;
		} else if (ptr->used_index == index_of_type_of_variant_parent) {
			ptr->storage.template get<index_of_type_of_variant_parent>() = value;
			//if throw all is OK
			return *ptr;
		} else {
			ptr->storage.destroy(ptr->used_index);
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(value);
			return *ptr;
		}
	}
	
	template <typename MakeType>
	VariantHeir& operator=(const std::initializer_list<MakeType>& list) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		if (!ptr->non_empty) {
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(list);
			return *ptr;
		} else {
			ptr->storage.destroy(ptr->used_index);
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(list);
			return *ptr;
		}
	}
	
	template <typename... Args, typename = std::enable_if_t<
		std::conjunction_v<
			std::is_constructible<T, Args...>,
			std::disjunction<
				std::negation<std::is_constructible<Types, Args...>>,
				std::is_same<Types, T>
			>...
		>
	>>
	VariantHeir& operator=(Args&&... args) {
		VariantHeir* ptr = static_cast<VariantHeir*>(this);
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		if (!ptr->non_empty) {
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(args...);
			return *ptr;
		} else {
			ptr->storage.destroy(ptr->used_index);
			ptr->used_index = index_of_type_of_variant_parent;
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(args...);
			return *ptr;
		}
	}
	
	void copy_union(const VariantUnion<Types...>& vu, size_t ind, bool non_emp) {
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		if ((ind == index_of_type_of_variant_parent) && non_emp) {
			VariantHeir* ptr = static_cast<VariantHeir*>(this);
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(
					vu.template get<index_of_type_of_variant_parent>()
			);
		}
	}
	
	void move_union(VariantUnion<Types...>& vu, size_t ind, bool non_emp) {
		VariantUnion<Types...> union_tmp;
		static constexpr size_t index_of_type_of_variant_parent = union_tmp.template get_index_by_type<T>();
		if ((ind == index_of_type_of_variant_parent) && non_emp) {
			VariantHeir* ptr = static_cast<VariantHeir*>(this);
			ptr->non_empty = ptr->storage.template push<index_of_type_of_variant_parent>(
					vu.template get<index_of_type_of_variant_parent>()
			);
			vu.destroy(index_of_type_of_variant_parent);
			try {
				vu.template push<index_of_type_of_variant_parent>();
				vu.destroy(index_of_type_of_variant_parent);
			} catch(...) {}
		}
	}
	
};

template <typename... Types>
class Variant : private VariantParent<Types, Types...>... {

private:
	template <typename T, typename... Ts>
	friend class VariantParent;

private:
	VariantUnion<Types...> storage;
	size_t used_index;
	//index of used type
	bool non_empty;
	//flag: does this element exist

public:
	Variant() {
		used_index = 0;
		non_empty = storage.template push<0>();
	}

	using VariantParent<Types, Types...>::VariantParent...;
	using ::VariantParent<Types, Types...>::operator=...;

	Variant(const Variant& variant) : VariantParent<Types, Types...>(variant)... {
		used_index = variant.used_index;
		non_empty = variant.non_empty;
		(::VariantParent<Types, Types...>::copy_union(variant.storage, variant.used_index, variant.non_empty), ...);
	}

	Variant(Variant&& variant) {
		used_index = variant.used_index;
		non_empty = variant.non_empty;
		(::VariantParent<Types, Types...>::move_union(variant.storage, variant.used_index, variant.non_empty), ...);
	}

	Variant& operator=(const Variant& variant) {
		if (non_empty) {
			storage.destroy(used_index);
		}
		used_index = variant.used_index;
		non_empty = variant.non_empty;
		(::VariantParent<Types, Types...>::copy_union(variant.storage, variant.used_index, variant.non_empty), ...);
		return *this;
	}

	Variant& operator=(Variant&& variant) {
		if (non_empty) {
			storage.destroy(used_index);
		} 
		used_index = variant.used_index;
		non_empty = variant.non_empty;
		(::VariantParent<Types, Types...>::move_union(variant.storage, variant.used_index, variant.non_empty), ...);
		return *this;
	}
	
	~Variant() {
		if (non_empty) {
			storage.destroy(used_index);
		}
	}

	template <size_t I, typename... Args>
	auto& emplace(Args&&... args) {
		if (non_empty) {
			storage.destroy(used_index);
		}
		used_index = I;
		non_empty = storage.template push<I>(args...);
		return storage.template get<I>();
	}
	
	template <typename T, typename... Args>
	T& emplace(Args&&... args) {
		VariantUnion<Types...> union_tmp;
		static constexpr size_t I = union_tmp.template get_index_by_type<T>();
		return emplace<I>(args...);
	}
	
	template <size_t I, typename MakeType>
	auto& emplace(std::initializer_list<MakeType> list) {
		if (non_empty) {
			storage.destroy(used_index);
		}
		used_index = I;
		non_empty = storage.template push<I>(list);
		return storage.template get<I>();
	}
	
	template <typename T, typename MakeType>
	T& emplace(std::initializer_list<MakeType> list) {
		VariantUnion<Types...> union_tmp;
		static constexpr size_t I = union_tmp.template get_index_by_type<T>();
		return emplace<I>(list);
	}
	
	size_t index() const {
		return used_index;
	}
	
	bool valueless_by_exception() const {
		return !non_empty;
	}
	
private:
	template <typename T, typename... Ts>
	friend bool holds_alternative(const Variant<Ts...>&);
	
	template <typename T, typename... Ts>
	friend bool holds_alternative(const Variant<Ts...>&&);
	
	template <typename T, typename... Ts>
	friend T& get(Variant<Ts...>&);

	template <typename T, typename... Ts>
	friend const T& get(const Variant<Ts...>&);

	template <typename T, typename... Ts>
	friend T&& get(Variant<Ts...>&&);

	template <typename T, typename... Ts>
	friend const T&& get(const Variant<Ts...>&&);

	template <size_t I, typename... Ts>
	friend auto& get(Variant<Ts...>&);

	template <size_t I, typename... Ts>
	friend const auto& get(const Variant<Ts...>&);

	template <size_t I, typename... Ts>
	friend auto&& get(Variant<Ts...>&&);

	template <size_t I, typename... Ts>
	friend const auto&& get(const Variant<Ts...>&&);

};

template <typename T, typename... Types>
bool holds_alternative(const Variant<Types...>& variant) {
	VariantUnion<Types...> union_tmp;
	static constexpr size_t index = union_tmp.template get_index_by_type<T>();
	return (index == variant.used_index) ? true : false;
}

template <typename T, typename... Types>
bool holds_alternative(const Variant<Types...>&& variant) {
	VariantUnion<Types...> union_tmp;
	static constexpr size_t index = union_tmp.template get_index_by_type<T>();
	return (index == variant.used_index) ? true : false;
}

template <typename T, typename... Types>
T& get(Variant<Types...>& variant) {
	bool flag = holds_alternative<T>(variant);
	VariantUnion<Types...> union_tmp;
	static constexpr size_t index = union_tmp.template get_index_by_type<T>();
	if (!flag)
		throw "This type does not used now";
	T& answer = variant.storage.template get<index>();
	return answer;
}

template <typename T, typename... Types>
const T& get(const Variant<Types...>& variant) {
	bool flag = holds_alternative<T>(variant);
	VariantUnion<Types...> union_tmp;
	static constexpr size_t index = union_tmp.template get_index_by_type<T>();
	if (!flag)
		throw "This type does not used now";
	T& answer = variant.storage.template get<index>();
	return answer;
}

template <typename T, typename... Types>
T&& get(Variant<Types...>&& variant) {
	bool flag = holds_alternative<T>(variant);
	VariantUnion<Types...> union_tmp;
	static constexpr size_t index = union_tmp.template get_index_by_type<T>();
	if (!flag)
		throw "This type does not used now";
	T& answer = variant.storage.template get<index>();
	return std::move(answer);
}

template <typename T, typename... Types>
const T&& get(const Variant<Types...>&& variant) {
	bool flag = holds_alternative<T>(variant);
	VariantUnion<Types...> union_tmp;
	static constexpr size_t index = union_tmp.template get_index_by_type<T>();
	if (!flag)
		throw "This type does not used now";
	T& answer = variant.storage.template get<index>();
	return std::move(answer);
}

template <size_t I, typename... Types>
auto& get(Variant<Types...>& variant) {
	bool flag = (variant.used_index == I);
	if (!flag)
		throw "This type does not used now";
	auto& answer = variant.storage.template get<I>();
	return answer;
}

template <size_t I, typename... Types>
const auto& get(const Variant<Types...>& variant) {
	bool flag = (variant.used_index == I);
	if (!flag)
		throw "This type does not used now";
	auto& answer = variant.storage.template get<I>();
	return answer;
}

template <size_t I, typename... Types>
auto&& get(Variant<Types...>&& variant) {
	bool flag = (variant.used_index == I);
	if (!flag)
		throw "This type does not used now";
	auto& answer = variant.storage.template get<I>();
	return std::move(answer);
}

template <size_t I, typename... Types>
const auto&& get(const Variant<Types...>&& variant) {
	bool flag = (variant.used_index == I);
	if (!flag)
		throw "This type does not used now";
	auto& answer = variant.storage.template get<I>();
	return std::move(answer);
}
