
#define PROTOGEN_TRAIT_MACRO(MSGTYPE) \
	namespace PROTOGEN_NS { \
		template<> struct traits<MSGTYPE> { \
			static void clear( MSGTYPE &value ) { value.clear(); } \
			static void write( std::ostream &out, const MSGTYPE &value ) { value.serialize(out); } \
			template<typename I> static bool read( PROTOGEN_NS::InputStream<I> &in, MSGTYPE &value ) { return value.deserialize(in); } \
			static void swap( MSGTYPE &a, MSGTYPE &b ) { a.swap(b); } \
		}; \
	}

#if __cplusplus >= 201103L
#define PROTOGEN_FIELD_MOVECTOR_TEMPLATE(MSGTYPE) Field( Field<MSGTYPE> &&that ) { this->value_.swap(that.value_); }
#else
#define PROTOGEN_FIELD_MOVECTOR_TEMPLATE(MSGTYPE)
#endif

#define PROTOGEN_FIELD_TEMPLATE(MSGTYPE) \
	namespace PROTOGEN_NS { \
        template<> class Field<MSGTYPE> { \
		protected: \
			MSGTYPE value_; \
		public: \
			Field() { clear(); } \
			PROTOGEN_FIELD_MOVECTOR_TEMPLATE(MSGTYPE); \
			void swap( Field<MSGTYPE> &that ) { traits<MSGTYPE>::swap(this->value_, that.value_); } \
			const MSGTYPE &operator()() const { return value_; } \
			MSGTYPE &operator()() { return value_; } \
			void operator ()(const MSGTYPE &value ) { this->value_ = value; } \
			bool undefined() const { return value_.undefined(); } \
			void clear() { traits<MSGTYPE>::clear(value_); } \
			Field<MSGTYPE> &operator=( const Field<MSGTYPE> &that ) { this->value_ = that.value_; return *this; } \
			bool operator==( const MSGTYPE &that ) const { return this->value_ == that; } \
			bool operator==( const Field<MSGTYPE> &that ) const { return this->value_ == that.value_; } \
	    }; \
    }


namespace PROTOGEN_NS {

enum FieldType
{
    TYPE_DOUBLE       =  6,
    TYPE_FLOAT        =  7,
    TYPE_INT32        =  8,
    TYPE_INT64        =  9,
    TYPE_UINT32       =  10,
    TYPE_UINT64       =  11,
    TYPE_SINT32       =  12,
    TYPE_SINT64       =  13,
    TYPE_FIXED32      =  14,
    TYPE_FIXED64      =  15,
    TYPE_SFIXED32     =  16,
    TYPE_SFIXED64     =  17,
    TYPE_BOOL         =  18,
    TYPE_STRING       =  19,
    TYPE_BYTES        =  20,
    TYPE_MESSAGE      =  21
};


struct ErrorInfo
{
    int line;
    int column;
    std::string message;
    bool set;

