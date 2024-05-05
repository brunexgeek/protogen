#include <chrono>
#include <string>
#include <cstring>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>
#include <test1.pg.hh>
#include <test3.pg.hh>
#include <test7.pg.hh>

using namespace std::chrono;

using namespace protogen_3_0_0;

bool RUN_TEST1( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    phonebook::AddressBook book;
    //book.owner.id = 33;
    book.owner.name = "이주영";
    book.owner.email = "ljy@example.com";
    book.owner.last_updated = (uint32_t) duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    phonebook::Person person;
    person.email = "test@example.com";
    person.id = 1234;
    person.name = "Michelle";

    phonebook::PhoneNumber number;
    number.number = "+55 33 995-3636-1111";
    number.type = true;
    person.phones.push_back(number);

    number.number = "+38 10 105-9482-3057";
    number.type = false;
    person.phones.push_back(number);

    book.people.push_back(person);

    Parameters params;
    params.serialize_null = true;

    std::string json1;
    std::string json2;
    phonebook::AddressBook temp;
    book.serialize(json1, &params);
    temp.deserialize(json1);
    temp.serialize(json2, &params);

    bool result = (json1 == json2) ;//&& (book == temp);
    std::cerr << "[TEST #1] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

bool RUN_TEST2( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    phonebook::AddressBook book;
    //book.owner.id = 33;
    book.owner.name = "이주영";
    book.owner.email = "ljy@example.com";
    book.owner.last_updated = (uint32_t) duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    Parameters params;
    params.serialize_null = true;
    params.ensure_ascii = true;

    std::string json1;
    std::string json2;
    phonebook::AddressBook temp;
    book.serialize(json1, &params);
    temp.deserialize(json1);
    temp.serialize(json2, &params);

    bool result = (json1 == json2) ;//&& (book == temp);
    std::cerr << "[TEST #2] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

bool RUN_TEST3( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    options::Cake cake;
    cake.name = "Strawberry Cake";
    cake.weight = 29;
    cake.flavor = "strawberry";
    cake.ingredients.push_back("milk");
    cake.ingredients.push_back("strawberry");
    cake.ingredients.push_back("flour");

    std::string json1;
    std::string json2;
    options::Cake temp;
    cake.serialize(json1);
    temp.deserialize(json1);
    temp.serialize(json2);

    bool result = (json1 == json2) && (cake != temp) && temp.flavor.empty();
    std::cerr << "[TEST #3] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

const char *ERRORS[] =
{
    "PGERR_OK",
    "PGERR_IGNORE_FAILED",
    "PGERR_MISSING_FIELD",
    "PGERR_INVALID_SEPARATOR",
    "PGERR_INVALID_VALUE",
    "PGERR_INVALID_OBJECT",
    "PGERR_INVALID_NAME",
};

bool RUN_TEST4( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    struct { int line; int col; error_code code; std::string json; } CASES[] =
    {
        {3, 15, error_code::PGERR_INVALID_NAME,      "{ \n\n   \"bla\" : 55,," },
        {1,  2, error_code::PGERR_INVALID_NAME,      "{\"bla" },
        {1,  7, error_code::PGERR_INVALID_SEPARATOR, "{\"bla\"" },
        {1,  1, error_code::PGERR_INVALID_OBJECT,    "" },
        {1,  6, error_code::PGERR_INVALID_VALUE,     "{\"1\":45}" },
        {1, 12, error_code::PGERR_INVALID_OBJECT,    "{\"blip1\":{}" },
        {1, 11, error_code::PGERR_IGNORE_FAILED,     "{\"blip2\":[}" },
        {1, 10, error_code::PGERR_IGNORE_FAILED,     "{\"blip3\":\"}" },
        {0,  0, error_code::PGERR_OK,                "{\"name\":null}" },
        {0,  0, error_code::PGERR_INVALID_VALUE,     "{\"name\":\"\\uFFFF\"}" },
        {0,  0, error_code::PGERR_INVALID_VALUE,     "{\"name\":\"\\u01\"}" },
        {0,  0, error_code::PGERR_OK,                ""}
    };

    options::Cake temp;
    Parameters params;
    bool result = true;

    int i = 0;
    for (; result && CASES[i].line != 0; ++i)
    {
        result &= !temp.deserialize(CASES[i].json, &params);
        result &= CASES[i].line == params.error.line && CASES[i].col == params.error.column;
        result &= CASES[i].code == params.error.code;
    }

    std::cerr << "[TEST #4] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    if (!result)
    {
        --i;
        std::cerr << "   Failed JSON entry #" << i << ": '" << CASES[i].json << '\'' << std::endl;
        std::cerr << "   Expected [";
        std::cerr << ERRORS[CASES[i].code] << " at " << CASES[i].line << ':' << CASES[i].col << "]";
        std::cerr << " but got [";
        std::cerr << ERRORS[params.error.code] << " at " << params.error.line << ':' << params.error.column << ']' << std::endl;
    }

    return result;
}


bool RUN_TEST5( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    bool result = false;
    phonebook::Person person;
    person.email = "test@example.com";
    person.id = 1234;
    person.name = "Michelle";

    // std::string
    std::string json1;
    person.serialize(json1);

    // std::vector<char>
    std::vector<char> json2;
    person.serialize(json2);
    json2.push_back(0);

    // std::ostream
    std::stringstream temp;
    person.serialize(temp);

    result |= (json1 == json2.data() && json2.data() == temp.str());

    // std::string
    phonebook::Person retrieved1;
    retrieved1.deserialize(json1);
    result |= (retrieved1 == person);

    // std::vector<char>
    phonebook::Person retrieved2;
    retrieved2.deserialize(json2);
    result |= (retrieved2 != person);

    // std::istream
    phonebook::Person retrieved3;
    retrieved3.deserialize(temp);
    result |= (retrieved3 != person);

    // C string
    phonebook::Person retrieved4;
    retrieved4.deserialize(json1.c_str(), json1.length());
    result |= (retrieved4 != person);

    std::cerr << "[TEST #5] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;

    return true;
}

bool RUN_TEST6( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    phonebook::Person person;
    person.email = "test@example.com";
    person.id = 1234;
    person.name = "Michelle";

    phonebook::PhoneNumber number;
    number.number = "+55 33 995-3636-1111";
    number.type = true;
    person.phones.push_back(number);

    number.clear();
    number.number = "+38 10 105-9482-3057";
    number.type = false;
    person.phones.push_back(number);

    std::string json1;
    person.serialize(json1);

    auto t = std::chrono::system_clock::now();
    for (int i = 0; i < 100000; ++i)
    {
        person.deserialize(json1);
    }
    uint64_t count = (std::chrono::system_clock::now() - t).count() / 1000;

    std::cerr << "[TEST #6] Passed!" << std::endl;
    std::cerr << "   " << json1 << std::endl;
    std::cerr << "   Took " << count << " us" << std::endl;

    return true;
}

bool RUN_TEST7A( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    types::Basic object1;
    object1.a = std::numeric_limits<decltype(object1.a)>::max();
    object1.b = std::numeric_limits<decltype(object1.b)>::max();
    object1.c = std::numeric_limits<decltype(object1.c)>::max();
    object1.d = std::numeric_limits<decltype(object1.d)>::max();
    object1.e = std::numeric_limits<decltype(object1.e)>::max();
    object1.f = std::numeric_limits<decltype(object1.f)>::max();
    object1.g = std::numeric_limits<decltype(object1.g)>::max();
    object1.h = std::numeric_limits<decltype(object1.h)>::max();
    object1.i = std::numeric_limits<decltype(object1.i)>::max();
    object1.j = std::numeric_limits<decltype(object1.j)>::max();
    object1.k = std::numeric_limits<decltype(object1.k)>::max();
    object1.l = std::numeric_limits<decltype(object1.l)>::max();
    object1.m = 13; // should be true as 13 is non-zero

    std::string json;
    object1.serialize(json);
    types::Basic object2;
    object2.deserialize(json);

    #define LITERAL(x) #x
    #define COMPARING(a, b) \
        if (a != b) { \
            result = false; \
            output += std::string(LITERAL(a)) + " == " + std::to_string(a) + " and " + LITERAL(b) + " == " + std::to_string(b); } \
        else

    bool result = true;
    std::string output = "   ";
    COMPARING(object1.a, object2.a) // comparing FP with ==
    COMPARING(object1.b, object2.b) // comparing FP with ==
    COMPARING(object1.c, object2.c)
    COMPARING(object1.d, object2.d)
    COMPARING(object1.e, object2.e)
    COMPARING(object1.f, object2.f)
    COMPARING(object1.g, object2.g)
    COMPARING(object1.h, object2.h)
    COMPARING(object1.i, object2.i)
    COMPARING(object1.j, object2.j)
    COMPARING(object1.k, object2.k)
    COMPARING(object1.l, object2.l)
    COMPARING(object1.m, object2.m)
    output.clear();

    std::cerr << "[TEST #7A] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    if (!result)
        std::cerr << output << std::endl;

    return true;
}

bool RUN_TEST7B( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    types::Container object1;
    object1.a.push_back(10);
    object1.b.push_back(10);
    object1.c.push_back(10);
    object1.d.push_back(10);
    object1.e.push_back(10);
    object1.f.push_back(10);
    object1.g.push_back(10);
    object1.h.push_back(10);
    object1.i.push_back(10);
    object1.j.push_back(10);
    object1.k.push_back(10);
    object1.l.push_back(10);
    object1.m.push_back(false);
    object1.n.push_back("test");
    object1.o.push_back(10);

    std::string json1;
    object1.serialize(json1);
    types::Container object2;
    object2.deserialize(json1);
    std::string json2;
    object2.serialize(json2);

    bool result = json1 == json2;
    result &= object1 == object2;
    std::cerr << "[TEST #7B] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    if (!result)
    {
        std::cerr << "   " << json1 << std::endl;
        std::cerr << "   " << json2 << std::endl;
    }

    return true;
}

static bool test8_iteration( const char **values, bool ensure_ascii = false )
{
    phonebook::AddressBook book;
    Parameters params;
    params.ensure_ascii = ensure_ascii;

    for (int i = 0; values[i] != nullptr; ++i)
    {
        book.owner.name = values[i];

        std::string json1;
        book.serialize(json1, &params);
        book.clear();

        bool result = book.deserialize(json1, &params);
        std::string json2;
        Parameters params2 = params;
        book.serialize(json2, &params2);

        result = result && json1 == json2 && book.owner.name == values[i];
        if (!result)
        {
            std::cerr << "[TEST #8] Failed!" << std::endl;
            if (params.error.code != PGERR_OK)
                std::cerr << ERRORS[params.error.code] << " at " << params.error.line << ':' << params.error.column << ']' << std::endl;
            std::cerr << "   " << json1 << " -> " << values[i] << '\n';
            std::cerr << "   " << json2 << " -> " << *book.owner.name << '\n';
            std::cerr << "   '" << *book.owner.name << "' == '" << values[i] << "'  ->" << (book.owner.name == values[i] ? "true" : "false") << '\n';
            return false;
        }
    }
    return true;
}

bool RUN_TEST8( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    static const char *VALUES[] = {
        "a", // 1-byte UTF-8
        "ß", // 2-bytes UTF-8
        "이", // 3-bytes UTF-8
        "𑙤", // 4-bytes UTF-8
        "이주영𑙤Hello",
        nullptr
    };

    if (!test8_iteration(VALUES, false))
        return false;
    if (!test8_iteration(VALUES, true))
        return false;

    std::cerr << "[TEST #8] Passed!" << std::endl;
    return true;
}

bool RUN_TEST9( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    std::list<phonebook::Person> people;
    for (int i = 0; i < 5; ++i)
    {
        phonebook::Person person;
        person.email = "test@example.com";
        person.id = 1234 + i;
        person.name = "Person " + std::to_string(i);
        people.push_back(std::move(person));
    }

    std::string json1;
    protogen_3_0_0::serialize_array(people, json1);

    people.clear();
    auto result = protogen_3_0_0::deserialize_array(people, json1);
    std::string json2;
    protogen_3_0_0::serialize_array(people, json2);

    if (!result || json1 != json2)
    {
        std::cerr << "[TEST #9] Failed!" << std::endl;
        std::cerr << "   " << json1 << '\n';
        std::cerr << "   " << json2 << '\n';
        return false;
    }
    else
    {
        std::cerr << "[TEST #9] Passed!" << std::endl;
        std::cerr << "   " << json1 << '\n';
        return true;
    }
}

bool RUN_TEST10( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    std::list<phonebook::Person> people;

    std::string json1;
    std::vector<char> json2;

    if (!protogen_3_0_0::serialize_array(people, json1))
        goto FAILED;
    //if (!protogen_3_0_0::deserialize_array(people, json1.c_str(), json1.length()))
    //    goto FAILED;
    if (!protogen_3_0_0::deserialize_array(people, json1))
        goto FAILED;

    if (!protogen_3_0_0::serialize_array(people, json2))
        goto FAILED;
    if (!protogen_3_0_0::deserialize_array(people, json2))
        goto FAILED;

    std::cerr << "[TEST #10] Passed!" << std::endl;
    return true;

    FAILED:
    std::cerr << "[TEST #10] Failed!" << std::endl;
    return false;
}

int main( int argc, char **argv)
{
    bool result = true;
    result &= RUN_TEST1(argc, argv);
    result &= RUN_TEST2(argc, argv);
    result &= RUN_TEST3(argc, argv);
    result &= RUN_TEST4(argc, argv);
    result &= RUN_TEST5(argc, argv);
    result &= RUN_TEST6(argc, argv);
    result &= RUN_TEST7A(argc, argv);
    result &= RUN_TEST7B(argc, argv);
    result &= RUN_TEST8(argc, argv);
    result &= RUN_TEST9(argc, argv);
    result &= RUN_TEST10(argc, argv);
    return (int) !result;
}
