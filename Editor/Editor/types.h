//------------------------------------------------------------------------------
// Math!
//------------------------------------------------------------------------------
# if !defined(__AX_MATH_2D_H__)
# define __AX_MATH_2D_H__


//------------------------------------------------------------------------------
# if defined(__cplusplus)


//------------------------------------------------------------------------------
# include <algorithm>
# include <type_traits>


//------------------------------------------------------------------------------
namespace ax {


//------------------------------------------------------------------------------
constexpr float AX_PI = 3.14159265358979323846f;


//------------------------------------------------------------------------------
enum class matrix_order
{
    prepend,
    append,
    set
};


//------------------------------------------------------------------------------
template <typename T>
struct basic_point
{
    typedef T value_type;

    T x, y;

    basic_point(): x(0), y(0) {}
    basic_point(T x, T y): x(x), y(y) {}

    basic_point(const basic_point&) = default;
    basic_point(basic_point&&) = default;
    basic_point& operator=(const basic_point&) = default;
    basic_point& operator=(basic_point&&) = default;

    inline basic_point cwise_min(const basic_point& rhs) const { return basic_point(std::min(x, rhs.x), std::min(y, rhs.y)); }
    inline basic_point cwise_max(const basic_point& rhs) const { return basic_point(std::max(x, rhs.x), std::max(y, rhs.y)); }

    friend inline bool operator == (const basic_point& lhs, const basic_point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
    friend inline bool operator != (const basic_point& lhs, const basic_point& rhs) { return !(lhs == rhs); }

    friend inline basic_point operator + (const basic_point& lhs, const basic_point& rhs) { return basic_point(lhs.x + rhs.x, lhs.y + rhs.y); }
    friend inline basic_point operator - (const basic_point& lhs, const basic_point& rhs) { return basic_point(lhs.x - rhs.x, lhs.y - rhs.y); }

    basic_point& operator += (const basic_point& rhs) { *this = *this + rhs; return *this; }
    basic_point& operator -= (const basic_point& rhs) { *this = *this - rhs; return *this; }
};

typedef basic_point<int>    point;
typedef basic_point<float>  pointf;


//------------------------------------------------------------------------------
template <typename T>
struct basic_size
{
    T w, h;

    basic_size(): w(0), h(0) {}
    basic_size(T w, T h): w(w), h(h) {}

    basic_size(const basic_size&) = default;
    basic_size(basic_size&&) = default;
    basic_size& operator=(const basic_size&) = default;
    basic_size& operator=(basic_size&&) = default;

    friend inline bool operator == (const basic_size& lhs, const basic_size& rhs) { return lhs.w == rhs.w && lhs.h == rhs.h; }
    friend inline bool operator != (const basic_size& lhs, const basic_size& rhs) { return !(lhs == rhs); }

    bool is_empty() const { return w <= 0 || h <= 0; }
};

typedef basic_size<int>    size;
typedef basic_size<float>  sizef;


//------------------------------------------------------------------------------
template <typename T>
struct basic_rect
{
    typedef typename basic_point<T> point_t;
    typedef typename basic_size<T>  size_t;

    union
    {
        struct { point_t location; size_t size; };
        struct { T x, y, w, h; };
    };

    basic_rect(): x(0), y(0), w(0), h(0) {}
    basic_rect(const point_t& tl, const point_t& br): location(tl), size(br.x - tl.x, br.y - tl.y) {}
    basic_rect(const point_t& l, const size_t& s): location(l), size(s) {}
    basic_rect(T x, T y, T w, T h): x(x), y(y), w(w), h(h) {}

    basic_rect(const basic_rect&) = default;
    basic_rect(basic_rect&&) = default;
    basic_rect& operator=(const basic_rect&) = default;
    basic_rect& operator=(basic_rect&&) = default;

    point_t top_left() const { return point_t(x, y); }
    point_t top_right() const { return point_t(x + w, y); }
    point_t bottom_left() const { return point_t(x, y + h); }
    point_t bottom_right() const { return point_t(x + w, y + h); }

    T left()   const { return x;     }
    T right()  const { return x + w; }
    T top()    const { return y;     }
    T bottom() const { return y + h; }

    point_t center()   const { return point_t(center_x(), center_y()); }
    T       center_x() const { return x + w / 2; }
    T       center_y() const { return y + h / 2; }

    pointf centerf()   const { return pointf(centerf_x(), centerf_y()); }
    float  centerf_x() const { return x + w / 2.0f; }
    float  centerf_y() const { return y + h / 2.0f; }

    bool is_empty() const { return size.is_empty(); }

    static inline basic_rect make_union(const basic_rect& lhs, const basic_rect& rhs)
    {
        if (lhs.is_empty())
            return rhs;
        else if (rhs.is_empty())
            return lhs;

        const auto tl = lhs.top_left().cwise_min(rhs.top_left());
        const auto br = lhs.bottom_right().cwise_max(rhs.bottom_right());

        return basic_rect(tl, br);
    }
};

typedef basic_rect<int>   rect;
typedef basic_rect<float> rectf;


//------------------------------------------------------------------------------
struct matrix
{
    float m11, m12, m21, m22, m31, m32;

