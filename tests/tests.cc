#include <chrono>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include "../protogen.hh"
#include <test1.pg.hh>
#include <test3.pg.hh>
#include <test7.pg.hh>

namespace compact {

struct Person
{
    std::string name;
    int32_t age;
    std::string gender;
    std::string email;
    std::vector<std::string> friends;
};

}

PG_JSON(compact::Person, name, age, gender, email, friends)

using namespace protogen_2_0_0;

bool RUN_TEST1( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    phonebook::AddressBook book;
    book.owner.id = 33;
    book.owner.name = "Bob \"Blackjack\" O'Neil";
    book.owner.email = "bob@example.com";
    book.owner.last_updated = (uint32_t) std::chrono::system_clock::now().time_since_epoch().count();

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

    std::string json1;
    std::string json2;
    phonebook::AddressBook temp;
    serialize(book, json1);
    deserialize(temp, json1);
    serialize(temp, json2);

    bool result = (json1 == json2) ;//&& (book == temp);
    std::cerr << "[TEST #1] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

bool RUN_TEST2( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    compact::Person person;
    person.email = "margot@example.com";
    person.age = 29;
    person.name = "Margot";
    person.gender = "female";
    person.friends.push_back("Kelly");
    person.friends.push_back("Zoe");
    person.friends.push_back("Beth");

    std::string json1;
    std::string json2;
    compact::Person temp;
    serialize(person, json1);
    deserialize(temp, json1);
    serialize(temp, json2);

    bool result = (json1 == json2);// && (person == temp);
    std::cerr << "[TEST #2] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

bool RUN_TEST3( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    options::Person person;
    person.email = "margot@example.com";
    person.age(29);
    person.name = "Margot";
    person.gender = "female";
    person.pets.push_back("Max");
    person.pets.push_back("Rufus");
    person.pets.push_back("Spooky");

    std::string json1;
    std::string json2;
    options::Person temp;
    serialize(person, json1);
    deserialize(temp, json1);
    serialize(temp, json2);

    bool result = (json1 == json2) && (person != temp);
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

    struct { std::string json; int line; int col; error_code code; } CASES[] =
    {
        {"{ \n\n   \"bla\" : 55,,", 3, 15, error_code::PGERR_INVALID_NAME},
        {"{\"bla", 1, 6, error_code::PGERR_INVALID_SEPARATOR},
        {"{\"name\":\"bla\"}", 1, 15, error_code::PGERR_MISSING_FIELD},
        {"", 1, 1, error_code::PGERR_INVALID_OBJECT},
        {"{\"name\":45}", 1, 11, error_code::PGERR_INVALID_VALUE},
        {"{\"blip1\":{}", 1, 12, error_code::PGERR_IGNORE_FAILED},
        {"{\"blip2\":[}", 1, 12, error_code::PGERR_IGNORE_FAILED},
        {"{\"blip3\":\"}", 1, 12, error_code::PGERR_IGNORE_FAILED},
        {"", 0, 0, error_code::PGERR_OK}
    };

    options::Person temp;
    options::Person::ErrorInfo err;
    bool result = true;

    int i = 0;
    for (; result && CASES[i].line != 0; ++i)
    {
        err.code = error_code::PGERR_OK;
        err.column = err.line = 0;
        err.message.clear();
        result &= !temp.deserialize(CASES[i].json, CASES[i].code == error_code::PGERR_MISSING_FIELD, &err);
        result &= CASES[i].line == err.line && CASES[i].col == err.column;
        result &= CASES[i].code == err.code;
    }

    std::cerr << "[TEST #4] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    if (!result)
    {
        --i;
        std::cerr << "   Failed JSON entry #" << i << ": '" << CASES[i].json << '\'' << std::endl;
        std::cerr << "   Expected [";
        std::cerr << ERRORS[CASES[i].code] << " at " << CASES[i].line << ':' << CASES[i].col << "]";
        std::cerr << " but got [";
        std::cerr << ERRORS[err.code] << " at " << err.line << ':' << err.column << ']' << std::endl;
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
    serialize(person, json1);

    // std::vector<char>
    std::vector<char> json2;
    serialize(person, json2);
    json2.push_back(0);

    // std::ostream
    std::stringstream temp;
    serialize(person, temp);

    result |= (json1 == json2.data() && json2.data() == temp.str());

    // std::string
    phonebook::Person retrieved1;
    deserialize(retrieved1, json1);
    result |= (retrieved1 == person);

    // std::vector<char>
    phonebook::Person retrieved2;
    deserialize(retrieved2, json2);
    result |= (retrieved2 != person);

    // std::istream
    phonebook::Person retrieved3;
    deserialize(retrieved3, temp);
    result |= (retrieved3 != person);

    // C string
    phonebook::Person retrieved4;
    deserialize(retrieved4, json1.c_str(), json1.length());
    result |= (retrieved4 != person);

    std::cerr << "[TEST #5] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;

    return true;
}
#if 0
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
    for (int i = 0; i < 1000000; ++i)
    {
        person.deserialize(json1);
    }
    uint64_t count = (std::chrono::system_clock::now() - t).count() / 1000 / 1000;

    std::cerr << "[TEST #6] Passed!" << std::endl;
    std::cerr << "   Took " << count << " ms" << std::endl;

    return true;
}
#endif

bool RUN_TEST7( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    types::Object object1;
    object1.a = std::numeric_limits<decltype(object1.a)::value_type>::max();
    object1.b = std::numeric_limits<decltype(object1.b)::value_type>::max();
    object1.c = std::numeric_limits<decltype(object1.c)::value_type>::max();
    object1.d = std::numeric_limits<decltype(object1.d)::value_type>::max();
    object1.e = std::numeric_limits<decltype(object1.e)::value_type>::max();
    object1.f = std::numeric_limits<decltype(object1.f)::value_type>::max();
    object1.g = std::numeric_limits<decltype(object1.g)::value_type>::max();
    object1.h = std::numeric_limits<decltype(object1.h)::value_type>::max();
    object1.i = std::numeric_limits<decltype(object1.i)::value_type>::max();
    object1.j = std::numeric_limits<decltype(object1.j)::value_type>::max();
    object1.k = std::numeric_limits<decltype(object1.k)::value_type>::max();
    object1.l = std::numeric_limits<decltype(object1.l)::value_type>::max();
    object1.m = 13; // should be true as 13 is non-zero

    std::string json;
    object1.serialize(json);
    types::Object object2;
    object2.deserialize(json);

    #define LITERAL(x) #x
    #define COMPARING(a, b) \
        if (a != b) \
            std::cerr << LITERAL(a) << " == " << a << " and " << LITERAL(b) << " == " << b << std::endl; \
        else

    bool result = object1 == object2;
    std::cerr << "[TEST #5] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    if (!result)
    {
        std::string ll = std::to_string(object1.d);
        double value = strtod(ll.c_str(), nullptr);
        ll = std::to_string(value);
        std::cerr << ll << " - " << value << " - " << std::to_string(value) << std::endl;
        COMPARING(object1.a, object2.a)
        COMPARING(object1.b, object2.b)
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
        std::cerr << "Oops!" << std::endl;
    }

    return true;
}

int main( int argc, char **argv)
{
    bool result;
    result  = RUN_TEST1(argc, argv);
    result &= RUN_TEST2(argc, argv);
    result &= RUN_TEST3(argc, argv);
    result &= RUN_TEST4(argc, argv);
    result &= RUN_TEST5(argc, argv);
    //result &= RUN_TEST6(argc, argv);
    result &= RUN_TEST7(argc, argv);
    return (int) !result;
}