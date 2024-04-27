#ifndef PROTOGEN_JSON_X_Y_Z
#define PROTOGEN_JSON_X_Y_Z

#include "protogen.hh" // AUTO-REMOVE

namespace protogen_X_Y_Z {

using namespace protogen_X_Y_Z::internal;

struct json_context
{
    tokenizer *tok = nullptr;
    ostream *os = nullptr;
    uint32_t mask = 0;
    Parameters params;
    ErrorInfo *err = nullptr;
};

static int set_error( ErrorInfo *err, error_code code, const std::string &msg )
{
    if (err == nullptr || err->code != error_code::PGERR_OK)
        return PGR_ERROR;
    err->code = code;
    err->message = msg;
    err->line = err->column = 0;
    return PGR_ERROR;
}

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
    static int write( json_context &ctx, const field<T> &value )
    {
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
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "Invalid numeric value");
        value = string_to_number<T>(tt.value);
        ctx.tok->next();
        return PGR_OK;
    }
    static int write( json_context &ctx, const T &value )
    {
        (*ctx.os) << number_to_string(value);
        return PGR_OK;
    }
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
    static int write( json_context &ctx, const T &value )
    {
        (*ctx.os) <<  '[';
        size_t i = 0, t = value.size();
        for (auto it = value.begin(); it != value.end(); ++it, ++i)
        {
            json<typename T::value_type>::write(ctx, *it);
            if (i + 1 < t) (*ctx.os) <<  ',';
        }
        (*ctx.os) <<  ']';
        return PGR_OK;
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
    static int write( json_context &ctx, const std::vector<uint8_t> &value )
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
        return PGR_OK;
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
    static int write( json_context &ctx, const bool &value )
    {
        (*ctx.os) <<  (value ? "true" : "false");
        return PGR_OK;
    }
    static bool empty( const bool &value ) { (void) value; return false; }
    static void clear( bool &value ) { value = false; }
    static bool equal( const bool &a, const bool &b ) { return a == b; }
    static void swap( bool &a, bool &b ) { std::swap(a, b); }
};

