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

#ifndef PROTOGEN_X_Y_Z__JSON
#define PROTOGEN_X_Y_Z__JSON

#include "protogen.hh" // AUTO-REMOVE
#include <unordered_map> // used by 'field_index' functions

namespace protogen_X_Y_Z {

using namespace protogen_X_Y_Z::internal;

struct json_context
{
    // Tokenizer object used for loading JSON during deserialization
    tokenizer *tok = nullptr;
    // Output stream for writing JSON during serialization
    ostream *os = nullptr;
    // Configuration parameters and error information
    Parameters params;
};

static int set_error( ErrorInfo &error, error_code code, const std::string &msg )
{
    if (error.code != error_code::PGERR_OK)
        return PGR_ERROR;
    error.code = code;
    error.message = msg;
    error.line = error.column = 0;
    return PGR_ERROR;
}

template<typename T, typename E = void> struct json;

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
    return PGR_OK;
}

#define PG_X_Y_Z_ENTITY(N,O,S) \
    struct N : public O, public protogen_X_Y_Z::message< O, S > \
    { \
        typedef O value_type; \
        typedef S serializer_type; \
        N() = default; \
        N( const N& ) = default; \
        N( N &&that ) = default; \
        N &operator=( const N & ) = default; \
        using protogen_X_Y_Z::message<O, S>::serialize; \
        using protogen_X_Y_Z::message<O, S>::deserialize; \
        bool deserialize( protogen_X_Y_Z::istream& in, protogen_X_Y_Z::Parameters *params = nullptr ) override \
        { \
            protogen_X_Y_Z::json_context ctx; \
            if (params != nullptr) { \
                params->error.clear(); \
                ctx.params = *params; \
            } \
            protogen_X_Y_Z::tokenizer tok(in, ctx.params); \
            ctx.tok = &tok; \
            int result = S::read(ctx, *this); \
            if (result == protogen_X_Y_Z::PGR_OK) return true; \
            if (params != nullptr) params->error = std::move(ctx.params.error); \
            return false; \
        } \
        bool serialize( protogen_X_Y_Z::ostream &out, protogen_X_Y_Z::Parameters *params = nullptr ) const override \
        { \
            protogen_X_Y_Z::json_context ctx; \
            ctx.os = &out; \
            if (params != nullptr) { \
                params->error.clear(); \
                ctx.params = *params; \
            } \
            int result = S::write(ctx, *this); \
            if (result == protogen_X_Y_Z::PGR_OK) return true; \
            if (params != nullptr) params->error = std::move(ctx.params.error); \
            return false; \
        } \
        void clear() override { S::clear(*this); } \
        bool empty() const override { return S::empty(*this); } \
        bool equal( const O &that ) const override { return S::equal(*this, that); } \
        void swap( O &that ) { S::swap(*this, that); } \
    };

#define PG_X_Y_Z_ENTITY_SERIALIZER(N,O,S) \
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
    };}

} // namespace protogen_X_Y_Z

#endif // PROTOGEN_X_Y_Z__JSON

