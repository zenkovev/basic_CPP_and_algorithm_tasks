#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

template <typename Number>
struct Point {
    Number x;
    Number y;

    Point() = default;

    Point(Number x_tmp, Number y_tmp) :
        x(x_tmp), y(y_tmp)
    {}

    template <typename OtherNumber>
    Point(Point<OtherNumber> tmp) :
        x(tmp.x), y(tmp.y)
    {}

    Point(const Point& begin, const Point& end) {
        x = end.x - begin.x;
        y = end.y - begin.y;
    }

    Point& operator+=(const Point& right) {
        x += right.x;
        y += right.y;
        return *this;
    }

    Point& operator-=(const Point& right) {
        x -= right.x;
        y -= right.y;
        return *this;
    }

    Point& operator*=(Number right) {
        x *= right;
        y *= right;
        return *this;
    }

    Number LengthSquare() const {
        return x * x + y * y;
    }

    long double Length() const {
        long double len_sq = LengthSquare();
        return std::sqrt(len_sq);
    }
};

template <typename Number>
Point<Number> operator+(const Point<Number>& left, const Point<Number>& right) {
    Point<Number> answer = left;
    answer += right;
    return answer;
}

template <typename Number>
Point<Number> operator-(const Point<Number>& left, const Point<Number>& right) {
    Point<Number> answer = left;
    answer -= right;
    return answer;
}

template <typename Number>
Point<Number> operator*(const Point<Number>& left, Number right) {
    Point<Number> answer = left;
    answer *= right;
    return answer;
}

template <typename Number>
Point<Number> operator*(Number left, const Point<Number>& right) {
    Point<Number> answer = right;
    answer *= left;
    return answer;
}

template <typename Number>
Number ScalarProduct(const Point<Number>& left, const Point<Number>& right) {
    return left.x * right.x + left.y * right.y;
}

template <typename Number>
Number CrossProduct(const Point<Number>& left, const Point<Number>& right) {
    return left.x * right.y - left.y * right.x;
}

template <typename Number>
std::istream& operator>>(std::istream& is, Point<Number>& p) {
    is >> p.x >> p.y;
    return is;
}

template <typename Number>
std::ostream& operator<<(std::ostream& os, const Point<Number>& p) {
    os << p.x << ' ' << p.y;
    return os;
}

template <typename Number>
struct Line {
    Number a;
    Number b;
    Number c;

    Line() = default;

    Line(Number a_tmp, Number b_tmp, Number c_tmp) :
        a(a_tmp), b(b_tmp), c(c_tmp)
    {}

    template <typename OtherNumber>
    Line(Line<OtherNumber> tmp) :
        a(tmp.a), b(tmp.b), c(tmp.c)
    {}

    Point<Number> NormalVector() const {
        Point<Number> answer(a, b);
        return answer;
    }

    Point<Number> DirectionVector() const {
        Point<Number> answer(-b, a);
        return answer;
    }

    Number PointRelationLine(const Point<Number>& p) const {
        return a * p.x + b * p.y + c;
    }

    bool IsParallel(const Line& right) const {
        Point<Number> vector_1 = DirectionVector();
        Point<Number> vector_2 = right.DirectionVector();
        return CrossProduct(vector_1, vector_2) == 0;
    }

    Point<long double> IntersectionPoint(const Line& right) const {
        if (IsParallel(right)) {
            throw "Parallel lines do not have intersection point";
        }

        long double delta = a * right.b - b * right.a;
        long double delta_1 = b * right.c - c * right.b;
        long double delta_2 = c * right.a - a * right.c;

        return Point<long double>(delta_1 / delta, delta_2 / delta);
    }
};

template <typename Number>
class ConvexHull {
public:
    std::vector<Point<Number>> upper;
    std::vector<Point<Number>> lower;

    ConvexHull(std::vector<Point<Number>>&& points) {
        std::sort(points.begin(), points.end(), [](const Point<Number>& left, const Point<Number>& right) {
            if (left.x == right.x) {
                return left.y < right.y;
            }
            return left.x < right.x;
        });

        for (size_t i = 0; i < points.size(); ++i) {
            Point<Number>& R = points[i];

            while (true) {
                if (upper.size() < 2) {
                    break;
                }

                Point<Number>& P = upper[upper.size()-1];
                Point<Number>& Q = upper[upper.size()-2];

                if (CrossProduct(P-Q, R-P) < 0) {
                    break;
                }

                upper.pop_back();
            }
            upper.push_back(R);

            while (true) {
                if (lower.size() < 2) {
                    break;
                }

                Point<Number>& P = lower[lower.size()-1];
                Point<Number>& Q = lower[lower.size()-2];

                if (CrossProduct(P-Q, R-P) > 0) {
                    break;
                }

                lower.pop_back();
            }
            lower.push_back(R);
        }
    }

    void Print() {
        /* std::cout << "Upper part of Convex hull\n";
        for (size_t i = 0; i < upper.size(); ++i) {
            std::cout << upper[i] << '\n';
        }

        std::cout << "Lower part of Convex hull\n";
        for (size_t i = 0; i < lower.size(); ++i) {
            std::cout << lower[i] << '\n';
        } */

        std::cout << upper.size()+lower.size()-2 << '\n';
        for (size_t i = 0; i < upper.size(); ++i) {
            std::cout << upper[i] << '\n';
        }
        for (size_t i = lower.size()-2; i > 0; --i) {
            std::cout << lower[i] << '\n';
        }
    }
};

int main() {
    size_t N;
    std::cin >> N;

    std::vector<Point<long long int>> points(N);
    for (size_t i = 0; i < N; ++i) {
        std::cin >> points[i];
    }

    ConvexHull<long long int> convex_hull(std::move(points));
    convex_hull.Print();
}