static void write_escaped_utf8(internal::ostream *out, uint32_t codepoint)
{
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "\\u%04x", codepoint);
    (*out) << buffer;
}

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
    static int write( json_context &ctx, const std::string &value )
    {
        (*ctx.os) <<  '"';
        size_t size = value.size();
        for (size_t i = 0; i < size;)
        {
            uint8_t byte1 = value[i];
            // 1-byte character
            if (byte1 <= 0x7F)
            {
                switch (byte1)
                {
                    case '"':  (*ctx.os) <<  "\\\""; break;
                    case '\\': (*ctx.os) <<  "\\\\"; break;
                    case '/':  (*ctx.os) <<  "\\/"; break;
                    case '\b': (*ctx.os) <<  "\\b"; break;
                    case '\f': (*ctx.os) <<  "\\f"; break;
                    case '\r': (*ctx.os) <<  "\\r"; break;
                    case '\n': (*ctx.os) <<  "\\n"; break;
                    case '\t': (*ctx.os) <<  "\\t"; break;
                    default:   (*ctx.os) << (char) byte1;
                }
                i++;
            }
            else
            {
                // check whether we do not need to escape
                if (!ctx.params.ensure_ascii)
                {
                    (*ctx.os) << (char) byte1;
                    i++;
                    continue;
                }

                // 2-byte character

                if (i + 1 >= size)
                    return set_error(ctx.err, error_code::PGERR_INVALID_VALUE, "Invalid UTF-8 code point");

                uint8_t byte2 = value[i + 1];
                if (byte1 >= 0xC0 && byte1 <= 0xDF && (byte2 & 0xC0) == 0x80)
                {
                    uint32_t codepoint = ((byte1 & 0x1F) << 6) | (byte2 & 0x3F);
                    write_escaped_utf8(ctx.os, codepoint);
                    i += 2;
                    continue;
                }

                // 3-byte character

                if (i + 2 >= size)
                    return set_error(ctx.err, error_code::PGERR_INVALID_VALUE, "Invalid UTF-8 code point");

                uint8_t byte3 = value[i + 2];
                if (byte1 >= 0xE0 && byte1 <= 0xEF && i + 2 < size && (byte2 & 0xC0) == 0x80 && (byte3 & 0xC0) == 0x80)
                {
                    uint32_t codepoint = ((byte1 & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
                    write_escaped_utf8(ctx.os, codepoint);
                    i += 3;
                    continue;
                }

                // 4-byte character

                if (i + 3 >= size)
                    return set_error(ctx.err, error_code::PGERR_INVALID_VALUE, "Invalid UTF-8 code point");

                uint8_t byte4 = value[i + 3];
                if (byte1 >= 0xF0 && byte1 <= 0xF4 && i + 3 < size && (byte2 & 0xC0) == 0x80 && (byte3 & 0xC0) == 0x80 && (byte4 & 0xC0) == 0x80)
                {
                    uint32_t codepoint = ((byte1 & 0x07) << 18) | ((byte2 & 0x3F) << 12) | ((byte3 & 0x3F) << 6) | (byte4 & 0x3F);

                    // break the codepoint into UTF-16 surrogate pair
                    static const uint32_t LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
                    uint32_t lead = LEAD_OFFSET + (codepoint >> 10);
                    uint32_t trail = 0xDC00 + (codepoint & 0x3FF);
                    // write the surrogate pair
                    write_escaped_utf8(ctx.os, lead);
                    write_escaped_utf8(ctx.os, trail);
                    i += 4;
                    continue;
                }

                return set_error(ctx.err, error_code::PGERR_INVALID_VALUE, "Invalid UTF-8 code point");
            }
        }

        (*ctx.os) <<  '"';
        return PGR_OK;
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
    if (ctx.params.required && J::is_missing(ctx))
        return error_code::PGERR_MISSING_FIELD;
    return PGR_OK;
}

#define PG_DIF_EX(field_id, field_name, field_label) \
    if (name == field_label) { \
        int result = protogen_X_Y_Z::json<decltype(value.field_name)>::read(ctx, value.field_name); \
        if (result == PGR_OK) ctx.mask |= (1 << field_id); \
        return result; \
    } else

#define PG_DIF(field_name,user_data,field_id) \
    PG_DIF_EX(field_id, field_name, PG_MKSTR(field_name) )

#define PG_SIF_EX(field_name, field_label) \
    if (!protogen_X_Y_Z::json<decltype(value.field_name)>::empty(value.field_name)) \
    { \
        if (!first) (*ctx.os) <<  ','; \
        first = false; \
        (*ctx.os) <<  '\"' << field_label << "\":"; \
        protogen_X_Y_Z::json<decltype(value.field_name)>::write(ctx, value.field_name); \
    }

#define PG_SIF(field_name,user_data,field_id) \
    PG_SIF_EX(field_name, PG_MKSTR(field_name) )

#define PG_EIF(field_name,user_data,field_id) \
    if (!protogen_X_Y_Z::json<decltype(value.field_name)>::empty(value.field_name)) return false;

#define PG_CLL(field_name,user_data,field_id) \
    protogen_X_Y_Z::json<decltype(value.field_name)>::clear(value.field_name);

#define PG_QIF(field_name,user_data,field_id) \
    if (!protogen_X_Y_Z::json<decltype(a.field_name)>::equal(a.field_name, b.field_name)) return false;

#define PG_SLL(field_name,user_data,field_id) \
    protogen_X_Y_Z::json<decltype(a.field_name)>::swap(a.field_name, b.field_name);

#define PG_MIF(field_name,user_data,field_id) \
    if (!(ctx.mask & (1 << field_id))) { name = PG_MKSTR(field_name); } else

template<typename T>
bool deserialize( T &value, protogen_X_Y_Z::tokenizer& tok, const Parameters &params, ErrorInfo *err = nullptr )
{
    json_context ctx;
    ctx.tok = &tok;
    ctx.params = params;
    tok.set_error(err);
    if (json<T>::read(ctx, value) != PGR_ERROR)
        return true;
    return false;
}

template<typename T>
bool deserialize( T &value, istream &in, const Parameters &params, ErrorInfo *err = nullptr )
{
    tokenizer tok(in);
    return deserialize<T>(value, tok, params, err);
}

template<typename T>
bool deserialize( T &value, const std::string &in, const Parameters &params, ErrorInfo *err = nullptr )
{
    iterator_istream<std::string::const_iterator> is(in.begin(), in.end());
    return deserialize<T>(value, is, params, err);
}

template<typename T>
bool deserialize( T &value, const std::vector<char> &in, const Parameters &params, ErrorInfo *err = nullptr )
{
    iterator_istream<std::vector<char>::const_iterator> is(in.begin(), in.end());
    return deserialize<T>(value, is, params, err);
}

template<typename T>
bool deserialize( T &value, std::istream &in, const Parameters &params, ErrorInfo *err = nullptr )
{
    bool skip = in.flags() & std::ios_base::skipws;
    std::noskipws(in);
    std::istream_iterator<char> end;
    std::istream_iterator<char> begin(in);
    iterator_istream<std::istream_iterator<char>> is(begin, end);
    bool result = deserialize<T>(value, is, params, err);
    if (skip) std::skipws(in);
    return result;
}

template<typename T>
bool deserialize( T &value, const char *in, size_t len, const Parameters &params, ErrorInfo *err = nullptr )
{
    auto begin = mem_iterator<char>(in, len);
    auto end = mem_iterator<char>(in + len, 0);
    iterator_istream<mem_iterator<char>> is(begin, end);
    return deserialize<T>(value, is, params, err);
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

template<typename T, typename J = protogen_X_Y_Z::json<T>>
void clear( T &value ) { json<T>::clear(value); }

template<typename T, typename J = protogen_X_Y_Z::json<T>>
bool empty( const T &value ) { return json<T>::empty(value); }

#define PG_ENTITY(N,O,S) \
    struct N : public O, public protogen_X_Y_Z::message< O, S > \
    { \
        typedef O value_type; \
        typedef S serializer_type; \
        typedef protogen_X_Y_Z::ErrorInfo ErrorInfo; \
        N() = default; \
        N( const N& ) = default; \
        N( N &&that ) = default; \
        N &operator=( const N & ) = default; \
        using protogen_X_Y_Z::message<O, S>::serialize; \
        using protogen_X_Y_Z::message<O, S>::deserialize; \
        bool deserialize( protogen_X_Y_Z::tokenizer& tok, const protogen_X_Y_Z::Parameters &params, \
            protogen_X_Y_Z::ErrorInfo *err = nullptr ) override \
        { \
            protogen_X_Y_Z::json_context ctx; \
            ctx.tok = &tok; \
            ctx.params = params; \
            ctx.err = err; \
            int result = S::read(ctx, *this); \
            if (result == protogen_X_Y_Z::PGR_OK) return true; \
            return false; \
        } \
        bool serialize( protogen_X_Y_Z::ostream &out, const protogen_X_Y_Z::Parameters &params, \
            protogen_X_Y_Z::ErrorInfo *err = nullptr ) const override \
        { \
            protogen_X_Y_Z::json_context ctx; \
            ctx.os = &out; \
            ctx.params = params; \
            ctx.err = err; \
            int result = S::write(ctx, *this); \
            if (result == protogen_X_Y_Z::PGR_OK) return true; \
            return false; \
        } \
        void clear() override { S::clear(*this); } \
        bool empty() const override { return S::empty(*this); } \
        bool equal( const O &that ) const override { return S::equal(*this, that); } \
        void swap( O &that ) { S::swap(*this, that); } \
    };

#define PG_ENTITY_SERIALIZER(N,O,S) \
    namespace protogen_X_Y_Z { \
    template<> \
    struct json<N> \
    { \
        static int read( json_context &ctx, O &value ) { return S::read(ctx, value); } \
        static int read_field( json_context &ctx, const std::string &name, O &value ) { return S::read_field(ctx, name, value); } \
        static int write( json_context &ctx, const O &value ) { return S::write(ctx, value); } \
        static bool empty( const O &value ) { return S::empty(value); } \
        static void clear( O &value ) { S::clear(value); } \
        static bool equal( const O &a, const O &b ) { return S::equal(a, b); } \
        static void swap( O &a, O &b ) { S::swap(a, b); } \
        static bool is_missing( json_context &ctx ) { return S::is_missing(ctx); } \
    };}

} // namespace protogen_X_Y_Z

#endif // PROTOGEN_JSON_X_Y_Z
