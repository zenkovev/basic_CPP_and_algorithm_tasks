#include <iostream>
#include <stdexcept>
#include <assert.h>

using typebyte = uint8_t;
static size_t defaultlen = 128;

template<typename T>
//Here T is a simple type, as int or int*
void swap(T& left, T& right) {
	T tmp = left;
	left = right;
	right = tmp;
}

template<typename T>
class Deque {

private:
	T** pointers = nullptr;
	
	size_t capacity = 0;
	size_t len = defaultlen;

	size_t start;
	size_t finish;
	size_t start_in;
	size_t finish_in;
	//start and finish shows first array and array after last
	//start_in and finish_in shows first element in first array
	//and element after last element in last array
	
	inline T* private_malloc() {
		return reinterpret_cast<T*>(new typebyte[len*sizeof(T)]);
	}

	inline void private_free(T* memory) {
		delete [] reinterpret_cast<typebyte*>(memory);
	}

	inline size_t pr_start_in(size_t i) const {
		return (i == start) ? start_in : 0;
	}

	inline size_t pr_start_inin(size_t ii, size_t i, size_t j) const {
                return (ii == i) ? j : 0;
        }

	inline size_t pr_finish_in(size_t i) const {
		return (i != finish-1) ? len : finish_in;
	}

	inline size_t pr_finish_inin(size_t ii, size_t i, size_t j) const {
		return (ii != i) ? len : j;
	}
	
	void private_destroy(size_t i, size_t j) {
		//called if constructor get throw
		for (size_t ii = start; ii <= i; ++ii) {
			for (size_t jj = pr_start_in(ii); jj < pr_finish_inin(ii, i, j); ++jj) {
				(pointers[i]+jj)->~T();
			}
		}
		for (size_t ii = 0; ii < capacity; ++ii) {
			private_free(pointers[i]);
		}
		delete [] pointers;
	}

	void private_at_correct(size_t& index, size_t& index_in, size_t number_of_element) const {
		number_of_element += start_in;
		index = start + number_of_element/len;
		index_in = number_of_element % len;
	}

	T& private_at(size_t number_of_element) const {
		size_t index, index_in;
		private_at_correct(index, index_in, number_of_element);
		return pointers[index][index_in];
	}

	void private_reallock() {
		T** pointers_tmp = pointers;
                size_t capacity_tmp = capacity;
                size_t start_tmp = start;
                size_t finish_tmp = finish;

                size_t cap_3 = finish-start;
                capacity = cap_3*3;
                pointers = new T*[capacity];
                start = cap_3;
                finish = cap_3*2;

                for (size_t i = 0; i < start_tmp; ++i) {
                        private_free(pointers_tmp[i]);
                }
                for (size_t i = finish_tmp; i < capacity_tmp; ++i) {
                        private_free(pointers_tmp[i]);
                }

                for (size_t i = 0; i < cap_3; ++i) {
                        pointers[i] = private_malloc();
                        pointers[i+cap_3] = pointers_tmp[start_tmp+i];
                        pointers[i+cap_3*2] = private_malloc();
                }

                delete [] pointers_tmp;
	}

	T* private_find_pointer(size_t index, size_t index_in) const {
		if (index < capacity) {
			return pointers[index] + index_in;
		}
		return nullptr;
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
		const Deque* main_deque = nullptr;
		size_t index;
		size_t index_in;
		
		friend void Deque<T>::insert(Deque<T>::iterator, const T&);
		friend void Deque<T>::erase(Deque<T>::iterator);

	public:
		all_iterator() {}
		
		all_iterator(const Deque& deq, size_t number_of_element) {
			main_deque = &deq;
			deq.private_at_correct(index, index_in, number_of_element);
		}

		all_iterator(const all_iterator<false, T>& it) {
			main_deque = it.main_deque;
			index = it.index;
			index_in = it.index_in;
		}

		all_iterator& operator++() {
			if (index_in != main_deque->len-1) {
				++index_in;
			} else {
				++index;
				index_in = 0;
			}
			return *this;
		}

		all_iterator& operator--() {
			if (index_in != 0) {
				--index_in;
			} else {
				--index;
				index_in = main_deque->len-1;
			}
			return *this;
		}

		all_iterator& operator+=(int right_in) {
			right_in += index_in;
			index_in = 0;
			int right = right_in / main_deque->len;
			right_in %= main_deque->len;
			if (right_in < 0) {
				--right;
				right_in += main_deque->len;
			}
			index += right;
			index_in = right_in;
			return *this;
		}
		
		all_iterator& operator-=(int right_in) {
			return (*this)+=(-right_in);
		}

		all_iterator operator+(int right_in) const {
			all_iterator centre(*this);
			return centre += right_in;
		}

		all_iterator operator-(int right_in) const {
                        all_iterator centre(*this);
                        return centre -= right_in;
                }

		bool operator==(const all_iterator& r) const {
			return (index == r.index) && (index_in == r.index_in);
		}

		bool operator!=(const all_iterator& r) const {
			return !(*this == r);
		}

		bool operator<(const all_iterator& r) const {
			if (index != r.index) {
				return index < r.index;
			}
			return index_in < r.index_in;
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
			if (*this >= r) {
				size_t answer = (index-r.index)*main_deque->len + index_in - r.index_in;
				return static_cast<int>(answer);
			}
			size_t answer = (r.index-index)*main_deque->len + r.index_in - index_in;
			return -static_cast<int>(answer);
		}

		type_of_const* operator->() const {
                        return main_deque->private_find_pointer(index, index_in);
                }

                type_of_const& operator*() const {
                        return *(main_deque->private_find_pointer(index, index_in));
                }
	
	};

