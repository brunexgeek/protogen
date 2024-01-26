#ifndef PROTOGEN_JSON_3_0_0
#define PROTOGEN_JSON_3_0_0

#include "protogen.hh" // AUTO-REMOVE

namespace protogen_3_0_0 {

using namespace protogen_3_0_0::internal;

struct json_context
{
    tokenizer *tok = nullptr;
    ostream *os = nullptr;
    uint32_t mask = 0;
    bool required = false;
};

template<typename T>
struct json<field<T>, typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
    static int read( json_context &ctx, field<T> &value )
    {
        T temp;
        json<T>::clear(temp);
        int result = json<T>::read(ctx, temp);
        value = temp;
        return result;
    }
    static void write( json_context &ctx, const field<T> &value )
    {
        T temp = (T) value;
        json<T>::write(ctx, temp);
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
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "Invalid numeric value");
        value = string_to_number<T>(tt.value);
        ctx.tok->next();
        return PGR_OK;
    }
    static void write( json_context &ctx, const T &value ) { (*ctx.os) << number_to_string(value); }
    static bool empty( const T &value ) { (void) value; return false; }
    static void clear( T &value ) { value = (T) 0; }
    static bool equal( const T &a, const T &b ) { return a == b; } // TODO: create float comparison using epsilon
    static void swap( T &a, T &b ) { std::swap(a, b); }
};

template<typename T>
struct json<T, typename std::enable_if<is_container<T>::value>::type >
{
    static int read( json_context &ctx, T &value )
    {
        if (ctx.tok->peek().id == token_id::NIL) return PGR_NIL;
        if (!ctx.tok->expect(token_id::ARRS))
            return ctx.tok->error(error_code::PGERR_INVALID_OBJECT, "Invalid object");
        while (true)
        {
            typename T::value_type temp;
            int result = json<typename T::value_type>::read(ctx, temp);
            if (result == PGR_ERROR) return result;
            if (result == PGR_OK) value.push_back(temp);
            if (!ctx.tok->expect(token_id::COMMA)) break;
        }
        if (!ctx.tok->expect(token_id::ARRE))
            return ctx.tok->error(error_code::PGERR_INVALID_OBJECT, "Invalid object");
        return PGR_OK;
    }
    static void write( json_context &ctx, const T &value )
    {
        (*ctx.os) <<  '[';
        size_t i = 0, t = value.size();
        for (auto it = value.begin(); it != value.end(); ++it, ++i)
        {
            json<typename T::value_type>::write(ctx, *it);
            if (i + 1 < t) (*ctx.os) <<  ',';
        }
        (*ctx.os) <<  ']';
    }
    static bool empty( const T &value ) { return value.empty(); }
    static void clear( T &value ) { value.clear(); }
    static bool equal( const T &a, const T &b ) { return a == b; }
    static void swap( T &a, T &b ) { std::swap(a, b); }
};


