#include <test1.pg.hh>
#include <test2.pg.hh>
#include <test3.pg.hh>

bool RUN_TEST1( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    phonebook::Person person;
    person.email("test@example.com");
    person.id(1234);
    person.name("Michelle");

    phonebook::PhoneNumber number;
    number.number("+55 33 995-3636-1111");
    number.type(true);
    person.phones->push_back(number);

    number.clear();
    number.number("+38 10 105-9482-3057");
    number.type(false);
    person.phones->push_back(number);

    std::string json1;
    std::string json2;
    phonebook::Person temp;
    person.serialize(json1);
    temp.deserialize(json1);
    temp.serialize(json2);

    bool result = (json1 == json2) && (person == temp);
    std::cerr << "[TEST #1] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

bool RUN_TEST2( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    compact::Person person;
    person.email("margot@example.com");
    person.age(29);
    person.name("Margot");
    person.gender("female");
    person.friends->push_back("Kelly");
    person.friends->push_back("Zoe");
    person.friends->push_back("Beth");

    std::string json1;
    std::string json2;
    compact::Person temp;
    person.serialize(json1);
    temp.deserialize(json1);
    temp.serialize(json2);

    bool result = (json1 == json2) && (person == temp);
    std::cerr << "[TEST #2] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

bool RUN_TEST3( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    options::Person person;
    person.email("margot@example.com");
    person.age(29);
    person.name("Margot");
    person.gender("female");
    person.friends->push_back("Kelly");
    person.friends->push_back("Zoe");
    person.friends->push_back("Beth");

    std::string json1;
    std::string json2;
    options::Person temp;
    person.serialize(json1);
    temp.deserialize(json1);
    temp.serialize(json2);

    bool result = (json1 == json2) && (person != temp);
    std::cerr << "[TEST #3] " << ((result) ? "Passed!" : "Failed!" ) << std::endl;
    std::cerr << "   " << json1 << std::endl << "   " << json2 << std::endl;

    return result;
}

int main( int argc, char **argv)
{
    bool result;
    result  = RUN_TEST1(argc, argv);
    result &= RUN_TEST2(argc, argv);
    result &= RUN_TEST3(argc, argv);
    return (int) !result;
}