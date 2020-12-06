#include "TestRunner.h"

#include "Common/String.h"

// Should be SSO for x86 & x86-64
static const char* sso_string = "SSO St";

// Shouldn't be SSO
static const char* non_sso_string = "NON SSO String!!!";


TEST(Basic) {
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
    test.append("L");
    Assert::that(test.c_string()).is_equal("HEL");
    test.append("LO");
    Assert::that(test.c_string()).is_equal("HELLO");

    test << " WORLD " << kernel::format::as_hex << 0xDEADBEEF << kernel::format::as_dec << 123;

    Assert::that(test.c_string()).is_equal("HELLO WORLD 0xDEADBEEF123");
}

TEST(CopyOwnership) {
    kernel::String test = non_sso_string;
    kernel::String test1 = test;

    Assert::that(test1.c_string()).is_equal(test.c_string());
    Assert::that(test1.size()).is_equal(test.size());

    kernel::String test2 = sso_string;
    kernel::String test3 = test2;

    Assert::that(test3.c_string()).is_equal(test2.c_string());
    Assert::that(test3.size()).is_equal(test2.size());
}

TEST(MoveOwnership) {
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

TEST(PopBackSSO) {
    kernel::String sso_test = "sso";

    sso_test.pop_back();
    Assert::that(sso_test.c_string()).is_equal("ss");
    Assert::that(sso_test.size()).is_equal(2);

    sso_test.pop_back();
    Assert::that(sso_test.c_string()).is_equal("s");
    Assert::that(sso_test.size()).is_equal(1);

    sso_test.pop_back();
    Assert::that(sso_test.c_string()).is_equal("");
    Assert::that(sso_test.size()).is_equal(0);

    sso_test.pop_back();
    Assert::that(sso_test.c_string()).is_equal("");
    Assert::that(sso_test.size()).is_equal(0);
}

TEST(PopBackNonSSO) {
    kernel::String non_sso_test = "non-sso-string-that-is-long";
    size_t initial_length = non_sso_test.size();

    non_sso_test.pop_back();
    Assert::that(non_sso_test.c_string()).is_equal("non-sso-string-that-is-lon");
    Assert::that(non_sso_test.size()).is_equal(initial_length - 1);

    for (size_t i = 0; i < initial_length - 4; ++i) {
        non_sso_test.pop_back();
    }

    Assert::that(non_sso_test.c_string()).is_equal("non");
    Assert::that(non_sso_test.size()).is_equal(3);
}

TEST(StackStringBuilder) {
    kernel::StackStringBuilder builder;
    builder += 123;
    builder += "456";
    builder << kernel::format::as_hex << 0xDEADBEEF;
    builder.append("test");
    builder.append(true);
    builder.append('!');

    Assert::that(const_cast<const char*>(builder.data())).is_equal("1234560xDEADBEEFtesttrue!");
}

TEST(StackStringBuilderLeftShift) {
    kernel::StackStringBuilder builder;

    int* my_pointer = reinterpret_cast<int*>(static_cast<decltype(sizeof(int))>(0xDEADBEEF));

    builder << 123 << "456" << my_pointer << "test" << true << '!';

    const char* expected_string = nullptr;

    if constexpr (sizeof(void*) == 8) {
        expected_string = "1234560x00000000DEADBEEFtesttrue!";
    } else {
        expected_string = "1234560xDEADBEEFtesttrue!";
    }

    Assert::that(const_cast<const char*>(builder.data())).is_equal(expected_string);
}
