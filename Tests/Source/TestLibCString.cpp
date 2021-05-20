#include "TestRunner.h"

namespace libc {

#include "LibC/string.h"
#include "LibC/string.c"
#include "LibC/strings.h"
#include "LibC/strings.c"

}

TEST(Strcpy) {
    char str[3];
    memset(str, 0xFF, 3);

    const char* my_str = "X";
    libc::strcpy(str, my_str);

    Assert::that(str[0]).is_equal('X');
    Assert::that(str[1]).is_equal('\0');
    Assert::that(str[2]).is_equal(0xFF);

    memset(str, 0xFF, 3);

    const char* my_str_1 = "";
    libc::strcpy(str, my_str_1);
    Assert::that(str[0]).is_equal('\0');
    Assert::that(str[1]).is_equal(0xFF);
    Assert::that(str[2]).is_equal(0xFF);
}

TEST(Strncpy) {
    char str[4];
    memset(str, 0xFF, 4);

    const char* my_str = "X";
    libc::strncpy(str, my_str, 3);

    Assert::that(str[0]).is_equal('X');
    Assert::that(str[1]).is_equal('\0');
    Assert::that(str[2]).is_equal('\0');
    Assert::that(str[3]).is_equal(0xFF);

    memset(str, 0xFF, 4);

    const char* my_str_1 = "";
    libc::strncpy(str, my_str_1, 3);
    Assert::that(str[0]).is_equal('\0');
    Assert::that(str[1]).is_equal('\0');
    Assert::that(str[2]).is_equal('\0');
    Assert::that(str[3]).is_equal(0xFF);

    memset(str, 0xFF, 4);

    const char* my_str_2 = "HELLO";
    libc::strncpy(str, my_str_2, 3);
    Assert::that(str[0]).is_equal('H');
    Assert::that(str[1]).is_equal('E');
    Assert::that(str[2]).is_equal('L');
    Assert::that(str[3]).is_equal(0xFF);
}

TEST(Strcat) {
    const char* my_str = "456";

    char str[7] = "123";
    str[6] = 0xFF;

    libc::strcat(str, my_str);
    Assert::that(std::string(str)).is_equal("123456");
}

TEST(Strncat) {
    const char* my_str = "4567";

    char str[3 + 3 + 2] = "123";
    str[6] = 0xFF;
    str[7] = 0xFF;

    libc::strncat(str, my_str, 3);
    Assert::that(std::string(str)).is_equal("123456");
    Assert::that(str[7]).is_equal(0xFF);

    char str1[3 + 4 + 2] = "123";
    str1[7] = 0xFF;
    str1[8] = 0xFF;

    libc::strncat(str1, my_str, 5);
    Assert::that(std::string(str1)).is_equal("1234567");
    Assert::that(str1[7]).is_equal('\0');
    Assert::that(str1[8]).is_equal(0xFF);
}

TEST(Strxfrm) {
    const char* my_str = "123";

    Assert::that(libc::strxfrm(nullptr, my_str, 0)).is_equal(3);
    Assert::that(libc::strxfrm(nullptr, "", 0)).is_equal(0);

    char my_buf[10] = {};
    my_buf[5] = 0xFF;
    libc::strxfrm(my_buf, my_str, 9);
    Assert::that(std::string(my_buf)).is_equal(my_str);
    Assert::that(my_buf[4]).is_equal('\0');
    Assert::that(my_buf[5]).is_equal(0xFF);

    memset(my_buf, 0xFF, 10);
    libc::strxfrm(my_buf, my_str, 1);
    Assert::that(std::string(my_buf)).is_equal("1");
    Assert::that(my_buf[2]).is_equal(0xFF);
}

TEST(Strlen) {
    Assert::that(libc::strlen("")).is_equal(0);
    Assert::that(libc::strlen("123")).is_equal(3);
}

TEST(Strcmp) {
    Assert::that(libc::strcmp("a", "b")).is_equal(-1);
    Assert::that(libc::strcmp("b", "a")).is_equal(1);
    Assert::that(libc::strcmp("a", "a")).is_equal(0);
    Assert::that(libc::strcmp("", "")).is_equal(0);
    Assert::that(libc::strcmp("asd", "")).is_equal('a');
    Assert::that(libc::strcmp("", "asd")).is_equal(-'a');
    Assert::that(libc::strcmp("asdasd", "asd")).is_equal('a');
}

TEST(Strncmp) {
    Assert::that(libc::strncmp("a", "b", 99)).is_equal(-1);
    Assert::that(libc::strncmp("b", "a", 99)).is_equal(1);
    Assert::that(libc::strncmp("a", "a", 99)).is_equal(0);
    Assert::that(libc::strncmp("", "", 99)).is_equal(0);
    Assert::that(libc::strncmp("asd", "", 99)).is_equal('a');
    Assert::that(libc::strncmp("", "asd", 99)).is_equal(-'a');
    Assert::that(libc::strncmp("asdasd", "asd", 3)).is_equal(0);
    Assert::that(libc::strncmp("asdasd", "asd", 6)).is_equal('a');
}

TEST(Strchr) {
    char my_str[] = "asd";

    Assert::that(libc::strchr(my_str, 'a')).is_equal(my_str);
    Assert::that(libc::strchr(my_str, 's')).is_equal(my_str + 1);
    Assert::that(libc::strchr(my_str, 'd')).is_equal(my_str + 2);
    Assert::that(libc::strchr(my_str, '\0')).is_equal(my_str + 3);
    Assert::that(libc::strchr(my_str, 'X')).is_equal(nullptr);

    char my_str1[] = "";
    Assert::that(libc::strchr(my_str1, '\0')).is_equal(my_str1);
    Assert::that(libc::strchr(my_str1, 'a')).is_equal(nullptr);
}

TEST(Strrchr) {
    char my_str[] = "asda";

    Assert::that(libc::strrchr(my_str, 'a')).is_equal(my_str + 3);
    Assert::that(libc::strrchr(my_str, 's')).is_equal(my_str + 1);
    Assert::that(libc::strrchr(my_str, 'd')).is_equal(my_str + 2);
    Assert::that(libc::strrchr(my_str, '\0')).is_equal(my_str + 4);
    Assert::that(libc::strrchr(my_str, 'X')).is_equal(nullptr);

    char my_str1[] = "";
    Assert::that(libc::strrchr(my_str1, '\0')).is_equal(my_str1);
    Assert::that(libc::strrchr(my_str1, 'a')).is_equal(nullptr);
}