// Base64 encoder/decoder based on Joe DF's implementation
// Original source at <https://github.com/joedf/base64.c> (MIT licensed)
template <>
struct json< std::vector<uint8_t> >
{
    static int b64_int( int ch )
    {
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        if (ch == '=') return 64;
        if (ch >= '0' && ch <= '9') return ch + 4;
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return (ch - 'a') + 26;
        return 0;
    }
    static void write( json_context &ctx, const std::vector<uint8_t> &value )
    {
        static const char *B64_SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        char o[5] = { 0 };
        size_t i = 0;
        size_t size = value.size();

        (*ctx.os) <<  '"';

        for (i = 0; i + 2 < size; i += 3)
        {
            o[0] = B64_SYMBOLS[ (value[i] & 0xFF) >> 2 ];
            o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) | ((value[i + 1] & 0xF0) >> 4) ];
            o[2] = B64_SYMBOLS[ ((value[i+1] & 0x0F) << 2) | ((value[i+2] & 0xC0) >> 6) ];
            o[3] = B64_SYMBOLS[ value[i+2] & 0x3F ];
            (*ctx.os) <<  o;
        }

        if (size - i)
        {
            o[0] = B64_SYMBOLS[ (value[i] & 0xFF) >> 2 ];
            o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) ];
            o[2] = '=';
            o[3] = '=';

            if (size - i == 2)
            {
                o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) | ((value[i + 1] & 0xF0) >> 4) ];
                o[2] = B64_SYMBOLS[ ((value[i+1] & 0x0F) << 2) ];
            }

            (*ctx.os) <<  o;
        }
        (*ctx.os) <<  '"';
    }
    static int read( json_context &ctx, std::vector<uint8_t> &value )
    {
        if (ctx.tok->peek().id == token_id::NIL) return PGR_NIL;
        if (ctx.tok->peek().id != token_id::STRING)
            return ctx.tok->error(error_code::PGERR_INVALID_OBJECT, "Invalid string");

        size_t k = 0;
        int s[4];
        std::string text = ctx.tok->peek().value;
        ctx.tok->next();
        const char *ptr = text.c_str();

        while (true)
        {
            // read 4 characters
            for (size_t j = 0; j < 4; ++j)
            {
                int ch = *ptr++;
                if (ch == 0)
                {
                    if (j != 0) ctx.tok->error(error_code::PGERR_INVALID_OBJECT, "Invalid base64 data");
                    return PGR_OK;
                }
                s[j] = b64_int(ch);
            }
            // decode base64 tuple
            value.push_back( (uint8_t) (((s[0] & 0xFF) << 2 ) | ((s[1] & 0x30) >> 4)) );
            if (s[2] != 64)
            {
                value.push_back( (uint8_t) (((s[1] & 0x0F) << 4) | ((s[2] & 0x3C) >> 2)) );
                if ((s[3]!=64))
                {
                    value.push_back( (uint8_t) (((s[2] & 0x03) << 6) | s[3]) );
                    k+=3;
                }
                else
                    k+=2;
            }
            else
                k+=1;
        }
    }
    static bool empty( const std::vector<uint8_t> &value ) { return value.empty(); }
    static void clear( std::vector<uint8_t> &value ) { value.clear(); }
    static bool equal( const std::vector<uint8_t> &a, const std::vector<uint8_t> &b ) { return a == b; }
    static void swap( std::vector<uint8_t> &a, std::vector<uint8_t> &b ) { std::swap(a, b); }
};

template<>
struct json<bool, void>
{
    static int read( json_context &ctx, bool &value )
    {
        auto &tt = ctx.tok->peek();
        if (tt.id == token_id::NIL) return PGR_NIL;
        if (tt.id != token_id::BTRUE && tt.id != token_id::BFALSE)
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "Invalid boolean value");
        value = tt.id == token_id::BTRUE;
        ctx.tok->next();
        return PGR_OK;
    }
    static void write( json_context &ctx, const bool &value )
    {
        (*ctx.os) <<  (value ? "true" : "false");
    }
    static bool empty( const bool &value ) { (void) value; return false; }
    static void clear( bool &value ) { value = false; }
    static bool equal( const bool &a, const bool &b ) { return a == b; }
    static void swap( bool &a, bool &b ) { std::swap(a, b); }
};

template<>
struct json<std::string, void>
{
    static int read( json_context &ctx, std::string &value )
    {
        auto tt = ctx.tok->peek();
        if (tt.id == token_id::NIL) return PGR_NIL;
        if (!ctx.tok->expect(token_id::STRING))
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "Invalid string value");
        value = tt.value;
        return PGR_OK;
    }
    static void write( json_context &ctx, const std::string &value )
    {
        (*ctx.os) <<  '"';
        for (std::string::const_iterator it = value.begin(); it != value.end(); ++it)
        {
            switch (*it)
            {
                case '"':  (*ctx.os) <<  "\\\""; break;
                case '\\': (*ctx.os) <<  "\\\\"; break;
                case '/':  (*ctx.os) <<  "\\/"; break;
                case '\b': (*ctx.os) <<  "\\b"; break;
                case '\f': (*ctx.os) <<  "\\f"; break;
                case '\r': (*ctx.os) <<  "\\r"; break;
                case '\n': (*ctx.os) <<  "\\n"; break;
                case '\t': (*ctx.os) <<  "\\t"; break;
                default:   (*ctx.os) << *it;
            }
        }
        (*ctx.os) <<  '"';
    }
    static bool empty( const std::string &value ) { return value.empty(); }
    static void clear( std::string &value ) { value.clear(); }
    static bool equal( const std::string &a, const std::string &b ) { return a == b; }
    static void swap( std::string &a, std::string &b ) { a.swap(b); }
};


