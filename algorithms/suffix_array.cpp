#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace constants {
static const size_t kSigma = 26;
}

class SuffixArray {
 private:
  std::vector<size_t> string_;
  size_t k_s_;

  std::vector<size_t> equiv_classes_;
  // vector of equivalence classes

  std::vector<size_t> order_upd_;
  std::vector<size_t> equiv_classes_upd_;
  // tmp vectors for update

  size_t number_of_equiv_classes_;
  // number of equivalence classes

 public:
  std::vector<size_t> order;
  // vector of sorted substrings

  SuffixArray(const std::vector<size_t>& string_tmp)
      : string_(string_tmp),
        k_s_(string_.size()),
        order(k_s_),
        equiv_classes_(k_s_),
        order_upd_(k_s_),
        equiv_classes_upd_(k_s_) {}

  void FirstCountingSort() {
    std::vector<size_t> counter(std::max(k_s_, constants::kSigma + 1));

    for (size_t i = 0; i < k_s_; ++i) {
      counter[i] = 0;
    }
    for (size_t i = 0; i < k_s_; ++i) {
      ++counter[string_[i]];
    }
    for (size_t i = 1; i < constants::kSigma + 1; ++i) {
      counter[i] += counter[i - 1];
    }
    for (size_t i = 0; i < k_s_; ++i) {
      order[--counter[string_[i]]] = i;
    }
    equiv_classes_[order[0]] = 0;
    number_of_equiv_classes_ = 1;
    for (size_t i = 1; i < k_s_; ++i) {
      if (string_[order[i]] != string_[order[i - 1]]) {
        ++number_of_equiv_classes_;
      }
      equiv_classes_[order[i]] = number_of_equiv_classes_ - 1;
    }
  }

  void IterationCountingSort() {
    std::vector<size_t> counter(std::max(k_s_, constants::kSigma + 1));

    for (size_t k = 0; 1ULL << k < k_s_; ++k) {
      // k is size of part of pair
      for (size_t i = 0; i < k_s_; ++i) {
        order_upd_[i] = k_s_ + order[i] - (1ULL << k);
        if (order_upd_[i] >= k_s_) {
          order_upd_[i] -= k_s_;
        }
      }

      for (size_t i = 0; i < k_s_; ++i) {
        counter[i] = 0;
      }
      for (size_t i = 0; i < k_s_; ++i) {
        ++counter[equiv_classes_[order_upd_[i]]];
      }
      for (size_t i = 1; i < k_s_; ++i) {
        counter[i] += counter[i - 1];
      }

      for (size_t i = k_s_; i > 0; --i) {
        size_t j = i - 1;
        order[--counter[equiv_classes_[order_upd_[j]]]] = order_upd_[j];
      }

      equiv_classes_upd_[order[0]] = 0;
      number_of_equiv_classes_ = 1;
      for (size_t i = 1; i < k_s_; ++i) {
        size_t current_shift = (order[i] + (1ULL << k)) % k_s_;
        size_t previous_shift = (order[i - 1] + (1ULL << k)) % k_s_;
        if (equiv_classes_[order[i]] != equiv_classes_[order[i - 1]] ||
            equiv_classes_[current_shift] != equiv_classes_[previous_shift]) {
          ++number_of_equiv_classes_;
        }
        equiv_classes_upd_[order[i]] = number_of_equiv_classes_ - 1;
      }

      for (size_t i = 0; i < k_s_; ++i) {
        equiv_classes_[i] = equiv_classes_upd_[i];
      }
    }
  }
};

int main() {
  std::string string_char;
  std::cin >> string_char;

  std::vector<size_t> string;
  for (size_t i = 0; i < string_char.size(); ++i) {
    string.push_back(string_char[i] - 'a' + 1);
  }
  string.push_back(0);

  SuffixArray suffix_array(string);
  suffix_array.FirstCountingSort();
  suffix_array.IterationCountingSort();

  for (size_t i = 1; i < string.size(); ++i) {
    std::cout << suffix_array.order[i] + 1 << ' ';
  }
  std::cout << '\n';
}
