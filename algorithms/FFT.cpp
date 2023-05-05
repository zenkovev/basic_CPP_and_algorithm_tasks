#include <cassert>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

template <typename Element>
class Field {
 public:
  bool is_positive = true;
  using ObjectT = Element;

  virtual Element PrimitiveRoot(int degree) = 0;

  virtual Element ConvertFromInt(int n) = 0;
};

template <typename FloatType>
class ComplexField : public Field<std::complex<FloatType>> {
 public:
  std::complex<FloatType> PrimitiveRoot(int degree) override {
    if (degree <= 0) {
      throw "Root of non-positive degree";
    }

    const long double kPiLong = static_cast<long double>(4272943) / 1360120;
    FloatType famous_mathematical_constant_pi = kPiLong;
    // I apologize for the mockery, but I did not start this war
    // Here the name pi is best suited, but clang-tidy doesn't miss it

    FloatType imag_degree = 2 * famous_mathematical_constant_pi / degree;
    if (!Field<std::complex<FloatType>>::is_positive) {
      imag_degree *= -1;
    }

    std::complex<FloatType> complex_degree =
        std::complex<FloatType>(0, imag_degree);
    return exp(complex_degree);
  }

  std::complex<FloatType> ConvertFromInt(int n) override {
    return std::complex<FloatType>(static_cast<FloatType>(n), 0.0);
  }
};

bool IsTwoDegree(int n) {
  if (n <= 0) {
    return false;
  }
  return (n & (n - 1)) == 0;
}

template <typename Element>
class FFTSequence {
 private:
  std::vector<Element>& sequence_;

 public:
  Element& operator[](size_t pos) { return sequence_[pos]; }

  const Element& operator[](size_t pos) const { return sequence_[pos]; }

  size_t Size() { return sequence_.size(); }

  void PushBack(const Element& value) { sequence_.push_back(value); }

  void PopBack() { sequence_.pop_back(); }

  FFTSequence() = delete;

  FFTSequence(std::vector<Element>& sequence_tmp) : sequence_(sequence_tmp) {}

 private:
  void MakeFFTOrder(size_t left, size_t right) {
    if (left == right) {
      return;
    }
    {
      std::vector<Element> subsequence;
      for (size_t i = left; i <= right; i += 2) {
        subsequence.push_back(sequence_[i]);
      }
      for (size_t i = left + 1; i <= right; i += 2) {
        subsequence.push_back(sequence_[i]);
      }
      for (size_t i = 0; i < subsequence.size(); ++i) {
        sequence_[left + i] = subsequence[i];
      }
    }
    MakeFFTOrder(left, left + (right - left + 1) / 2 - 1);
    MakeFFTOrder(left + (right - left + 1) / 2, right);
  }

 public:
  void FFT(Field<Element>& field_element, size_t size) {
    if (size <= 0) {
      throw "Size of sequence_ must be positive";
    }
    while (sequence_.size() < size) {
      sequence_.push_back(field_element.ConvertFromInt(0));
    }
    while (!IsTwoDegree(sequence_.size())) {
      sequence_.push_back(field_element.ConvertFromInt(0));
    }

    Element primitive_root_two = field_element.PrimitiveRoot(2);

    MakeFFTOrder(0, sequence_.size() - 1);

    for (size_t len = 1; len * 2 <= sequence_.size(); len *= 2) {
      for (size_t start = 0; start < sequence_.size(); start += len * 2) {
        Element primitive_root_multiplier = field_element.ConvertFromInt(1);
        Element primitive_root = field_element.PrimitiveRoot(len * 2);

        for (size_t index = 0; index < len; ++index) {
          Element one =
              sequence_[start + index] +
              primitive_root_multiplier * sequence_[start + len + index];
          Element two = sequence_[start + index] +
                        primitive_root_multiplier * primitive_root_two *
                            sequence_[start + len + index];
          sequence_[start + index] = one;
          sequence_[start + len + index] = two;

          primitive_root_multiplier *= primitive_root;
        }
      }
    }
  }
};

template <typename Element1, typename Element2>
Element2 Convert(const Element1& first) {
  return Element2(first);
}

std::complex<double> Convert(int first) {
  return std::complex<double>(static_cast<double>(first), 0.0);
}

