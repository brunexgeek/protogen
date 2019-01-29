/*
 * Copyright 2018 Bruno Ribeiro <https://github.com/brunexgeek>
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

#include <protogen/proto3.hh>
#include <iterator>
#include <sstream>


#define IS_LETTER(x)           ( ((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || (x) == '_' )
#define IS_DIGIT(x)            ( (x) >= '0' && (x) <= '9' )
#define IS_LETTER_OR_DIGIT(x)  ( IS_LETTER(x) || IS_DIGIT(x) )

#define IS_VALID_TYPE(x)      ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_MESSAGE )
#define TOKEN_POSITION(t)     (t).line, (t).column

#define TOKEN_EOF              0
#define TOKEN_MESSAGE          1
#define TOKEN_NAME             2
#define TOKEN_EQUAL            3
#define TOKEN_REPEATED         4
#define TOKEN_T_DOUBLE         6
#define TOKEN_T_FLOAT          7
#define TOKEN_T_INT32          8
#define TOKEN_T_INT64          9
#define TOKEN_T_UINT32         10
#define TOKEN_T_UINT64         11
#define TOKEN_T_SINT32         12
#define TOKEN_T_SINT64         13
#define TOKEN_T_FIXED32        14
#define TOKEN_T_FIXED64        15
#define TOKEN_T_SFIXED32       16
#define TOKEN_T_SFIXED64       17
#define TOKEN_T_BOOL           18
#define TOKEN_T_STRING         19
#define TOKEN_T_BYTES          20
#define TOKEN_T_MESSAGE        21
#define TOKEN_SYNTAX           22
#define TOKEN_QNAME            23
#define TOKEN_STRING           24
#define TOKEN_INTEGER          25
#define TOKEN_COMMENT          26
#define TOKEN_ENUM             27
#define TOKEN_SCOLON           28
#define TOKEN_PACKAGE          29
#define TOKEN_LT               30
#define TOKEN_GT               31
#define TOKEN_MAP              32
#define TOKEN_COMMA            33
#define TOKEN_BEGIN            34
#define TOKEN_END              35
#define TOKEN_OPTION           36
#define TOKEN_TRUE             37
#define TOKEN_FALSE            38
#define TOKEN_LBRACKET         39
#define TOKEN_RBRACKET         40


#ifdef BUILD_DEBUG

static const char *TOKENS[] =
{
    "TOKEN_EOF",
    "TOKEN_MESSAGE",
    "TOKEN_NAME",
    "TOKEN_EQUAL",
    "TOKEN_REPEATED",
    "TOKEN_T_DOUBLE",
    "TOKEN_T_FLOAT",
    "TOKEN_T_INT32",
    "TOKEN_T_INT64",
    "TOKEN_T_UINT32",
    "TOKEN_T_UINT64",
    "TOKEN_T_SINT32",
    "TOKEN_T_SINT64",
    "TOKEN_T_FIXED32",
    "TOKEN_T_FIXED64",
    "TOKEN_T_SFIXED32",
    "TOKEN_T_SFIXED64",
    "TOKEN_T_BOOL",
    "TOKEN_T_STRING",
    "TOKEN_T_BYTES",
    "TOKEN_T_MESSAGE",
    "TOKEN_SYNTAX",
    "TOKEN_QNAME",
    "TOKEN_STRING",
    "TOKEN_INTEGER",
    "TOKEN_COMMENT",
    "TOKEN_ENUM",
    "TOKEN_SCOLON",
    "TOKEN_PACKAGE",
    "TOKEN_LT",
    "TOKEN_GT",
    "TOKEN_MAP",
    "TOKEN_COMMA",
    "TOKEN_BEGIN",
    "TOKEN_END",
    "TOKEN_OPTION",
    "TOKEN_TRUE",
    "TOKEN_FALSE",
    "TOKEN_LBRACKET",
    "TOKEN_RBRACKET",
};

static const char *TYPES[] =
{
    "double",
    "float",
    "int32",
    "int64",
    "uint32",
    "uint64",
    "sint32",
    "sint64",
    "fixed32",
    "fixed64",
    "sfixed32",
    "sfixed64",
    "bool",
    "string",
    "bytes",
    nullptr,
};

#endif

static const struct
{
    int code;
    const char *keyword;
} KEYWORDS[] =
{
    { TOKEN_MESSAGE     , "message" },
    { TOKEN_REPEATED    , "repeated" },
    { TOKEN_T_STRING    , "string" },
    { TOKEN_ENUM        , "enum" },
    { TOKEN_T_DOUBLE    , "double" },
    { TOKEN_T_FLOAT     , "float" },
    { TOKEN_T_BOOL      , "bool" },
    { TOKEN_T_INT32     , "int32" },
    { TOKEN_T_INT64     , "int64" },
    { TOKEN_T_UINT32    , "uint32" },
    { TOKEN_T_UINT64    , "uint64" },
    { TOKEN_T_SINT32    , "sint32" },
    { TOKEN_T_SINT64    , "sint64" },
    { TOKEN_T_FIXED32   , "fixed32" },
    { TOKEN_T_FIXED64   , "fixed64" },
    { TOKEN_T_SFIXED32  , "sfixed32" },
    { TOKEN_T_SFIXED64  , "sfixed64" },
    { TOKEN_T_BYTES     , "bytes" },
    { TOKEN_PACKAGE     , "package" },
    { TOKEN_SYNTAX      , "syntax" },
    { TOKEN_MAP         , "map" },
    { TOKEN_OPTION      , "option" },
    { TOKEN_TRUE        , "true" },
    { TOKEN_FALSE       , "false" },
    { 0, nullptr },
};


namespace protogen {


template <typename I> class InputStream
{
    protected:
        I cur_, end_;
        int last_, line_, column_;
        bool ungot_;

    public:
        InputStream( const I& first, const I& last ) : cur_(first), end_(last),
            last_(-1), line_(1), column_(0), ungot_(false)
        {
        }

        bool eof()
        {
            return last_ == -2;
        }

        int get()
        {
            if (ungot_)
            {
                ungot_ = false;
                return last_;
            }
            if (last_ == '\n')
            {
                ++line_;
                column_ = 0;
            }
            if (cur_ == end_)
            {
                last_ = -2;
                return -1;
            }

            last_ = *cur_ & 0xFF;
            ++cur_;
            ++column_;
            return last_;
        }

        void unget()
        {
            if (last_ >= 0)
            {
                if (ungot_) throw std::runtime_error("unable to unget");
                ungot_ = true;
            }
        }

        int cur() const { return *cur_; }

        int line() const { return line_; }

        int column() const { return column_; }

        void skipws()
        {
            while (1) {
                int ch = get();
                if (! (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'))
                {
                    unget();
                    break;
                }
            }
        }

        bool expect(int expect)
        {
            if (get() != expect)
            {
                unget();
                return false;
            }
            return true;
        }
};


struct Token
{
    int code;
    std::string value;
    int line, column;

    Token( int code = TOKEN_EOF, const std::string &value = "", int line = 1, int column = 1) :
        code(code), value(value), line(line), column(column)
    {
    }

    template<typename I>
    Token( int code, const std::string &value, InputStream<I> &is ) : code(code), value(value)
    {
        line = is.line();
        column = is.column() - value.length();
    }

};


template <typename I> class Tokenizer
{
    public:
        Token current;
        bool ungot;

        Tokenizer( InputStream<I> &is ) : ungot(false), is(is)
        {
        }

        void unget()
        {
            if (ungot)
                throw exception("Already ungot", current.line, current.column);
            ungot = true;
        }

        Token next()
        {
            int line = 1;
            int column = 1;

            while (true)
            {
                if (ungot)
                {
                    ungot = false;
                    return current;
                }
                is.skipws();

                line = is.line();
                column = is.column();

                int cur = is.get();
                if (cur < 0) break;

                if (IS_LETTER(cur))
                {
                    is.unget();
                    current = qname(line, column);
                }
                else
                if (IS_DIGIT(cur))
                    current = integer(cur, line, column);
                else
                if (cur == '/')
                {
                    comment(); //discarding
                    continue;
                }
                else
                if (cur == '\n' || cur == '\r')
                    continue;
                else
                if (cur == '"')
                    current = literalString(line, column);
                else
                if (cur == '=')
                    current = Token(TOKEN_EQUAL, "", line, column);
                else
                if (cur == '{')
                    current = Token(TOKEN_BEGIN, "", line, column);
                else
                if (cur == '}')
                    current = Token(TOKEN_END, "", line, column);
                else
                if (cur == ';')
                    current = Token(TOKEN_SCOLON, "", line, column);
                else
                if (cur == ',')
                    current = Token(TOKEN_COMMA, "", line, column);
                else
                if (cur == '<')
                    current = Token(TOKEN_LT, "", line, column);
                else
                if (cur == '>')
                    current = Token(TOKEN_GT, "", line, column);
                else
                if (cur == '[')
                    current = Token(TOKEN_LBRACKET, "", line, column);
                else
                if (cur == ']')
                    current = Token(TOKEN_RBRACKET, "", line, column);
                else
                    throw exception("Invalid symbol", line, column);

                return current;
            }

            return current = Token(TOKEN_EOF, "", line, column);
        }

    private:
        InputStream<I> &is;

        Token comment()
        {
            Token temp(TOKEN_COMMENT, "");
            int cur = is.get();

            if (cur == '/')
            {
                while ((cur = is.get()) != '\n') temp.value += (char) cur;
                return temp;
            }
            else
            if (cur == '*')
            {
                while (true)
                {
                    cur = is.get();
                    if (cur == '*' && is.expect('/')) return temp;
                    if (cur < 0) return Token();
                    temp.value += (char) cur;
                }
            }

            return Token();
        }

        Token qname( int line = 1, int column = 1 )
        {
            // capture the identifier
            int type = TOKEN_NAME;
            std::string name = this->name();
            while (is.get() == '.')
            {
                type = TOKEN_QNAME;
                std::string cur = this->name();
                if (cur.length() == 0)
                    throw exception("Invalid identifier", line, column);
                name += '.';
                name += cur;
            }
            is.unget();

            // we found a keyword?
            if (type == TOKEN_NAME)
            {
                for (int i = 0; KEYWORDS[i].keyword != nullptr; ++i)
                {
                    if (name == KEYWORDS[i].keyword)
                    {
                        type = KEYWORDS[i].code;
                        break;
                    }
                }
            }

            return Token(type, name, line, column);
        }

        std::string name()
        {
            std::string temp;
            bool first = true;
            int cur;
            while ((cur = is.get()) >= 0)
            {
                if ((first && !IS_LETTER(cur)) || !IS_LETTER_OR_DIGIT(cur)) break;
                first = false;
                temp += (char)cur;
            }
            is.unget();
            return temp;
        }

        Token integer( int first = 0, int line = 1, int column = 1 )
        {
            Token tt(TOKEN_INTEGER, "", line, column);
            if (first != 0) tt.value += (char) first;

            do
            {
                int cur = is.get();
                if (!IS_DIGIT(cur))
                {
                    is.unget();
                    return tt;
                }
                tt.value += (char) cur;
            } while (true);

            return tt;
        }

        Token literalString(int line = 1, int column = 1 )
        {
            Token tt(TOKEN_STRING, "", line, column);

            do
            {
                int cur = is.get();
                if (cur == '\n' || cur == 0) return Token();
                if (cur == '"') return tt;
                tt.value += (char) cur;
            } while (true);

            return tt;
        }
};


template<typename T>
struct Context
{
    Tokenizer<T> &tokens;
    Proto3 &tree;
    InputStream<T> &is;
    std::string package;

    Context( Tokenizer<T> &tokenizer, Proto3 &tree, InputStream<T> &is ) :
        tokens(tokenizer), tree(tree), is(is)
    {
    }
};


typedef Context< std::istream_iterator<char> > ProtoContext;

} // namespace protogen

#ifdef BUILD_DEBUG

std::ostream &operator<<( std::ostream &out, const protogen::Token &tt );
std::ostream &operator<<( std::ostream &out, protogen::Message &message );
std::ostream &operator<<( std::ostream &out, protogen::Proto3 &proto );

#endif

namespace protogen {


TypeInfo::TypeInfo() : id(TYPE_DOUBLE), ref(nullptr), repeated(false)
{
}


Field::Field() : index(0)
{
}


std::string Message::qualifiedName() const
{
    if (package.empty()) return name;
    return package + '.' + name;
}


static OptionEntry parseOption( ProtoContext &ctx )
{
    // the token 'option' is already consumed at this point
    OptionEntry temp;

    // option name
    ctx.tokens.next();
    if (ctx.tokens.current.code != TOKEN_NAME && ctx.tokens.current.code != TOKEN_QNAME)
        throw exception("Missing option name", TOKEN_POSITION(ctx.tokens.current));
    temp.name = ctx.tokens.current.value;
    // equal symbol
    if (ctx.tokens.next().code != TOKEN_EQUAL)
        throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));
    // option value
    ctx.tokens.next();
    switch (ctx.tokens.current.code)
    {
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            temp.type = OptionType::BOOLEAN; break;
        case TOKEN_NAME:
        case TOKEN_QNAME:
            temp.type = OptionType::IDENTIFIER; break;
        case TOKEN_INTEGER:
            temp.type = OptionType::INTEGER; break;
        case TOKEN_STRING:
            temp.type = OptionType::STRING; break;
        default:
            throw exception("Invalid option value", TOKEN_POSITION(ctx.tokens.current));
    }
    temp.value = ctx.tokens.current.value;
    return temp;
}


static void parseFieldOptions( ProtoContext &ctx, OptionMap &entries )
{
    while (true)
    {
        if (ctx.tokens.next().code == TOKEN_RBRACKET) break;
        ctx.tokens.unget();
        OptionEntry option = parseOption(ctx);
        if (ctx.tokens.next().code != TOKEN_COMMA) ctx.tokens.unget();
        entries[option.name] = option;
    }
    // give back the TOKEN_RBRACKET
    ctx.tokens.unget();
}


static void parseStandardOption( ProtoContext &ctx, OptionMap &entries )
{
    // the token 'option' is already consumed at this point

    OptionEntry option = parseOption(ctx);

    // semicolon symbol
    if (ctx.tokens.next().code != TOKEN_SCOLON)
        throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));

    entries[option.name] = option;
}


static Message *findMessage( ProtoContext &ctx, const std::string &name )
{
    for (auto it = ctx.tree.messages.begin(); it != ctx.tree.messages.end(); ++it)
        if ((*it)->qualifiedName() == name) return *it;
    return nullptr;
}


static void parseField( ProtoContext &ctx, Message &message )
{
    Field field;

    if (ctx.tokens.current.code == TOKEN_REPEATED)
    {
        field.type.repeated = true;
        ctx.tokens.next();
    }
    else
        field.type.repeated = false;

    // type
    if (ctx.tokens.current.code >= TOKEN_T_DOUBLE && ctx.tokens.current.code <= TOKEN_T_BYTES)
        field.type.id = (FieldType) ctx.tokens.current.code;
    else
    if (ctx.tokens.current.code == TOKEN_NAME)
    {
        field.type.id = (FieldType) TOKEN_T_MESSAGE;
        if (!message.package.empty())
        {
            field.type.qname += message.package;
            field.type.qname += '.';
        }
        field.type.qname += ctx.tokens.current.value;
        field.type.ref = findMessage(ctx, field.type.qname);
    }
# if 0
    else
    if (ctx.tokens.current.code == TOKEN_MAP)
    {
        if (ctx.tokens.next().code != TOKEN_LT) throw exception("Missing <");

        // key type
        if (ctx.tokens.next().code != TOKEN_T_STRING) throw exception("Key type must be 'string'");
        field.type.id = (FieldType) ctx.tokens.current.code;
        // comma
        if (ctx.tokens.next().code != TOKEN_COMMA) throw exception("Missing comma");
        //value type
        if (ctx.tokens.next().code != TOKEN_T_STRING) throw exception("Key type must be 'string'");
        field.type.id = (FieldType) ctx.tokens.current.code;

        if (ctx.tokens.next().code != TOKEN_GT) throw exception("Missing >");
    }
#endif
    else
        throw exception("Missing field type", TOKEN_POSITION(ctx.tokens.current));

    // name
    if (ctx.tokens.next().code != TOKEN_NAME) throw exception("Missing field name", TOKEN_POSITION(ctx.tokens.current));
    field.name = ctx.tokens.current.value;
    // equal symbol
    if (ctx.tokens.next().code != TOKEN_EQUAL) throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));
    // index
    if (ctx.tokens.next().code != TOKEN_INTEGER) throw exception("Missing field index", TOKEN_POSITION(ctx.tokens.current));
    field.index = (int) strtol(ctx.tokens.current.value.c_str(), nullptr, 10);

    ctx.tokens.next();

    // options
    if (ctx.tokens.current.code == TOKEN_LBRACKET)
    {
        parseFieldOptions(ctx, field.options);
        if (ctx.tokens.next().code != TOKEN_RBRACKET)
            throw exception("Expected ']'", TOKEN_POSITION(ctx.tokens.current));
        ctx.tokens.next();
    }

    // semi-colon
    if (ctx.tokens.current.code != TOKEN_SCOLON)
        throw exception("Expected ';'", TOKEN_POSITION(ctx.tokens.current));

    message.fields.push_back(field);
}


void Message::splitPackage(
    std::vector<std::string> &out )
{
    std::string current;

    const char *ptr = package.c_str();
    while (true)
    {
        if (*ptr == '.' || *ptr == 0)
        {
            if (!current.empty())
            {
                out.push_back(current);
                current.clear();
            }
            if (*ptr == 0) break;
        }
        else
            current += *ptr;
        ++ptr;
    }
}


static void parseMessage( ProtoContext &ctx )
{
    if (ctx.tokens.current.code == TOKEN_MESSAGE && ctx.tokens.next().code == TOKEN_NAME)
    {
        Message *message = new(std::nothrow) Message();
        if (message == nullptr)
            throw exception("Out of memory");
        //splitPackage(message.package, ctx.package);
        message->package = ctx.package;
        message->name = ctx.tokens.current.value;
        if (ctx.tokens.next().code == TOKEN_BEGIN)
        {
            while (ctx.tokens.next().code != TOKEN_END)
            {
                if (ctx.tokens.current.code == TOKEN_OPTION)
                    parseStandardOption(ctx, message->options);
                else
                    parseField(ctx, *message);
            }
        }
        ctx.tree.messages.push_back(message);
    }
    else
        throw exception("Invalid message");
}


static void parsePackage( ProtoContext &ctx )
{
    Token tt = ctx.tokens.next();
    if ((tt.code == TOKEN_NAME || tt.code == TOKEN_QNAME) && ctx.tokens.next().code == TOKEN_SCOLON)
    {
        ctx.package = tt.value;
    }
    else
        throw exception("Invalid package");
}


static void parseSyntax( ProtoContext &ctx )
{
    // the token 'syntax' is already consumed at this point

    if (ctx.tokens.next().code != TOKEN_EQUAL)
        throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));
    Token tt = ctx.tokens.next();
    if (tt.code == TOKEN_STRING && ctx.tokens.next().code == TOKEN_SCOLON)
    {
        if (tt.value != "proto3") throw exception("Invalid language version", TOKEN_POSITION(ctx.tokens.current));
    }
    else
        throw exception("Invalid package");
}


static void parseProto( ProtoContext &ctx )
{
    do
    {
        ctx.tokens.next();
        if (ctx.tokens.current.code == TOKEN_MESSAGE)
            parseMessage(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_PACKAGE)
            parsePackage(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_COMMENT)
            continue;
        else
        if (ctx.tokens.current.code == TOKEN_SYNTAX)
            parseSyntax(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_OPTION)
            parseStandardOption(ctx, ctx.tree.options);
        else
        if (ctx.tokens.current.code == TOKEN_EOF)
            break;
        else
        {
            throw exception("Unexpected token", TOKEN_POSITION(ctx.tokens.current));
        }
    } while (ctx.tokens.current.code != 0);
}


Proto3::~Proto3()
{
    for (auto it = messages.begin(); it != messages.end(); ++it) delete *it;
}


void Proto3::parse( Proto3 &tree, std::istream &input, const std::string &fileName )
{
    std::ios_base::fmtflags flags = input.flags();
    std::noskipws(input);
    std::istream_iterator<char> end;
    std::istream_iterator<char> begin(input);

    InputStream< std::istream_iterator<char> > is(begin, end);
    Tokenizer< std::istream_iterator<char> > tok(is);

    ProtoContext ctx(tok, tree, is);
    tree.fileName = fileName;

    try
    {
        parseProto(ctx);
        if (flags & std::ios::skipws) std::skipws(input);
    } catch (exception &ex)
    {
        if (flags & std::ios::skipws) std::skipws(input);
        throw ex;
    }

    // check if we have unresolved types
    for (auto mit = tree.messages.begin(); mit != tree.messages.end(); ++mit)
    {
        for (auto fit = (*mit)->fields.begin(); fit != (*mit)->fields.end(); ++fit)
        {
            if (fit->type.ref != nullptr || fit->type.id != TYPE_MESSAGE) continue;

            fit->type.ref = findMessage(ctx, fit->type.qname);
            if (fit->type.ref == nullptr)
                throw exception("Unable to find message '" + fit->type.qname + "'");
        }
    }
}




} // protogen


#ifdef BUILD_DEBUG

std::ostream &operator<<( std::ostream &out, const protogen::Token &tt )
{
    out << '[' << TOKENS[tt.code] << ": ";
    if (!tt.value.empty()) out << "value='" << tt.value << "' ";
    out << "pos=" << tt.line << ':' << tt.column << "]";
    return out;
}


std::ostream &operator<<( std::ostream &out, protogen::Field &field )
{
    if (field.type.repeated)
        out << "repeated ";
    if (field.type.id >= TOKEN_T_DOUBLE && field.type.id <= TOKEN_T_BYTES)
        out << TYPES[field.type.id - TOKEN_T_DOUBLE];
    else
        out << field.type.qname;
    out << ' ' << field.name << " = " << field.index << ";";
    return out;
}


std::ostream &operator<<( std::ostream &out, protogen::Message &message )
{
    out << "message " << message.name << " {" << std::endl;;

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        out << *it << std::endl;;
    }
    out << '}' << std::endl;
    return out;
}


std::ostream &operator<<( std::ostream &out, protogen::Proto3 &proto )
{
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
    {
        out << *it << '\n';
    }
    return out;
}

#endif