    matrix(): m11(1), m12(0), m21(0), m22(1), m31(0), m32(0) {}
    matrix(float m11, float m12, float m21, float m22, float m31, float m32):
        m11(m11), m12(m12), m21(m21), m22(m22), m31(m31), m32(m32) {}

    void zero();
    void reset();

    bool invert();

    void translate(float x, float y, matrix_order order = matrix_order::prepend);
    void rotate(float angle, matrix_order order = matrix_order::prepend);
    void rotate_at(float angle, float cx, float cy, matrix_order order = matrix_order::prepend);
    void scale(float x, float y, matrix_order order = matrix_order::prepend);
    void shear(float x, float y, matrix_order order = matrix_order::prepend);

    void combine(const matrix& matrix, matrix_order order = matrix_order::prepend);

    matrix inverted() const;
};

struct matrix4
{
    float m11, m12, m13, m14;
    float m21, m22, m23, m24;
    float m31, m32, m33, m34;
    float m41, m42, m43, m44;

    matrix4():
        m11(1), m12(0), m13(0), m14(0),
        m21(0), m22(1), m23(0), m24(0),
        m31(0), m32(0), m33(1), m34(0),
        m41(0), m42(0), m43(0), m44(1)
    {
    }

    matrix4(
        float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44):
        m11(m11), m12(m12), m13(m13), m14(m14),
        m21(m21), m22(m22), m23(m23), m24(m24),
        m31(m31), m32(m32), m33(m33), m34(m34),
        m41(m41), m42(m42), m43(m43), m44(m44)
    {
    }

    matrix4(const matrix& m):
        m11(m.m11), m12(m.m12), m13(0.0f), m14(0.0f),
        m21(m.m21), m22(m.m22), m23(0.0f), m24(0.0f),
        m31(0.0f),  m32(0.0f),  m33(1.0f), m34(0.0f),
        m41(m.m31), m42(m.m32), m43(0.0f), m44(1.0f)
    {
    }

    void zero();
    void reset();

    bool invert();
    void transpose();

    void translate(float x, float y, float z, matrix_order order = matrix_order::prepend);
    void rotate_x(float angle, matrix_order order = matrix_order::prepend);
    void rotate_y(float angle, matrix_order order = matrix_order::prepend);
    void rotate_z(float angle, matrix_order order = matrix_order::prepend);
    void rotate_axis(float angle, float x, float y, float z, matrix_order order = matrix_order::prepend);
    void scale(float x, float y, float z, matrix_order order = matrix_order::prepend);

    void combine(const matrix4& matrix, matrix_order order = matrix_order::prepend);

    matrix4 inverted() const;
    matrix4 transposed() const;
};


//------------------------------------------------------------------------------
namespace detail {
template <typename M, typename T> void transform_points(const M& matrix, basic_point<T>* points, size_t count);
template <typename M, typename T> void transform_vectors(const M& matrix, basic_point<T>* points, size_t count);
} // namespace detail


//------------------------------------------------------------------------------
template <typename M, typename T>
inline basic_point<T> transformed(const basic_point<T>& point, const M& matrix)
{
    auto result = point;
    detail::transform_points(matrix, &result, 1);
    return result;
}

template <typename M, typename T>
inline void transform(basic_point<T>& point, const M& matrix)
{
    detail::transform_points(matrix, &point, 1);
}

template <typename M, typename T, size_t N>
inline void transform(basic_point<T> (&point)[N], const M& matrix)
{
    detail::transform_points(matrix, point, N);
}

template <typename M, typename T>
inline void transform(basic_point<T>* point, size_t n, const M& matrix)
{
    detail::transform_points(matrix, point, n);
}

template <typename M, typename T>
inline basic_point<T> transformed_v(const basic_point<T>& vector, const M& matrix)
{
    auto result = vector;
    detail::transform_vectors(matrix, &result, 1);
    return result;
}

template <typename M, typename T>
inline void transform_v(basic_point<T>& point, const M& matrix)
{
    detail::transform_vectors(matrix, &point, 1);
}

template <typename M, typename T, size_t N>
inline void transform_v(basic_point<T>(&point)[N], const M& matrix)
{
    detail::transform_vectors(matrix, point, N);
}

template <typename M, typename T>
inline void transform_v(basic_point<T>* point, size_t n, const M& matrix)
{
    detail::transform_vectors(matrix, point, n);
}


//------------------------------------------------------------------------------
} // namespace ax


//------------------------------------------------------------------------------
# endif // defined(__cplusplus)


//------------------------------------------------------------------------------
# endif // __AX_MATH_2D_H__