int Convert(const std::complex<double>& first) { return round(first.real()); }

inline int Min(int left, int right) { return (left <= right) ? left : right; }

int DegreeOfTwo(int n) {
  if (n > 0) {
    throw "Can find degree of two only for positive numbers";
  }
  while ((n & (n - 1)) != 0) {
    ++n;
  }
  return n;
}

template <typename Element>
class Polynom {
 private:
  std::vector<Element> coeff_;

 public:
  Element& operator[](size_t pos) { return coeff_[pos]; }

  const Element& operator[](size_t pos) const { return coeff_[pos]; }

  size_t Size() { return coeff_.size(); }

  void PushBack(const Element& value) { coeff_.push_back(value); }

  void PopBack() { coeff_.pop_back(); }

  Polynom() = default;

  Polynom(const std::vector<Element>& coeff_tmp) : coeff_(coeff_tmp) {}

  Polynom<Element>& operator+=(const Polynom<Element>& right) {
    for (size_t i = 0; i < Min(coeff_.size(), right.coeff_.size()); ++i) {
      coeff_[i] += right.coeff_[i];
    }
    for (size_t i = coeff_.size(); i < right.coeff_.size(); ++i) {
      coeff_.push_back(right.coeff_[i]);
    }
    return *this;
  }

  Polynom<Element>& operator*=(const Element& right) {
    for (size_t i = 0; i < coeff_.size(); ++i) {
      coeff_[i] *= right;
    }
    return *this;
  }

  template <typename FFTElement>
  Polynom<Element> Mul(const Polynom<Element>& right,
                       Field<FFTElement>& field_element) const {
    if (coeff_.empty() || right.coeff_.empty()) {
      return Polynom<Element>();
    }
    size_t size = coeff_.size() + right.coeff_.size() - 1;

    std::vector<FFTElement> polynom_a;
    for (size_t i = 0; i < coeff_.size(); ++i) {
      polynom_a.push_back(Convert(coeff_[i]));
    }
    std::vector<FFTElement> polynom_b;
    for (size_t i = 0; i < right.coeff_.size(); ++i) {
      polynom_b.push_back(Convert(right.coeff_[i]));
    }

    FFTSequence<FFTElement> polynom_a_fft(polynom_a);
    FFTSequence<FFTElement> polynom_b_fft(polynom_b);

    field_element.is_positive = true;
    polynom_a_fft.FFT(field_element, size);
    polynom_b_fft.FFT(field_element, size);

    for (size_t i = 0; i < polynom_a_fft.Size(); ++i) {
      polynom_a_fft[i] = polynom_a_fft[i] * polynom_b_fft[i];
    }

    field_element.is_positive = false;
    polynom_a_fft.FFT(field_element, size);

    for (size_t i = 0; i < polynom_a_fft.Size(); ++i) {
      polynom_a_fft[i] /= field_element.ConvertFromInt(polynom_a_fft.Size());
    }

    while (polynom_a_fft.Size() > size) {
      polynom_a_fft.PopBack();
    }

    Polynom<Element> answer;
    for (size_t i = 0; i < polynom_a_fft.Size(); ++i) {
      answer.coeff_.push_back(Convert(polynom_a_fft[i]));
    }
    return answer;
  }
};

int main() {
  int polynom_a_deg;
  std::cin >> polynom_a_deg;
  std::vector<int> polynom_a(polynom_a_deg + 1);
  for (int i = 0; i <= polynom_a_deg; ++i) {
    std::cin >> polynom_a[i];
  }
  int polynom_b_deg;
  std::cin >> polynom_b_deg;
  std::vector<int> polynom_b(polynom_b_deg + 1);
  for (int i = 0; i <= polynom_b_deg; ++i) {
    std::cin >> polynom_b[i];
  }

  Polynom<int> polynom_aa(polynom_a);
  Polynom<int> polynom_bb(polynom_b);

  ComplexField<double> complex_z;
  Polynom<int> polynom_cc =
      polynom_aa.Mul<std::complex<double>>(polynom_bb, complex_z);

  std::cout << polynom_cc.Size() - 1 << ' ';
  for (size_t i = 0; i < polynom_cc.Size(); ++i) {
    std::cout << polynom_cc[i] << ' ';
  }
  std::cout << '\n';
}
