#include "json.hh" // AUTO-REMOVE

#ifndef PROTOGEN_X_Y_Z__JSON_NUMBER
#define PROTOGEN_X_Y_Z__JSON_NUMBER

namespace protogen_X_Y_Z {

template<typename T> class field
{
    static_assert(std::is_arithmetic<T>::value, "invalid arithmetic type");
    protected:
        T value_ = (T) 0;
        bool empty_ = true;
    public:
        typedef T value_type;
        field() = default;
        field( const field<T> &that ) = default;
        field( field<T> &&that )  = default;
        void swap( field<T> &that ) { std::swap(this->value_, that.value_); std::swap(this->empty_, that.empty_); }
        void swap( T &that ) { std::swap(this->value_, that); empty_ = false; }
        bool empty() const { return empty_; }
        void clear() { value_ = (T) 0; empty_ = true; }
        field<T> &operator=( const field<T> &that ) { this->empty_ = that.empty_; if (!empty_) this->value_ = that.value_; return *this; }
        field<T> &operator=( const T &that ) { this->empty_ = false; this->value_ = that; return *this; }
        bool operator==( const T &that ) const { return !this->empty_ && this->value_ == that; }
        bool operator!=( const T &that ) const { return !this->empty_ && this->value_ != that; }
        bool operator==( const field<T> &that ) const { return this->empty_ == that.empty_ && this->value_ == that.value_;  }
        bool operator!=( const field<T> &that ) const { return this->empty_ != that.empty_ || this->value_ != that.value_;  }
        operator T() const { return this->value_; }
};

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
static T string_to_number( const std::string &text )
{
    double value;
#if defined(_WIN32) || defined(_WIN64)
    static _locale_t loc = _create_locale(LC_NUMERIC, "C");
    if (loc == nullptr) return 0;
    value = _strtod_l(text.c_str(), nullptr, loc);
#else
    static locale_t loc = newlocale(LC_NUMERIC_MASK, "C", 0);
    if (loc == 0) return 0;
#ifdef __USE_GNU
    value = strtod_l(text.c_str(), nullptr, loc);
#else
    locale_t old = uselocale(loc);
    if (old == 0) return 0;
    value = strtod(text.c_str(), nullptr);
    uselocale(old);
#endif
#endif
    return static_cast<T>(value);
}

template<typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, int>::type = 0>
T string_to_number( const std::string &text )
{
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<T>( _strtoi64(text.c_str(), nullptr, 10) );
#else
    return static_cast<T>( strtol(text.c_str(), nullptr, 10) );
#endif
}

template<typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_signed<T>::value, int>::type = 0>
T string_to_number( const std::string &text )
{
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<T>( _strtoui64(text.c_str(), nullptr, 10) );
#else
    return static_cast<T>( strtoul(text.c_str(), nullptr, 10) );
#endif
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
std::string number_to_string( const T &value )
{
    char tmp[320] = {0};
#if defined(_WIN32) || defined(_WIN64)
    static _locale_t loc = _create_locale(LC_NUMERIC, "C");
    if (loc == nullptr) return "0";
    _snprintf_l(tmp, sizeof(tmp) - 1, "%f", loc, value);
#else
    static locale_t loc = newlocale(LC_NUMERIC_MASK, "C", 0);
    if (loc == 0) return "0";
    locale_t old = uselocale(loc);
    if (old == 0) return "0";
    snprintf(tmp, sizeof(tmp) - 1, "%f", value);
    uselocale(old);
#endif
    tmp[sizeof(tmp) - 1] = 0;
    return tmp;
}

template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
std::string number_to_string( const T &value )
{
    return std::to_string(value);
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
int equal_number( const T &value1, const T &value2 )
{
    return std::nextafter(value1, std::numeric_limits<T>::lowest()) <= value2
        && std::nextafter(value1, std::numeric_limits<T>::max()) >= value2;
}

template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
int equal_number( const T &value1, const T &value2 )
{
    return value1 == value2;
}

template<typename T>
struct json<field<T>, typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
    static int read( json_context &ctx, field<T> &value )
    {
        T temp;
        json<T>::clear(temp);
        int result = json<T>::read(ctx, temp);
        if (result != PGR_NIL)
            value = temp;
        return result;
    }
    static int write( json_context &ctx, const field<T> &value )
    {
        if (value.empty())
        {
            *(ctx.os) << "null";
            return PGR_OK;
        }
        T temp = (T) value;
        return json<T>::write(ctx, temp);
    }
    static bool empty( const field<T> &value ) { return value.empty(); }
    static void clear( field<T> &value ) { value.clear(); }
    static bool equal( const field<T> &a, const field<T> &b ) { return a == b; }
    static void swap( field<T> &a, field<T> &b ) { a.swap(b); }
};

template<typename T>
struct json<T, typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
    static int read( json_context &ctx, T &value )
    {
        auto &tt = ctx.tok->peek();
        if (tt.id == token_id::NIL) return PGR_NIL;
        if (tt.id != token_id::NUMBER)
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "invalid numeric value");
        value = string_to_number<T>(tt.value);
        ctx.tok->next();
        return PGR_OK;
    }
    static int write( json_context &ctx, const T &value )
    {
        (*ctx.os) << number_to_string(value);
        return PGR_OK;
    }
    static bool empty( const T &value ) { return equal_number(value, (T) 0); }
    static void clear( T &value ) { value = (T) 0; }
    static bool equal( const T &a, const T &b ) { return equal_number(a, b); }
    static void swap( T &a, T &b ) { std::swap(a, b); }
};

template<>
struct json<bool, void>
{
    static int read( json_context &ctx, bool &value )
    {
        auto &tt = ctx.tok->peek();
        if (tt.id == token_id::NIL) return PGR_NIL;
        if (tt.id != token_id::BTRUE && tt.id != token_id::BFALSE)
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "invalid boolean value");
        value = tt.id == token_id::BTRUE;
        ctx.tok->next();
        return PGR_OK;
    }
    static int write( json_context &ctx, const bool &value )
    {
        (*ctx.os) <<  (value ? "true" : "false");
        return PGR_OK;
    }
    static bool empty( const bool &value ) { return !value; }
    static void clear( bool &value ) { value = false; }
    static bool equal( const bool &a, const bool &b ) { return a == b; }
    static void swap( bool &a, bool &b ) { std::swap(a, b); }
};

} // namespace protogen_X_Y_Z

#endif // PROTOGEN_X_Y_Z__JSON_NUMBER
