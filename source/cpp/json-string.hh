/*
 * Copyright 2023-2024 Bruno Ribeiro <https://github.com/brunexgeek>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "json.hh" // AUTO-REMOVE

#ifndef PROTOGEN_X_Y_Z__JSON_STRING
#define PROTOGEN_X_Y_Z__JSON_STRING

namespace protogen_X_Y_Z {

class string_field
{
    protected:
        std::string value_;
        bool null_ = true;
    public:
        typedef std::string value_type;
        string_field() = default;
        string_field( const string_field &that ) = default;
        string_field( string_field &&that )  = default;
        string_field( const value_type &that ) : value_(that) { null_ = false; }
        string_field( const char *that ) : value_(that) { null_ = false; }
        void swap( string_field &that ) { std::swap(this->value_, that.value_); std::swap(this->null_, that.null_); }
        void swap( value_type &that ) { std::swap(this->value_, that); null_ = false; }
        bool empty() const { return null_ && value_.empty(); }
        void empty(bool state) { null_ = state; if (state) value_.clear(); }
        void clear() { value_.clear(); null_ = true; }
        string_field &operator=( const string_field &that ) { this->null_ = that.null_; if (!null_) this->value_ = that.value_; return *this; }
        string_field &operator=( const value_type &that ) { this->null_ = false; this->value_ = that; return *this; }
        string_field &operator=( const char *that ) { this->null_ = false; this->value_ = that; return *this; }
        bool operator==( const char *that ) const { return !this->null_ && this->value_ == that; }
        bool operator!=( const char *that ) const { return !this->null_ && this->value_ != that; }
        bool operator==( const value_type &that ) const { return !this->null_ && this->value_ == that; }
        bool operator!=( const value_type &that ) const { return !this->null_ && this->value_ != that; }
        bool operator==( const string_field &that ) const { return this->null_ == that.null_ && this->value_ == that.value_;  }
        bool operator!=( const string_field &that ) const { return this->null_ != that.null_ || this->value_ != that.value_;  }
        operator value_type&() { return this->value_; }
        operator const value_type&() const { return this->value_; }
        value_type &operator *() { return this->value_; }
        const value_type &operator *() const { return this->value_; }
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
            return ctx.tok->error(error_code::PGERR_INVALID_VALUE, "invalid string value");
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
                    return set_error(ctx.params.error, error_code::PGERR_INVALID_VALUE, "invalid UTF-8 code point");

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
                    return set_error(ctx.params.error, error_code::PGERR_INVALID_VALUE, "invalid UTF-8 code point");

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
                    return set_error(ctx.params.error, error_code::PGERR_INVALID_VALUE, "invalid UTF-8 code point");

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

                return set_error(ctx.params.error, error_code::PGERR_INVALID_VALUE, "invalid UTF-8 code point");
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

template <>
struct json<string_field, void>
{
    static int read( json_context &ctx, string_field &value )
    {
        std::string temp;
        temp.clear();
        int result = json<std::string, void>::read(ctx, temp);
        value.empty(result == PGR_NIL);
        value = temp;
        return result;
    }
    static int write( json_context &ctx, const string_field &value )
    {
        if (value.empty())
        {
            *(ctx.os) << "null";
            return PGR_OK;
        }
        return json<std::string, void>::write(ctx, value);
    }
    static bool empty( const string_field &value ) { return value.empty(); }
    static void clear( string_field &value ) { value.clear(); }
    static bool equal( const string_field &a, const string_field &b ) { return a == b; }
    static void swap( string_field &a, string_field &b ) { a.swap(b); }
};

} // namespace protogen_X_Y_Z

#endif // PROTOGEN_X_Y_Z__JSON_STRING
