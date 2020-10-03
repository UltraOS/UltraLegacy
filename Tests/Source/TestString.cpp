#include "Common/String.h"

#include "TestRunner.h"

// Should be SSO for x86 & x86-64
static const char* sso_string = "SSO St";

// Shouldn't be SSO
static const char* non_sso_string = "NON SSO String!!!";


TEST(BasicString) {
    kernel::String test;

    test = sso_string;

    Assert::that(test.c_string()).is_equal(sso_string);

    test = non_sso_string;

    Assert::that(test.c_string()).is_equal(non_sso_string);
}

TEST(Append) {
    kernel::String test;

    Assert::that(test.c_string()).is_equal("");

    test.append('H');
    Assert::that(test.c_string()).is_equal("H");
    test.append("E");
    Assert::that(test.c_string()).is_equal("HE");
    test.append("L", 1);
    Assert::that(test.c_string()).is_equal("HEL");
    test.append("LO");
    Assert::that(test.c_string()).is_equal("HELLO");
    test.append(" WORLD, NON SSO!!");
    Assert::that(test.c_string()).is_equal("HELLO WORLD, NON SSO!!");
}

TEST(StringCopyOwnership) {
    kernel::String test = non_sso_string;
    kernel::String test1 = test;

    Assert::that(test1.c_string()).is_equal(test.c_string());
    Assert::that(test1.size()).is_equal(test.size());

    kernel::String test2 = sso_string;
    kernel::String test3 = test2;

    Assert::that(test3.c_string()).is_equal(test2.c_string());
    Assert::that(test3.size()).is_equal(test2.size());
}

TEST(StringMoveOwnership) {
    kernel::String test = non_sso_string;
    kernel::String test1 = kernel::move(test);

    Assert::that(test.c_string()).is_equal("");
    Assert::that(test.size()).is_equal(0);

    Assert::that(test1.c_string()).is_equal(non_sso_string);
    Assert::that(test1.size()).is_equal(kernel::String::length_of(non_sso_string));

    kernel::String test2 = sso_string;
    kernel::String test3 = kernel::move(test2);

    Assert::that(test2.c_string()).is_equal("");
    Assert::that(test2.size()).is_equal(0);

    Assert::that(test3.c_string()).is_equal(sso_string);
    Assert::that(test3.size()).is_equal(kernel::String::length_of(sso_string));
}

TEST(StackStringBuilder) {
    kernel::StackStringBuilder builder;
    builder += 123;
    builder += "321";
    builder.append_hex(0xDEADBEEF);
    builder.append("test");
    builder.seal();

    std::string string(builder.as_view().data());
    Assert::that(string).is_equal("1233210xDEADBEEFtest");
}
