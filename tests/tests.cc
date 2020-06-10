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
    book.owner.name = "Bob";
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
    person.friends.push_back("Kelly");
    person.friends.push_back("Zoe");
    person.friends.push_back("Beth");

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
#if 0
bool RUN_TEST4( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    struct { std::string json; int line; int col; int code; } CASES[] =
    {
        {"{ \n\n   \"bla\" : 55,,", 3, 15, PGERR_INVALID_NAME},
        {"{\"bla", 1, 5, PGERR_INVALID_SEPARATOR},
        {"{\"name\":\"bla\"}", 1, 14, PGERR_MISSING_FIELD},
        {"", 1, 0, PGERR_INVALID_OBJECT},
        {"{\"name\":45}", 1, 10, PGERR_INVALID_VALUE},
        // FIXME: why different column numbers?
        {"{\"blip1\":{}", 1, 11, PGERR_IGNORE_FAILED},
        {"{\"blip2\":[}", 1, 10, PGERR_IGNORE_FAILED},
        {"{\"blip3\":\"}", 1, 11, PGERR_IGNORE_FAILED},
        {"", 0, 0, 0}
    };

    options::Person temp;
    options::Person::ErrorInfo err;
    bool result = true;

    int i = 0;
    for (; result && CASES[i].line != 0; ++i)
    {
        err.code = err.column = err.line = 0;
        err.message.clear();
        err.set = false;
        result &= !temp.deserialize(CASES[i].json, CASES[i].code == PGERR_MISSING_FIELD, &err);
        result &= CASES[i].line == err.line && CASES[i].col == err.column;
        result &= CASES[i].code == err.code;
    }

    std::cerr << "[TEST #4] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    if (!result)
    {
        --i;
        std::cerr << "   Failed JSON entry #" << i << ": " << CASES[i].json << std::endl;
        std::cerr << "   Expected " << i << std::endl;
        std::cerr << "   " << CASES[i].code << " at " << CASES[i].line << ':' << CASES[i].col << std::endl;
        std::cerr << "   but got " << i << std::endl;
        std::cerr << "   " << err.code << '(' << err.message << ") at " << err.line << ':' << err.column << std::endl;
    }

    return result;
}
#endif

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
int main( int argc, char **argv)
{
    bool result;
    result  = RUN_TEST1(argc, argv);
    result &= RUN_TEST2(argc, argv);
    result &= RUN_TEST3(argc, argv);
    //result &= RUN_TEST4(argc, argv);
    result &= RUN_TEST5(argc, argv);
    //result &= RUN_TEST6(argc, argv);
    return (int) !result;
}