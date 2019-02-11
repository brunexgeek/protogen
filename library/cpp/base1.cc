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