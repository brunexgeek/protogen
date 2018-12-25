#include <protogen/proto3.hh>
#include <iterator>
#include <sstream>


#define IS_LETTER(x)           ( ((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || (x) == '_' )
#define IS_DIGIT(x)            ( (x) >= '0' && (x) <= '9' )
#define IS_LETTER_OR_DIGIT(x)  ( IS_LETTER(x) || IS_DIGIT(x) )

#define IS_VALID_TYPE(x)      ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_MESSAGE )

#define TOKEN_NONE             0
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
#define TOKEN_BEGIN            22
#define TOKEN_END              23
#define TOKEN_STRING           24
#define TOKEN_INTEGER          25
#define TOKEN_COMMENT          26
#define TOKEN_ENUM             27
#define TOKEN_SCOLON           28
#define TOKEN_PACKAGE          29
#define TOKEN_QNAME            30
#define TOKEN_LT               31
#define TOKEN_GT               32
#define TOKEN_MAP              33
#define TOKEN_COMMA            34


static const char *TOKENS[] =
{
    "TOKEN_NONE",
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
    "TOKEN_BEGIN",
    "TOKEN_END",
    "TOKEN_STRING",
    "TOKEN_INTEGER",
    "TOKEN_COMMENT",
    "TOKEN_ENUM",
    "TOKEN_SCOLON",
    "TOKEN_PACKAGE",
    "TOKEN_QNAME",
    "TOKEN_LT",
    "TOKEN_GT",
    "TOKEN_MAP",
    "TOKEN_COMMA",
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
    { TOKEN_MAP         , "map" },
    { 0, nullptr },
};