	template <bool flag_of_const, typename type_of_const>
	class all_reverse_iterator {
	
	private:
		all_iterator<flag_of_const, type_of_const> field;
		using ait = all_iterator<flag_of_const, type_of_const>;

	public:
		all_reverse_iterator() {}
		
		all_reverse_iterator(const Deque& deq, size_t number_of_element) {
			ait field_tmp(deq, number_of_element+1);
			field = field_tmp;
		}

		all_reverse_iterator(const all_reverse_iterator<false, T>& it) {
			field = it.field;
		}

		all_reverse_iterator& operator++() {
			--field;
			return *this;
		}

		all_reverse_iterator& operator--() {
			++field;
			return *this;
		}

		all_reverse_iterator& operator+=(int right_in) {
			field -= right_in;
			return *this;
		}
		
		all_reverse_iterator& operator-=(int right_in) {
			field += right_in;
			return *this;
		}

		all_reverse_iterator operator+(int right_in) const {
			all_reverse_iterator centre(*this);
			return centre += right_in;
		}

		all_reverse_iterator operator-(int right_in) const {
                        all_reverse_iterator centre(*this);
                        return centre -= right_in;
                }

		bool operator==(const all_reverse_iterator& r) const {
			return field == r.field;
		}

		bool operator!=(const all_reverse_iterator& r) const {
			return field != r.field;
		}

		bool operator<(const all_reverse_iterator& r) const {
			return field > r.field;
		}

		bool operator>(const all_reverse_iterator& r) const {
                        return field < r.field;
                }

		bool operator<=(const all_reverse_iterator& r) const {
			return field >= r.field;
		}

		bool operator>=(const all_reverse_iterator& r) const {
                        return field <= r.field;
                }

		int operator-(const all_reverse_iterator& r) const {
			return field - r.field; 
		}

		type_of_const* operator->() const {
			return &(*(field-1));
                }

                type_of_const& operator*() const {
                        return *(field-1);
                }
	
	};

public:
	Deque(size_t lenlen = defaultlen) {
		capacity = 3;
		len = lenlen;
		assert(len != 0);

		pointers = new T*[capacity];
		for (size_t i = 0; i < capacity; ++i) {
                        pointers[i] = private_malloc();
                }
		
		start = 1;
                finish = 2;
                start_in = 0;
                finish_in = 0;
	}

	Deque(const Deque& deq) {
		capacity = deq.capacity;
		len = deq.len;
		
		pointers = new T*[capacity];
		for (size_t i = 0; i < capacity; ++i) {
			pointers[i] = private_malloc();
		}

		start = deq.start;
                finish = deq.finish;
                start_in = deq.start_in;
                finish_in = deq.finish_in;

		for (size_t i = start; i < finish; ++i) {
                        for (size_t j = 0; j < pr_finish_in(i); ++j) {
				try {
					pointers[i][j] = deq.pointers[i][j];
				} catch(...) {
					private_destroy(i, j);
					throw;
				}
			}
                }
	}

	Deque(int sizesize, const T& value = T(), size_t lenlen = defaultlen) {
		assert(lenlen != 0);
		len = lenlen;
		assert(sizesize > 0);
		size_t capacity_3 = sizesize / len + (sizesize % len != 0);
		capacity = capacity_3*3;
		
		pointers = new T*[capacity];
                for (size_t i = 0; i < capacity; ++i) {
                        pointers[i] = private_malloc();
                }

		start = capacity_3;
		finish = capacity_3*2;
		start_in = 0;
		finish_in = sizesize - (finish-start-1)*len;

		for (size_t i = start; i < finish; ++i) {
                        for (size_t j = 0; j < pr_finish_in(i); ++j) {
				try {
					pointers[i][j] = value;
                                } catch(...) {
                                        private_destroy(i, j);
                                        throw;
				}
                        }
                }
	}

