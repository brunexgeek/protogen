#ifndef PROTOGEN_CPP_ENABLE_ERRORS
    #define PROTOGEN_REV(err,input,name,type) return false
    #define PROTOGEN_REI(err,input,name) return false
    #define PROTOGEN_REF(err,input,name) return false
    #define PROTOGEN_REM(err,input,name) return false
    #define PROTOGEN_REG(err,input,name) return false
#else
    #define PROTOGEN_REV(err,input,name,type) \
        do { if (err == NULL || err->set) return false; \
             err->set = true; \
             err->line = input.line(); err->column = input.column(); \
             err->message = std::string("Invalid '") + type + "' value in field '" + name + "'"; \
             return false; } while(false)
    #define PROTOGEN_REI(err,input,name) \
        do { if (err == NULL || err->set) return false; \
             err->set = true; \
             err->line = input.line(); err->column = input.column(); \
             err->message = std::string("Unable to skip field '") + name + "'"; \
             return false; } while(false)
    #define PROTOGEN_RETERR_FORMAT(err,input,name) \
        do { if (err == NULL || err->set) return false; \
             err->set = true; \
             err->line = input.line(); err->column = input.column(); \
             err->message = std::string("Invalid JSON after field '") + name + "'"; \
             return false; } while(false)
    #define PROTOGEN_REM(err,input,name) \
        do { if (err == NULL || err->set) return false; \
             err->set = true; \
             err->line = input.line(); err->column = input.column(); \
             err->message = std::string("Missing required field '") + name + "'"; \
             return false; } while(false)
    #define PROTOGEN_REG(err,input,msg) \
        do { if (err == NULL || err->set) return false; \
             err->set = true; \
             err->line = input.line(); err->column = input.column(); \
             err->message = msg; \
             return false; } while(false)
#endif

#undef PROTOGEN_TRAIT_MACRO
#define PROTOGEN_TRAIT_MACRO(MSGTYPE) \
	namespace PROTOGEN_NS { \
		template<> struct traits<MSGTYPE> { \
			static void clear( MSGTYPE &value ) { value.clear(); } \
			static void write( std::ostream &out, const MSGTYPE &value ) { value.serialize(out); } \
			template<typename I> static bool read( PROTOGEN_NS::InputStream<I> &in, MSGTYPE &value, \
                bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ) { return value.deserialize(in, required, err); } \
			static void swap( MSGTYPE &a, MSGTYPE &b ) { a.swap(b); } \
		}; \
	}

#undef PROTOGEN_FIELD_MOVECTOR_TEMPLATE
#if __cplusplus >= 201103L
#define PROTOGEN_FIELD_MOVECTOR_TEMPLATE(MSGTYPE) Field( Field<MSGTYPE> &&that ) { this->value_.swap(that.value_); }
#else
#define PROTOGEN_FIELD_MOVECTOR_TEMPLATE(MSGTYPE)
#endif

#undef PROTOGEN_FIELD_TEMPLATE
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
			Field<MSGTYPE> &operator=( const MSGTYPE &that ) { this->value_ = that; return *this; } \
			bool operator==( const Field<MSGTYPE> &that ) const { return this->value_ == that.value_; } \
			bool operator==( const MSGTYPE &that ) const { return this->value_ == that; } \
			explicit operator MSGTYPE() { return this->value_; } \
			explicit operator MSGTYPE() const { return this->value_; } \
	    }; \
    }