namespace protogen {


template <typename Iter> class InputStream
{
    protected:
        Iter cur_, end_;
        int last_ch_, prev_;
        bool ungot_;
        int line_, column_;
    public:
        InputStream( const Iter& first, const Iter& last ) : cur_(first), end_(last),
            last_ch_(-1), prev_(-1), ungot_(false), line_(1), column_(1)
        {
        }

        bool eof()
        {
            return last_ch_ == -2;
        }

        int prev()
        {
            if (prev_ >= 0)
                return prev_;
            else
                return -1;
        }

        int get()
        {
            if (!ungot_) prev_ = last_ch_;

            if (ungot_)
            {
                ungot_ = false;
                return last_ch_;
            }

            if (cur_ == end_)
            {
                last_ch_ = -2;
                return -1;
            }
            if (last_ch_ == '\n')
            {
                line_++;
                column_ = 1;
            }
            last_ch_ = *cur_ & 0xff;
            ++cur_;
            ++column_;
            return last_ch_;
        }

        void unget()
        {
            if (last_ch_ >= 0)
            {
                if (ungot_) throw std::runtime_error("unable to unget");
                ungot_ = true;
            }
        }

        int cur() const { return *cur_; }

        int line() const { return line_; }

        int column() const { return column_; }

        void skip_ws()
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

    Token( int code = TOKEN_NONE ) : code(code)
    {
    }

    Token( int code, const std::string &value ) : code(code), value(value)
    {
    }

};


std::ostream &operator<<( std::ostream &out, const Token &tt )
{
    out << '[' << TOKENS[tt.code];
    if (!tt.value.empty()) out << ':' << tt.value;
    out << ']';
    return out;
}


template <typename Iter> class Tokenizer
{
    public:
        Token current;

        Tokenizer( InputStream<Iter> &is ) : is(is)
        {
        }

        Token next()
        {
            while (true)
            {
                is.skip_ws();
                int cur = is.get();
                if (cur < 0) break;

                if (IS_LETTER(cur))
                    current = identifier(cur);
                else
                if (IS_DIGIT(cur))
                    current = integer(cur);
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
                    current = literalString();
                else
                if (cur == '=')
                    current = Token(TOKEN_EQUAL);
                else
                if (cur == '{')
                    current = Token(TOKEN_BEGIN);
                else
                if (cur == '}')
                    current = Token(TOKEN_END);
                else
                if (cur == ';')
                    current = Token(TOKEN_SCOLON);
                else
                if (cur == ',')
                    current = Token(TOKEN_COMMA);
                else
                if (cur == '<')
                    current = Token(TOKEN_LT);
                else
                if (cur == '>')
                    current = Token(TOKEN_GT);
                else
                    throw exception("Invalid symbol");

                //std::cout << current << std::endl;
                return current;
            }

            return current = Token();
        }

    private:
        InputStream<Iter> &is;

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

        Token identifier( int first = 0 )
        {
            Token temp(TOKEN_NAME, "");
            if (first != 0) temp.value += (char) first;
            int cur = -1;
            while ((cur = is.get()) >= 0)
            {
                if (!IS_LETTER_OR_DIGIT(cur) && cur != '.')
                {
                    is.unget();
                    break;
                }
                if (cur == '.') temp.code = TOKEN_QNAME;
                temp.value += (char)cur;
            }

            if (temp.code == TOKEN_NAME)
            {
                for (int i = 0; KEYWORDS[i].keyword != nullptr; ++i)
                {
                    if (temp.value == KEYWORDS[i].keyword)
                    {
                        temp.code = KEYWORDS[i].code;
                        break;
                    }
                }
            }

            return temp;
        }

        Token integer( int first = 0 )
        {
            Token tt(TOKEN_INTEGER, "");
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

        Token literalString()
        {
            Token tt(TOKEN_STRING, "");

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
    std::string package;
    Proto3 &tree;
    InputStream<T> &is;

    Context( Tokenizer<T> tokenizer, Proto3 &tree, InputStream<T> &is ) :
        tokens(tokenizer), tree(tree), is(is)
    {
    }
};


typedef Context< std::istream_iterator<char> > ProtoContext;


Field::Field() : index(0), repeated(false)
{
}


Proto3::Proto3()
{
}

Proto3::~Proto3()
{
}


static void parseField( ProtoContext &ctx, Message &message )
{
    Field field;

    if (ctx.tokens.current.code == TOKEN_REPEATED)
    {
        field.repeated = true;
        ctx.tokens.next();
    }

    // type
    if (ctx.tokens.current.code >= TOKEN_T_DOUBLE && ctx.tokens.current.code <= TOKEN_T_BYTES)
        field.type.id = (FieldType) ctx.tokens.current.code;
    else
    if (ctx.tokens.current.code == TOKEN_NAME)
    {
        field.type.id = (FieldType) TOKEN_T_MESSAGE;
        field.type.name = ctx.tokens.current.value;
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
        throw exception("Missing field type");

    // name
    if (ctx.tokens.next().code != TOKEN_NAME) throw exception("Missing field name");
    field.name = ctx.tokens.current.value;
    // equal symbol
    if (ctx.tokens.next().code != TOKEN_EQUAL) throw exception("Expected '='");
    // index
    if (ctx.tokens.next().code != TOKEN_INTEGER) throw exception("Missing field index");
    field.index = (int) strtol(ctx.tokens.current.value.c_str(), nullptr, 10);
    // semi-colon
    if (ctx.tokens.next().code != TOKEN_SCOLON) throw exception("Expected ';'");

    message.fields.push_back(field);
}


static void splitPackage(
    std::vector<std::string> &out,
    const std::string &package )
{
    std::string current;

    const char *ptr = package.c_str();
    while (true)
    {
        if (*ptr == '.' || *ptr == 0)
        {
            out.push_back(current);
            current.clear();
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
        Message message;
        message.name = ctx.tokens.current.value;
        if (ctx.tokens.next().code == TOKEN_BEGIN)
        {
            while (ctx.tokens.next().code != TOKEN_END)
            {
                parseField(ctx, message);
            }
        }
        splitPackage(message.package, ctx.package);
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


static void parseProto( ProtoContext &ctx )
{
    try
    {
        do
        {
            ctx.tokens.next();
            if (ctx.tokens.current.code == TOKEN_MESSAGE)
                parseMessage(ctx);
            else
            if (ctx.tokens.current.code == TOKEN_PACKAGE)
                parsePackage(ctx);
        } while (ctx.tokens.current.code != 0);
    } catch (exception &ex)
    {
        ex.line = ctx.is.line();
        ex.column = ctx.is.column();
        throw ex;
    }
}


void Proto3::parse( Proto3 &tree, std::istream &input, std::string fileName )
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
}




} // protogen



std::ostream &operator<<( std::ostream &out, protogen::Field &field )
{
    if (field.repeated)
        out << "repeated ";
    if (field.type.id >= TOKEN_T_DOUBLE && field.type.id <= TOKEN_T_BYTES)
        out << TYPES[field.type.id - TOKEN_T_DOUBLE];
    else
        out << field.type.name;
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