template<typename T, typename J = json<T> >
static int read_object( json_context &ctx, T &object )
{
    if (ctx.tok->peek().id == token_id::NIL) return PGR_NIL;
    if (!ctx.tok->expect(token_id::OBJS))
        return ctx.tok->error(error_code::PGERR_INVALID_OBJECT, "objects must start with '{'");
    if (!ctx.tok->expect(token_id::OBJE))
    {
        while (true)
        {
            std::string name = ctx.tok->peek().value;
            if (!ctx.tok->expect(token_id::STRING))
                return ctx.tok->error(error_code::PGERR_INVALID_NAME, "object key must be string");
            if (!ctx.tok->expect(token_id::COLON))
                return ctx.tok->error(error_code::PGERR_INVALID_SEPARATOR, "field name and value must be separated by ':'");
            int result = J::read_field(ctx, name, object);
            if (result == PGR_ERROR) return result;
            if (result != PGR_OK)
            {
                result = ctx.tok->ignore();
                if (result == PGR_ERROR) return result;
            }
            if (ctx.tok->expect(token_id::COMMA)) continue;
            if (ctx.tok->expect(token_id::OBJE)) break;
            return ctx.tok->error(error_code::PGERR_INVALID_OBJECT, "invalid JSON object");
        };
    }
    if (ctx.required && J::is_missing(ctx))
        return error_code::PGERR_MISSING_FIELD;
    return PGR_OK;
}

#define PG_DIF_EX(field_id, field_name, field_label) \
    if (name == field_label) { \
        int result = protogen_3_0_0::json<decltype(value.field_name)>::read(ctx, value.field_name); \
        if (result == PGR_OK) ctx.mask |= (1 << field_id); \
        return result; \
    } else

#define PG_DIF(field_name,user_data,field_id) \
    PG_DIF_EX(field_id, field_name, PG_MKSTR(field_name) )

#define PG_SIF_EX(field_name, field_label) \
    if (!protogen_3_0_0::json<decltype(value.field_name)>::empty(value.field_name)) \
    { \
        if (!first) (*ctx.os) <<  ','; \
        first = false; \
        (*ctx.os) <<  '\"' << field_label << "\":"; \
        protogen_3_0_0::json<decltype(value.field_name)>::write(ctx, value.field_name); \
    }

#define PG_SIF(field_name,user_data,field_id) \
    PG_SIF_EX(field_name, PG_MKSTR(field_name) )

#define PG_EIF(field_name,user_data,field_id) \
    if (!protogen_3_0_0::json<decltype(value.field_name)>::empty(value.field_name)) return false;

#define PG_CLL(field_name,user_data,field_id) \
    protogen_3_0_0::json<decltype(value.field_name)>::clear(value.field_name);

#define PG_QIF(field_name,user_data,field_id) \
    if (!protogen_3_0_0::json<decltype(a.field_name)>::equal(a.field_name, b.field_name)) return false;

#define PG_SLL(field_name,user_data,field_id) \
    protogen_3_0_0::json<decltype(a.field_name)>::swap(a.field_name, b.field_name);

#define PG_MIF(field_name,user_data,field_id) \
    if (!(ctx.mask & (1 << field_id))) { name = PG_MKSTR(field_name); } else

template<typename T>
bool deserialize( T &value, protogen_3_0_0::tokenizer& tok, bool required = false, ErrorInfo *err = nullptr )
{
    json_context ctx;
    ctx.tok = &tok;
    ctx.required = required;
    if (json<T>::read(ctx, value) != PGR_ERROR) return true;
    if (err != nullptr) *err = tok.error();
    return false;
}

template<typename T>
bool deserialize( T &value, istream &in, bool required = false, ErrorInfo *err = nullptr )
{
    tokenizer tok(in);
    return deserialize<T>(value, tok, required, err);
}

template<typename T>
bool deserialize( T &value, const std::string &in, bool required = false, ErrorInfo *err = nullptr )
{
    iterator_istream<std::string::const_iterator> is(in.begin(), in.end());
    return deserialize<T>(value, is, required, err);
}