    ErrorInfo() : line(0), column(0), set(false) {}
};


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
                if (ungot_) throw std::logic_error("unable to unget");
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

// proto3 numerics
template<typename T> struct traits
{
    static void clear( T &value ) { value = (T) 0; }
    static void write( std::ostream &out, const T &value ) { out << value; }
    template<typename I>
    static bool read( InputStream<I> &in, T &value )
    {
        in.skipws();
        std::string temp;
        while (true)
        {
            int ch = in.get();
            if (ch != '.' && ch != '-' && (ch < '0' || ch > '9'))
            {
                in.unget();
                break;
            }
            temp += (char) ch;
        }
        if (temp.empty())
            return false;

#if defined(_WIN32) || defined(_WIN64)
        static _locale_t loc = _create_locale(LC_NUMERIC, "C");
        if (loc == NULL) return false;
        value = (T) _strtod_l(temp.c_str(), NULL, loc);
#else
        static locale_t loc = newlocale(LC_NUMERIC_MASK | LC_MONETARY_MASK, "C", 0);
        if (loc == 0) return false;
        uselocale(loc);
        value = (T) strtod(temp.c_str(), NULL);
        uselocale(LC_GLOBAL_LOCALE);
#endif
        return true;
    }
    static void swap( T &a, T &b ) { T temp = a; a = b; b = temp; }
};

// proto3 'bool'
template<> struct traits<bool>
{
    static void clear( bool &value ) { value = false; }
    static void write( std::ostream &out, const bool &value ) { out << ((value) ? "true" : "false"); }
    template<typename I>
    static bool read( InputStream<I> &in, bool &value )
    {
        in.skipws();
        std::string temp;
        while (true)
        {
            int ch = in.get();
            switch (ch)
            {
                case 't':
                case 'r':
                case 'u':
                case 'e':
                case 'f':
                case 'a':
                case 'l':
                case 's':
                    temp += (char) ch;
                    break;
                default:
                    in.unget();
                    goto ESCAPE;
            }
        }
    ESCAPE:
        if (temp == "true")
            value = true;
        else
        if (temp == "false")
            value = false;
        else
            return false;
        return true;
    }
    static void swap( bool &a, bool &b ) { bool temp = a; a = b; b = temp; }
};

// proto3 'string'
template<> struct traits<std::string>
{
    static void clear( std::string &value ) { value.clear(); }
    static void write( std::ostream &out, const std::string &value )
    {
        out << '"';
        for (std::string::const_iterator it = value.begin(); it != value.end(); ++it)
        {
            switch (*it)
            {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '/':  out << "\\/"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\r': out << "\\r"; break;
                case '\n': out << "\\n"; break;
                case '\t': out << "\\t"; break;
                default:   out << *it;
            }
        }
        out << '"';
    }
    template <typename I>
    static bool read( InputStream<I> &in, std::string &value )
    {
        in.skipws();
        if (in.get() != '"') return false;
        while (true)
        {
            int ch = in.get();
            if (ch == '"') return true;

            if (ch == '\\')
            {
                ch = in.get();
                switch (ch)
                {
                    case '"':  ch = '"'; break;
                    case '\\': ch = '\\'; break;
                    case '/':  ch = '/'; break;
                    case 'b':  ch = '\b'; break;
                    case 'f':  ch = '\f'; break;
                    case 'r':  ch = '\r'; break;
                    case 'n':  ch = '\n'; break;
                    case 't':  ch = '\t'; break;
                    default: return false;
                }
            }

            if (ch <= 0) return false;
            value += (char) ch;
        }

        return false;
    }
    static void swap( std::string &a, std::string &b ) { a.swap(b); }
};

// proto3 'repeated'
template <typename T> struct traits< std::vector<T> >
{
    static void clear( std::vector<T> &value ) { value.clear(); }

    static bool write( std::ostream &out, const std::vector<T> &value )
    {
        bool begin = true;
        out << '[';
        for (typename std::vector<T>::const_iterator it = value.begin(); it != value.end(); ++it)
        {
            if (!begin) out << ",";
            begin = false;
            traits<T>::write(out, *it);
        }
        out << ']';
        return true;
    }

    template<typename I>
    static bool read( InputStream<I> &in, std::vector<T> &value )
    {
        in.skipws();
        if (in.get() != '[') return false;
        in.skipws();
        while (true)
        {
            T entry;
            if (!traits<T>::read(in, entry)) return false;
            value.push_back(entry);
            in.skipws();
            if (!in.expect(',')) break;
        }
        if (in.get() != ']') return false;

        return true;
    }

    static void swap( std::vector<T> &a, std::vector<T> &b ) { a.swap(b); }
};

// Base64 encoder/decoder based on Joe DF's implementation
// Original source at <https://github.com/joedf/base64.c> (MIT licensed)
static const char *B64_SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
template <> struct traits< std::vector<uint8_t> >
{
    static void clear( std::vector<uint8_t> &value ) { value.clear(); }

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

    static bool write( std::ostream &out, const std::vector<uint8_t> &value )
    {
        char o[5] = { 0 };
        size_t i = 0;
        size_t size = value.size();

        out << '"';

        for (i = 0; i + 2 < size; i += 3)
        {
            o[0] = B64_SYMBOLS[ (value[i] & 0xFF) >> 2 ];
            o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) | ((value[i + 1] & 0xF0) >> 4) ];
            o[2] = B64_SYMBOLS[ ((value[i+1] & 0x0F) << 2) | ((value[i+2] & 0xC0) >> 6) ];
            o[3] = B64_SYMBOLS[ value[i+2] & 0x3F ];
            out << o;
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

            out << o;
        }
        out << '"';

        return true;
    }

    template<typename I>
    static bool read( InputStream<I> &in, std::vector<uint8_t> &array )
    {
        size_t k = 0;
        int s[4];

        if (in.get() != '"') return false;

        while (true)
        {
            // read 4 characters
            for (size_t j = 0; j < 4; ++j)
            {
                int ch = in.get();
                if (ch == '"') return j == 0;
                s[j] = PROTOGEN_NS::traits< std::vector<uint8_t> >::b64_int(ch);
            }
            // decode base64 tuple
            array.push_back( ((s[0] & 0xFF) << 2 ) | ((s[1] & 0x30) >> 4) );
            if (s[2] != 64)
            {
                array.push_back( ((s[1] & 0x0F) << 4) | ((s[2] & 0x3C) >> 2) );
                if ((s[3]!=64))
                {
                    array.push_back( ((s[2] & 0x03) << 6) | s[3] );
                    k+=3;
                }
                else
                    k+=2;
            }
            else
                k+=1;
        }
    }

    static void swap( std::vector<uint8_t> &a, std::vector<uint8_t> &b ) { a.swap(b); }
};

