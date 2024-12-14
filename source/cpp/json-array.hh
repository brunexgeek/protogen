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

#ifndef PROTOGEN_X_Y_Z__JSON_ARRAY
#define PROTOGEN_X_Y_Z__JSON_ARRAY

namespace protogen_X_Y_Z {

template<typename T, typename _ = void>
struct is_container : std::false_type {};

template<typename... Ts>
struct is_container_helper {};

template<typename T>
struct is_container<
        T,
        typename std::conditional<
            false,
            is_container_helper<
                typename T::value_type,
                typename T::size_type,
                typename T::allocator_type,
                typename T::iterator,
                typename T::const_iterator,
                decltype(std::declval<T>().size()),
                decltype(std::declval<T>().begin()),
                decltype(std::declval<T>().end()),
                decltype(std::declval<T>().clear()),
                decltype(std::declval<T>().empty())
                >,
            void
            >::type
        > : public std::true_type {};

template<typename T>
struct json<T, typename std::enable_if<is_container<T>::value>::type >
{
    static int read( json_context &ctx, T &value )
    {
        if (ctx.tok->peek().id == token_id::NIL) return PGR_NIL;
        if (!ctx.tok->expect(token_id::ARRS))
            return ctx.tok->error(error_code::PGERR_INVALID_ARRAY, "invalid array");
        if (!ctx.tok->expect(token_id::ARRE))
        {
            while (true)
            {
                typename T::value_type temp;
                int result = json<typename T::value_type>::read(ctx, temp);
                if (result == PGR_ERROR) return result;
                if (result == PGR_OK) value.push_back(temp);

                if (!ctx.tok->expect(token_id::COMMA))
                {
                    if (ctx.tok->expect(token_id::ARRE))
                        break;
                    return ctx.tok->error(error_code::PGERR_INVALID_ARRAY, "invalid array");
                }
            }
        }
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

//
// Deserialization of arrays
//

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool deserialize_array( T& container, protogen_X_Y_Z::istream& in, protogen_X_Y_Z::Parameters *params = nullptr )
{
    protogen_X_Y_Z::json_context ctx;
    if (params != nullptr) {
        params->error.clear();
        ctx.params = *params;
    }
    protogen_X_Y_Z::tokenizer tok(in, ctx.params);
    ctx.tok = &tok;
    int result = json<T>::read(ctx, container);
    if (result == protogen_X_Y_Z::PGR_OK) return true;
    if (params != nullptr) params->error = std::move(ctx.params.error);
    return false;
}

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool deserialize_array( T& container, const std::string &in, Parameters *params = nullptr )
{
    iterator_istream<std::string::const_iterator> is(in.begin(), in.end());
    return deserialize_array(container, is, params);
}

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool deserialize_array( T& container, const char *in, size_t len, Parameters *params = nullptr )
{
    auto begin = mem_iterator<char>(in, len);
    auto end = mem_iterator<char>(in + len, 0);
    iterator_istream<mem_iterator<char>> is(begin, end);
    return deserialize_array(container, is, params);
}

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool deserialize_array( T& container, const std::vector<char> &in, Parameters *params = nullptr )
{
    iterator_istream<std::vector<char>::const_iterator> is(in.begin(), in.end());
    return deserialize_array(container, is, params);
}

//
// Serialization of arrays
//

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool serialize_array( const T& container, protogen_X_Y_Z::ostream &out, protogen_X_Y_Z::Parameters *params = nullptr )
{
    protogen_X_Y_Z::json_context ctx;
    ctx.os = &out;
    if (params != nullptr) {
        params->error.clear();
        ctx.params = *params;
    }
    int result = json<T>::write(ctx, container);
    if (result == protogen_X_Y_Z::PGR_OK) return true;
    if (params != nullptr) params->error = std::move(ctx.params.error);
    return false;
}

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool serialize_array( const T& container, std::string &out, Parameters *params = nullptr )
{
    typedef std::back_insert_iterator<std::string> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    return serialize_array(container, os, params);
}

template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
bool serialize_array( const T& container, std::vector<char> &out, Parameters *params = nullptr )
{
    typedef std::back_insert_iterator<std::vector<char>> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    return serialize_array(container, os, params);
}

} // namespace protogen_X_Y_Z

#endif // PROTOGEN_X_Y_Z__JSON_ARRAY
