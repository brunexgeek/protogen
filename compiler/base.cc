#ifndef PROTOGEN_TEMPLATES
#define PROTOGEN_TEMPLATES

namespace protogen {

template<typename T> class Field
{
    protected:
        T value;
        uint32_t flags;
    public:
        Field() : flags(0) {}
        virtual ~Field() {}
        operator const T&() const { return value; }
        const T &operator()() const { return value; }
        void operator ()(const T &value ) { this->value = value; flags &= ~1; }
        Field<T> &operator=( const T &value ) { this->value = value; flags &= ~1; return *this; }
        bool undefined() const { return flags & 1; }
        virtual void clear() { flags |= 1; }
};

}

#endif // #ifndef PROTOGEN_TEMPLATES