// primitive fields
template<typename T> class Field
{
    protected:
        T value_;
        bool undefined_;
    public:
        Field() { clear(); }
        Field( const Field<T> &that ) { this->undefined_ = that.undefined_; if (!undefined_) this->value_ = that.value_; }
        #if __cplusplus >= 201103L
        Field( Field<T> &&that ) { this->undefined_ = that.undefined_; if (!undefined_) this->value_.swap(that.value_); }
        #endif
        void swap( Field<T> &that ) { traits<T>::swap(this->value_, that.value_); traits<bool>::swap(this->undefined_, that.undefined_); }
        const T &operator()() const { return value_; }
        void operator ()(const T &value ) { this->value_ = value; this->undefined_ = false; }
        bool undefined() const { return undefined_; }
        void clear() { traits<T>::clear(value_); undefined_ = true; }
        Field<T> &operator=( const Field<T> &that ) { this->undefined_ = that.undefined_; if (!undefined_) this->value_ = that.value_; return *this; }
        bool operator==( const T &that ) const { return !this->undefined_ && this->value_ == that; }
        bool operator==( const Field<T> &that ) const { return this->undefined_ == that.undefined_ && this->value_ == that.value_;  }
};

template<typename T> class RepeatedField
{
    protected:
        std::vector<T> value_;
    public:
        RepeatedField() {}
        RepeatedField( const RepeatedField<T> &that ) { this->value_ = that.value_; }
        #if __cplusplus >= 201103L
        RepeatedField( RepeatedField<T> &&that ) { this->value_.swap(that.value_); }
        #endif
        void swap( RepeatedField<T> &that ) { traits< std::vector<T> >::swap(this->value_, that.value_); }
        const std::vector<T> &operator()() const { return value_; }
        std::vector<T> &operator()() { return value_; }
        bool undefined() const { return value_.size() == 0; }
        void clear() { value_.clear(); }
        RepeatedField<T> &operator=( const RepeatedField<T> &that ) { this->value_ = that.value_; return *this; }
        bool operator==( const RepeatedField<T> &that ) const { return this->value_ == that.value_; }
};

#ifdef PROTOGEN_CPP_ENABLE_PARENT

class Message
{
    public:
        virtual void serialize( std::string &out ) const = 0;
        virtual void serialize( std::vector<char> &out ) const = 0;
        virtual void serialize( std::ostream &out ) const = 0;
        virtual bool deserialize( std::istream &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL  ) = 0;
        virtual bool deserialize( const std::string &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL  ) = 0;
        virtual bool deserialize( const std::vector<char> &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL  ) = 0;
        virtual void clear() = 0;
        virtual bool undefined() const = 0;
};

#endif // PROTOGEN_CPP_ENABLE_PARENT

namespace json {

#ifdef PROTOGEN_OBFUSCATE_STRINGS

static std::string reveal( const char *value, size_t length )
{
	std::string result(length, ' ');
	for (size_t i = 0; i < length; ++i)
		result[i] = (char) ((int) value[i] ^ 0x33);
	return result;
}

#endif

// Write a complete JSON field
template<typename T>
static bool write( std::ostream &out, bool &first, const std::string &name, const T &value )
{
    if (!first) out << ',';
    out << '"' << name << "\":";
    traits<T>::write(out, value);
    first = false;
    return true;
}

template<typename I>
static bool next( InputStream<I> &in )
{
    in.skipws();
    int ch = in.get();
    if (ch == ',') return true;
    if (ch == '}' || ch == ']')
    {
        in.unget();
        return true;
    }
    return false;
}

template<typename I>
static bool readName( InputStream<I> &in, std::string &name )
{
    if (!traits<std::string>::read(in, name)) return false;
    in.skipws();
    return (in.get() == ':');
}

template<typename I>
static bool ignoreString( InputStream<I> &in )
{
    if (in.get() != '"') return false;

    while (!in.eof())
    {
        int ch = in.get();
        if (ch == '\\')
        {
            int ch = in.get();
            switch (ch)
            {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'r':
                case 'n':
                case 't':
                    break;
                default: // invalid escape sequence
                    return false;
            }
        }
        else
        if (ch == '"')
            return true;
    }

    return false;
}

template<typename I>
static bool ignoreEnclosing( InputStream<I> &in, int begin, int end )
{
    if (in.get() != begin) return false;

    int count = 1;

    bool text = false;
    while (count != 0 && !in.eof())
    {
        int ch = in.get();
        if (ch == '"' && !text)
        {
            in.unget();
            if (!ignoreString(in)) return false;
        }
        else
        if (ch == end)
            --count;
        else
        if (ch == begin)
            ++count;
    }

    return count == 0;
}

template<typename I>
static bool ignore( InputStream<I> &in )
{
    in.skipws();
    int ch = in.get();
    in.unget();
    if (ch == '[')
        return ignoreEnclosing(in, '[', ']');
    else
    if (ch == '{')
        return ignoreEnclosing(in, '{', '}');
    else
    if (ch == '"')
        return ignoreString(in);
    else
    {
        while (!in.eof())
        {
            int ch = in.get();
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == ',' || ch == ']' || ch == '}')
            {
                in.unget();
                return true;
            }
        }
    }

    return false;
}

} // namespace json
} // namespace PROTOGEN_NS