template<typename T>
bool deserialize( T &value, const std::vector<char> &in, bool required = false, ErrorInfo *err = nullptr )
{
    iterator_istream<std::vector<char>::const_iterator> is(in.begin(), in.end());
    return deserialize<T>(value, is, required, err);
}

template<typename T>
bool deserialize( T &value, std::istream &in, bool required = false, ErrorInfo *err = nullptr )
{
    bool skip = in.flags() & std::ios_base::skipws;
    std::noskipws(in);
    std::istream_iterator<char> end;
    std::istream_iterator<char> begin(in);
    iterator_istream<std::istream_iterator<char>> is(begin, end);
    bool result = deserialize<T>(value, is, required, err);
    if (skip) std::skipws(in);
    return result;
}

template<typename T>
bool deserialize( T &value, const char *in, size_t len, bool required = false, ErrorInfo *err = nullptr )
{
    auto begin = mem_iterator<char>(in, len);
    auto end = mem_iterator<char>(in + len, 0);
    iterator_istream<mem_iterator<char>> is(begin, end);
    return deserialize<T>(value, is, required, err);
}

template<typename T>
void serialize( const T &value, ostream &out )
{
    json_context ctx;
    ctx.os = &out;
    json<T>::write(ctx, value);
}

template<typename T>
void serialize( const T &value, std::string &out )
{
    typedef std::back_insert_iterator<std::string> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    serialize<T>(value, os);
}

template<typename T>
void serialize( const T &value, std::vector<char> &out )
{
    typedef std::back_insert_iterator<std::vector<char>> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    serialize<T>(value, os);
}

template<typename T>
void serialize( const T &value, std::ostream &out )
{
    typedef std::ostream_iterator<char> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    serialize<T>(value, os);
}

template<typename T, typename J = protogen_3_0_0::json<T>>
void clear( T &value ) { json<T>::clear(value); }

template<typename T, typename J = protogen_3_0_0::json<T>>
bool empty( const T &value ) { return json<T>::empty(value); }

#define PG_ENTITY(N,O,S) \
    struct N : public O, public protogen_3_0_0::message< O, S > \
    { \
        typedef O value_type; \
        typedef S serializer_type; \
        typedef protogen_3_0_0::ErrorInfo ErrorInfo; \
        N() = default; \
        N( const N& ) = default; \
        N( N &&that ) = default; \
        N &operator=( const N & ) = default; \
        using protogen_3_0_0::message<O, S>::serialize; \
        using protogen_3_0_0::message<O, S>::deserialize; \
        bool deserialize( protogen_3_0_0::tokenizer& tok, bool required = false, \
            protogen_3_0_0::ErrorInfo *err = nullptr ) override \
        { \
            protogen_3_0_0::json_context ctx; \
            ctx.tok = &tok; \
            ctx.required = required; \
            int result = S::read(ctx, *this); \
            if (result == protogen_3_0_0::PGR_OK) return true; \
            if (err != nullptr) *err = tok.error(); \
            return false; \
        } \
        void serialize( protogen_3_0_0::ostream &out ) const override \
        { \
            protogen_3_0_0::json_context ctx; \
            ctx.os = &out; \
            S::write(ctx, *this); \
        } \
        void clear() override { S::clear(*this); } \
        bool empty() const override { return S::empty(*this); } \
        bool equal( const O &that ) const override { return S::equal(*this, that); } \
        void swap( O &that ) { S::swap(*this, that); } \
    };

#define PG_ENTITY_SERIALIZER(N,O,S) \
    namespace protogen_3_0_0 { \
    template<> \
    struct json<N> \
    { \
        static int read( json_context &ctx, O &value ) { return S::read(ctx, value); } \
        static int read_field( json_context &ctx, const std::string &name, O &value ) { return S::read_field(ctx, name, value); } \
        static void write( json_context &ctx, const O &value ) { S::write(ctx, value); } \
        static bool empty( const O &value ) { return S::empty(value); } \
        static void clear( O &value ) { S::clear(value); } \
        static bool equal( const O &a, const O &b ) { return S::equal(a, b); } \
        static void swap( O &a, O &b ) { S::swap(a, b); } \
        static bool is_missing( json_context &ctx ) { return S::is_missing(ctx); } \
    };}

} // namespace protogen_3_0_0

#endif // PROTOGEN_JSON_3_0_0