	~Deque() {
		for (size_t i = start; i < finish; ++i) {
                        for (size_t j = pr_start_in(i); j < pr_finish_in(i); ++j) {
                                (pointers[i]+j)->~T();
                        }
                }
		for (size_t i = 0; i < capacity; ++i) {
			private_free(pointers[i]);
		}
		delete [] pointers;
	}

	Deque& operator=(const Deque& deq) {
		Deque tmpdeq(deq);
		//if throw then all is OK
		swap<T**>(pointers, tmpdeq.pointers);
		swap<size_t>(capacity, tmpdeq.capacity);
		swap<size_t>(len, tmpdeq.len);
		swap<size_t>(start, tmpdeq.start);
		swap<size_t>(finish, tmpdeq.finish);
		swap<size_t>(start_in, tmpdeq.start_in);
		swap<size_t>(finish_in, tmpdeq.finish_in);
		return *this;
	}

	size_t size() const {
		size_t sizesize = (finish-start)*len;
		sizesize -= start_in;
		sizesize -= len - finish_in;
		return sizesize;
	}

	T& operator[](size_t index) {
		return private_at(index);
	}

	const T& operator[](size_t index) const {
		return private_at(index);
	}

	T& at(size_t index) {
		if (index >= size()) {
			throw std::out_of_range("Out of borders of deque");
		}
		return private_at(index);
	}

	const T& at(size_t index) const {
		if (index >= size()) {
                        throw std::out_of_range("Out of borders of deque");
                }
                return private_at(index);
	}

	void all_information(bool contain = false) const {
		std::cout << "----------" << '\n';
		std::cout << "Deque" << '\n';
		std::cout << "Capacity" << ' ' << capacity << '\n';
		std::cout << "Len" << ' ' << len << '\n';
		std::cout << "Size" << ' ' << size() << '\n';
		std::cout << "Start" << ' ' << start << '\n';
		std::cout << "Start_in" << ' ' << start_in << '\n';
		std::cout << "Finish" << ' ' << finish << '\n';
		std::cout << "Finish_in" << ' ' << finish_in << '\n';
		if (contain) {
			std::cout << '\n';
			for (size_t i = start; i < finish; ++i) {
				for (size_t j = pr_start_in(i); j < pr_finish_in(i); ++j) {
					std::cout << '[' << i << "][" << j << ']' << ' ';
					std::cout << pointers[i][j] << '\n';
				}
			}
		}
		std::cout << "----------" << '\n';
	}

	void public_information() const {
		std::cout << "----------" << '\n';
                std::cout << "Deque" << '\n';
		std::cout << "Size" << ' ' << size() << '\n';
		if (size() != 0) {
			for (size_t i = 0; i < size()-1; ++i) {
				std::cout << (*this)[i] << ' ';
			}
			std::cout << (*this)[size()-1] << '\n';
		}
		std::cout << "----------" << '\n';
	}

	void push_back(const T& value) {
		if (finish == capacity && finish_in == len) {
			private_reallock();
		}

		size_t next;
		size_t next_in;
		if (finish_in != len) {
			next = finish-1;
			next_in = finish_in;
		} else {
			next = finish;
			next_in = 0;
		}

		try {
			pointers[next][next_in] = value;
		} catch(...) {
			throw;
		}

		finish = next+1;
		finish_in = next_in+1;
	}

	void push_front(const T& value) {
		if (start == 0 && start_in == 0) {
			private_reallock();
		}

		size_t next;
                size_t next_in;
		if (start_in == 0) {
			next = start-1;
			next_in = len-1;
		} else {
			next = start;
			next_in = start_in-1;
		}

		try {
                        pointers[next][next_in] = value;
                } catch(...) {
                        throw;
                }

		start = next;
		start_in = next_in;
		if (size() == 1) {
			finish = start + 1;
			finish_in = start_in + 1;
		}
	}

	void pop_back() {
		if (size() == 0) return;
		(pointers[finish-1]+finish_in-1)->~T();
		if (finish_in-1 == 0) {
			finish -= 1;
			finish_in = len;
		} else {
			finish_in -= 1;
		}
		return;
	}

	void pop_front() {
		if (size() == 0) return;
                (pointers[start]+start_in)->~T();
                if (start_in != len-1) {
			start_in += 1;
		} else {
			start += 1;
			start_in = 0;
			if (size() == 0) {
				finish = start + 1;
				finish_in = start_in;
			}
		}
                return;
	}

	iterator begin() {
		iterator it(*this, 0);
		return it;
	}

	const_iterator begin() const {
		const_iterator it(*this, 0);
		return it;
	}

	const_iterator cbegin() const {
		const_iterator it(*this, 0);
                return it;
	}

	iterator end() {
		iterator it(*this, size());
                return it;
	}

