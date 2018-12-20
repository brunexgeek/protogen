#include <protogen/proto3.hh>
#include <iterator>
#include <sstream>


#ifndef PICOJSON_ASSERT
# define PICOJSON_ASSERT(e) do { if (! (e)) throw exception(#e); } while (0)
#endif

#define IS_LETTER(x)   ( ((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || (x) == '_' )
#define IS_DIGIT(x)    ( (x) >= '0' && (x) <= '9' )
#define IS_LETTER_OR_DIGIT(x)  ( IS_LETTER(x) || IS_DIGIT(x) )


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
    { TOKEN_ENUM        , "enum" },
    { TOKEN_T_DOUBLE    , "double" },
    { TOKEN_T_FLOAT     , "float" },
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
    { TOKEN_T_BOOL      , "bool" },
    { TOKEN_T_STRING    , "string" },
    { TOKEN_T_BYTES     , "bytes" },
    { 0, nullptr },
};


namespace protogen {


exception::exception( const std::string &message, int line, int column ) :
    line(line), column(column), message(message)
{
}

exception::~exception()
{
}

const char *exception::what() const throw()
{
    return message.c_str();
}

const std::string exception::cause() const
{
    std::stringstream ss;
    ss << message << ' ' << line << ':' << column;
    return ss.str();
}

template <typename Iter> class InputStream {
    protected:
        Iter cur_, end_;
        int last_ch_;
        bool ungot_;
        int line_, column_;
    public:
        InputStream( const Iter& first, const Iter& last ) : cur_(first), end_(last),
            last_ch_(-1), ungot_(false), line_(1), column_(1)
        {
        }

        int getc()
        {
            if (ungot_)
            {
                ungot_ = false;
                return last_ch_;
            }

            if (cur_ == end_)
            {
                last_ch_ = -1;
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

        void ungetc()
        {
            if (last_ch_ != -1)
            {
                PICOJSON_ASSERT(! ungot_);
                ungot_ = true;
            }
        }

        Iter cur() const { return cur_; }

        int line() const { return line_; }

        int column() const { return column_; }

        void skip_ws()
        {
            while (1) {
                int ch = getc();
                if (! (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'))
                {
                    ungetc();
                    break;
                }
            }
        }

        bool expect(int expect)
        {
            skip_ws();
            if (getc() != expect)
            {
                ungetc();
                return false;
            }
            return true;
        }

        bool match(const std::string& pattern)
        {
            for (std::string::const_iterator pi(pattern.begin()); pi != pattern.end(); ++pi)
            {
                if (getc() != *pi)
                {
                    ungetc();
                    return false;
                }
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
                int cur = is.getc();
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
            int cur = is.getc();

            if (cur == '/')
            {
                while ((cur = is.getc()) != '\n') temp.value += (char) cur;
                return temp;
            }

            return Token();
        }

        Token identifier( int first = 0 )
        {
            Token temp(TOKEN_NAME, "");
            if (first != 0) temp.value += (char) first;
            int cur = -1;
            while ((cur = is.getc()) >= 0)
            {
                if (!IS_LETTER_OR_DIGIT(cur) /*&& cur != '.'*/)
                {
                    is.ungetc();
                    break;
                }
                temp.value += (char)cur;
            }

            for (int i = 0; KEYWORDS[i].keyword != nullptr; ++i)
            {
                if (temp.value == KEYWORDS[i].keyword)
                {
                    temp.code = KEYWORDS[i].code;
                    break;
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
                int cur = is.getc();
                if (!IS_DIGIT(cur))
                {
                    is.ungetc();
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
                int cur = is.getc();
                if (cur == '\n' || cur == 0) return Token();
                if (cur == '"') return tt;
                tt.value += (char) cur;
            } while (true);

            return tt;
        }
};


Field::Field() : type(TYPE_DOUBLE), index(0), repeated(false)
{
}


Proto3::Proto3()
{
}

Proto3::~Proto3()
{
}


template <typename Iter>
static Field parseField( Tokenizer<Iter> &tk )
{
    Field field;

    if (tk.current.code == TOKEN_REPEATED)
    {
        field.repeated = true;
        tk.next();
    }

    if (tk.current.code >= TOKEN_T_DOUBLE && tk.current.code <= TOKEN_T_BYTES)
        field.type = (FieldType) tk.current.code;
    else
    if (tk.current.code == TOKEN_NAME)
    {
        field.type = (FieldType) TOKEN_T_MESSAGE;
        field.typeName = tk.current.value;
    }
    else
        throw exception("Missing field type");

    if (tk.next().code != TOKEN_NAME) throw exception("Missing field name");
    field.name = tk.current.value;
    if (tk.next().code != TOKEN_EQUAL) throw exception("Expected '='");
    if (tk.next().code != TOKEN_INTEGER) throw exception("Missing field index");
    field.index = (int) strtol(tk.current.value.c_str(), nullptr, 10);
    if (tk.next().code != TOKEN_SCOLON) throw exception("Expected ';'");

    return field;
}


template <typename Iter>
static Message parseMessage( Tokenizer<Iter> &tk )
{
    if (tk.current.code == TOKEN_MESSAGE && tk.next().code == TOKEN_NAME)
    {
        Message message;
        message.name = tk.current.value;
        if (tk.next().code == TOKEN_BEGIN)
        {
            while (tk.next().code != TOKEN_END)
            {
                message.fields.push_back(parseField(tk));
            }
            return message;
        }

    }

    throw exception("Invalid message");
}


Proto3 *Proto3::parse( std::istream &input )
{
    std::ios_base::fmtflags flags = input.flags();
    std::noskipws(input);
    std::istream_iterator<char> end;
    std::istream_iterator<char> begin(input);

    InputStream< std::istream_iterator<char> > is(begin, end);
    Tokenizer< std::istream_iterator<char> > tok(is);

    Proto3 *output = new Proto3();

    try {

        do
        {
            tok.next();
            if (tok.current.code == TOKEN_MESSAGE)
            {
                output->messages.push_back(parseMessage(tok));
            }
        } while (tok.current.code != 0);
    } catch (exception &ex)
    {
        ex.line = is.line();
        ex.column = is.column();
        std::cerr << ex.cause() << std::endl;
        throw ex;
    }

    if (flags & std::ios::skipws) std::skipws(input);

    return output;
}


} // protogen



std::ostream &operator<<( std::ostream &out, protogen::Field &field )
{
    if (field.repeated)
        out << "repeated ";
    if (field.type >= TOKEN_T_DOUBLE && field.type <= TOKEN_T_BYTES)
        out << TYPES[field.type - TOKEN_T_DOUBLE];
    else
        out << field.typeName;
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