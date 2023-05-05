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

enum PositionRelativelyLine {left, middle, right};

template <typename Number>
PositionRelativelyLine find_position_relatively_line(
    const Point<Number>& R,
    const Point<Number>& P,
    const Point<Number>& Q
) {
    // Line (P, Q)
    // Point R

    if (P.y == Q.y) {
        throw "Line must not be horizontal";
    }

    Number left_value = (R.x - P.x)*(Q.y - P.y);
    Number right_value = (R.y - P.y)*(Q.x - P.x);
    if (Q.y < P.y) {
        left_value *= Number(-1);
        right_value *= Number(-1);
    }

    if (left_value < right_value) {
        return left;
    }
    if (left_value == right_value) {
        return middle;
    }
    return right;
}

template <typename Number>
bool point_between_two_points_H(
    const Point<Number>& R,
    const Point<Number>& P,
    const Point<Number>& Q
) {
    return R.x >= std::min(P.x, Q.x) && R.x <= std::max(P.x, Q.x);
}

template <typename Number>
bool point_between_two_points_V(
    const Point<Number>& R,
    const Point<Number>& P,
    const Point<Number>& Q
) {
    return R.y >= std::min(P.y, Q.y) && R.y <= std::max(P.y, Q.y);
}

template <typename Number>
bool point_between_two_points_V_strict(
    const Point<Number>& R,
    const Point<Number>& P,
    const Point<Number>& Q
) {
    return R.y >= std::min(P.y, Q.y) && R.y < std::max(P.y, Q.y);
}

template <typename Number>
void find_in_out_polygon(
    const std::vector<Point<Number>>& points,
    const Point<Number>& current_point
) {
    size_t left_segment_counter = 0;

    size_t begin = points.size()-1;
    for (size_t end = 0; end < points.size(); ++end) {

        if (points[begin].y == points[end].y) {
            if (current_point.y == points[begin].y) {
                if (point_between_two_points_H(current_point, points[begin], points[end])) {
                    std::cout << "BOUNDARY\n";
                    return;
                }
            }
        } else {
            PositionRelativelyLine pos = find_position_relatively_line(
                current_point, points[begin], points[end]
            );
            if (pos == middle) {
                if (point_between_two_points_V(current_point, points[begin], points[end])) {
                    std::cout << "BOUNDARY\n";
                    return;
                }
            }

            if (pos == right) {
                if (point_between_two_points_V_strict(current_point, points[begin], points[end])) {
                    ++left_segment_counter;
                }
            }
        }

        begin = end;
    }

    if (left_segment_counter % 2 == 0) {
        std::cout << "OUTSIDE\n";
    } else {
        std::cout << "INSIDE\n";
    }
}

int main() {
    size_t n, m;
    std::cin >> n >> m;

    std::vector<Point<long long int>> points(n);
    for (size_t i = 0; i < n; ++i) {
        std::cin >> points[i];
    }

    for (size_t k = 0; k < m; ++k) {
        Point<long long int> current_point;
        std::cin >> current_point;
        find_in_out_polygon(points, current_point);
    }
}