	const_iterator end() const {
		const_iterator it(*this, size());
                return it;
	}

	const_iterator cend() const {
		const_iterator it(*this, size());
                return it;
	}

	using reverse_iterator = all_reverse_iterator<false, T>;
        using const_reverse_iterator = all_reverse_iterator<true, const T>;

	reverse_iterator rbegin() {
                reverse_iterator it(*this, size()-1);
                return it;
        }

        const_reverse_iterator rbegin() const {
                const_reverse_iterator it(*this, size()-1);
                return it;
        }

        const_reverse_iterator crbegin() const {
                const_reverse_iterator it(*this, size()-1);
                return it;
        }

        reverse_iterator rend() {
                reverse_iterator it(*this, -1);
                return it;
        }

        const_reverse_iterator rend() const {
                const_reverse_iterator it(*this, -1);
                return it;
        }

        const_reverse_iterator crend() const {
                const_reverse_iterator it(*this, -1);
                return it;
        }

	void insert(iterator it, const T& value) {
		Deque deq;
		(&deq)->~Deque();
		deq.capacity = capacity;
		deq.len = len;

		deq.pointers = new T*[deq.capacity];
		for (size_t i = 0; i < deq.capacity; ++i) {
			deq.pointers[i] = private_malloc();
		}

		deq.start = start;
		deq.finish = finish;
		deq.start_in = start_in;
		deq.finish_in = finish_in;

		for (size_t i = deq.start; i <= it.index; ++i) {
			for (size_t j = deq.start_in; j < deq.pr_finish_inin(i, it.index, it.index_in); ++j) {
				try {
					deq.pointers[i][j] = pointers[i][j];
				} catch(...) {
					deq.private_destroy(i, j);
					throw;
				}
			}
		}
		{
			size_t i = it.index;
			size_t j = it.index_in;
			try {
				deq.pointers[i][j] = value;
			} catch(...) {
				deq.private_destroy(i, j);
				throw;
			}
		}
		size_t last_i = it.index;
		size_t last_j = it.index_in;
		for (size_t i = it.index; i < deq.finish; ++i) {
			for (size_t j = deq.pr_start_inin(i, it.index, it.index_in); j < deq.pr_finish_in(i); ++j) {
				if (i == it.index && j == it.index_in) continue;
				try {
					deq.pointers[i][j] = pointers[last_i][last_j];
					last_i = i;
					last_j = j;
				} catch(...) {
					deq.private_destroy(i, j);
					throw;
				}
			}
		}
		deq.push_back(pointers[last_i][last_j]);
		//if throw all is OK
		
		swap<T**>(pointers, deq.pointers);
		swap<size_t>(capacity, deq.capacity);
		swap<size_t>(len, deq.len);
		swap<size_t>(start, deq.start);
		swap<size_t>(finish, deq.finish);
		swap<size_t>(start_in, deq.start_in);
		swap<size_t>(finish_in, deq.finish_in);
	}

	void erase(iterator it) {
		Deque deq;
		(&deq)->~Deque();
                deq.capacity = capacity;
                deq.len = len;

                deq.pointers = new T*[deq.capacity];
                for (size_t i = 0; i < deq.capacity; ++i) {
                        deq.pointers[i] = private_malloc();
                }

                deq.start = start;
                deq.finish = finish;
                deq.start_in = start_in;
                deq.finish_in = finish_in;
                
                for (size_t i = deq.start; i <= it.index; ++i) {
			for (size_t j = deq.start_in; j < deq.pr_finish_inin(i, it.index, it.index_in); ++j) {
				try {
					deq.pointers[i][j] = pointers[i][j];
				} catch(...) {
					deq.private_destroy(i, j);
					throw;
				}
			}
		}
                size_t last_i = it.index;
                size_t last_j = it.index_in;
                for (size_t i = it.index; i < deq.finish; ++i) {
                        for (size_t j = deq.pr_start_inin(i, it.index, it.index_in); j < deq.pr_finish_in(i); ++j) {
                                if (i == it.index && j == it.index_in) continue;
                                try {
                                        deq.pointers[last_i][last_j] = pointers[i][j];
                                        last_i = i;
                                        last_j = j;
                                } catch(...) {
                                        deq.private_destroy(last_i, last_j);
                                        throw;
                                }
			}
                }

		if (deq.finish_in-1 == 0) {
			deq.finish -= 1;
			deq.finish_in = len;
		} else {
			deq.finish_in -= 1;
		}

		swap<T**>(pointers, deq.pointers);
                swap<size_t>(capacity, deq.capacity);
                swap<size_t>(len, deq.len);
                swap<size_t>(start, deq.start);
                swap<size_t>(finish, deq.finish);
                swap<size_t>(start_in, deq.start_in);
                swap<size_t>(finish_in, deq.finish_in);
	}

};
