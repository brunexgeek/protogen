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

bool RUN_TEST4( int argc, char **argv)
{
    (void) argc;
    (void) argv;

    struct { std::string json; int line; int col; int code; } CASES[] =
    {
        {"{ \n\n   \"bla\" : 55,,", 3, 15, PGERR_INVALID_NAME},
        {"{\"bla", 1, 6, PGERR_INVALID_SEPARATOR},
        {"{\"name\":\"bla\"}", 1, 14, PGERR_MISSING_FIELD},
        {"", 1, 1, PGERR_INVALID_OBJECT},
        {"{\"name\":45}", 1, 10, PGERR_INVALID_VALUE},
        // FIXME: why different column numbers?
        {"{\"blip1\":{}", 1, 11, PGERR_IGNORE_FAILED},
        {"{\"blip2\":[}", 1, 10, PGERR_IGNORE_FAILED},
        {"{\"blip3\":\"}", 1, 12, PGERR_IGNORE_FAILED},
        {"", 0, 0, 0}
    };

    options::Person temp;
    options::Person::ErrorInfo err;
    bool result = true;

    for (int i = 0; result && CASES[i].line != 0; ++i)
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
        std::cerr << "   " << err.message << " at " << err.line << ':' << err.column << std::endl;

    return result;
}

int main( int argc, char **argv)
{
    bool result;
    result  = RUN_TEST1(argc, argv);
    result &= RUN_TEST2(argc, argv);
    result &= RUN_TEST3(argc, argv);
    result &= RUN_TEST4(argc, argv);
    return (int) !result;
}