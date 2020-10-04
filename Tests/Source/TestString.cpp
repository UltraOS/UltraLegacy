#include "TestRunner.h"

#include "Common/String.h"

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
    kernel::String test1 = move(test);

    Assert::that(test.c_string()).is_equal("");
    Assert::that(test.size()).is_equal(0);

    Assert::that(test1.c_string()).is_equal(non_sso_string);
    Assert::that(test1.size()).is_equal(kernel::String::length_of(non_sso_string));

    kernel::String test2 = sso_string;
    kernel::String test3 = move(test2);

    Assert::that(test2.c_string()).is_equal("");
    Assert::that(test2.size()).is_equal(0);

    Assert::that(test3.c_string()).is_equal(sso_string);
    Assert::that(test3.size()).is_equal(kernel::String::length_of(sso_string));
}

TEST(StackStringBuilder) {
    kernel::StackStringBuilder builder;
    builder += 123;
    builder += "456";
    builder.append_hex(0xDEADBEEF);
    builder.append("test");
    builder.append(true);
    builder.append('!');

    Assert::that(const_cast<const char*>(builder.data())).is_equal("1234560xDEADBEEFtesttrue!");
}

TEST(StackStringBuilderLeftShift) {
    kernel::StackStringBuilder builder;

    int* my_pointer = reinterpret_cast<int*>(static_cast<decltype(sizeof(int))>(0xDEADBEEF));

    builder << 123 << "456" << my_pointer << "test" << true << '!';

    // TODO: deadbeef is hardcoded as the 8 byte representation here, make it dynamic?
    Assert::that(const_cast<const char*>(builder.data())).is_equal("1234560x00000000DEADBEEFtesttrue!");
}
