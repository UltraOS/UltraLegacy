#include "TestRunner.h"

namespace libc {

#include "LibC/string.h"
#include "LibC/string.c"
#include "LibC/strings.h"
#include "LibC/strings.c"

}

TEST(Strcpy) {
    char str[3];
    libc::memset(str, 0xFF, 3);

    const char* my_str = "X";
    libc::strcpy(str, my_str);

    Assert::that(str[0]).is_equal('X');
    Assert::that(str[1]).is_equal('\0');
    Assert::that(str[2]).is_equal(0xFF);

    libc::memset(str, 0xFF, 3);

    const char* my_str_1 = "";
    libc::strcpy(str, my_str_1);
    Assert::that(str[0]).is_equal('\0');
    Assert::that(str[1]).is_equal(0xFF);
    Assert::that(str[2]).is_equal(0xFF);
}

TEST(Strncpy) {
    char str[4];
    libc::memset(str, 0xFF, 4);

    const char* my_str = "X";
    libc::strncpy(str, my_str, 3);

    Assert::that(str[0]).is_equal('X');
    Assert::that(str[1]).is_equal('\0');
    Assert::that(str[2]).is_equal('\0');
    Assert::that(str[3]).is_equal(0xFF);

    libc::memset(str, 0xFF, 4);

    const char* my_str_1 = "";
    libc::strncpy(str, my_str_1, 3);
    Assert::that(str[0]).is_equal('\0');
    Assert::that(str[1]).is_equal('\0');
    Assert::that(str[2]).is_equal('\0');
    Assert::that(str[3]).is_equal(0xFF);

    libc::memset(str, 0xFF, 4);

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

    libc::memset(my_buf, 0xFF, 10);
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
    Assert::that(libc::strchr(my_str, 'X')).is_null();

    char my_str1[] = "";
    Assert::that(libc::strchr(my_str1, '\0')).is_equal(my_str1);
    Assert::that(libc::strchr(my_str1, 'a')).is_null();
}

TEST(Strrchr) {
    char my_str[] = "asda";

    Assert::that(libc::strrchr(my_str, 'a')).is_equal(my_str + 3);
    Assert::that(libc::strrchr(my_str, 's')).is_equal(my_str + 1);
    Assert::that(libc::strrchr(my_str, 'd')).is_equal(my_str + 2);
    Assert::that(libc::strrchr(my_str, '\0')).is_equal(my_str + 4);
    Assert::that(libc::strrchr(my_str, 'X')).is_null();

    char my_str1[] = "";
    Assert::that(libc::strrchr(my_str1, '\0')).is_equal(my_str1);
    Assert::that(libc::strrchr(my_str1, 'a')).is_null();
}

TEST(Strspn) {
    const char* my_str = "aaaaaa11122231233";

    Assert::that(libc::strspn(my_str, "a123")).is_equal(libc::strlen(my_str));
    Assert::that(libc::strspn("", "")).is_equal(0);
    Assert::that(libc::strspn("", "asd")).is_equal(0);
    Assert::that(libc::strspn("123", "1")).is_equal(1);
    Assert::that(libc::strspn("112233", "1")).is_equal(2);
}

TEST(Strcspn) {
    const char* my_str = "aaaaaa11122231233";

    Assert::that(libc::strcspn(my_str, "bbfgblkh0ko45678")).is_equal(libc::strlen(my_str));
    Assert::that(libc::strcspn("", "")).is_equal(0);
    Assert::that(libc::strcspn("", "asd")).is_equal(0);
    Assert::that(libc::strcspn("123", "1")).is_equal(0);
    Assert::that(libc::strcspn("112233", "3")).is_equal(4);
}

TEST(Strpbrk) {
    char my_str[] = "aaaaaa11122231233";
    Assert::that(libc::strpbrk(my_str, "1")).is_equal(my_str + 6);
    Assert::that(libc::strpbrk(my_str, "a")).is_equal(my_str);
    Assert::that(libc::strpbrk(my_str, "3")).is_equal(my_str + 12);


    char my_str1[] = "aaa";
    Assert::that(libc::strpbrk(my_str1, "b")).is_null();

    char my_str2[] = "";
    Assert::that(libc::strpbrk(my_str2, "")).is_null();
    Assert::that(libc::strpbrk(my_str2, "asd")).is_null();
}

TEST(Strstr) {
    char my_str[] = "asdtest123hello";

    Assert::that(libc::strstr(my_str, "")).is_equal(my_str);
    Assert::that(libc::strstr(my_str, "asd")).is_equal(my_str);
    Assert::that(libc::strstr(my_str, "test")).is_equal(my_str + 3);
    Assert::that(libc::strstr(my_str, "123")).is_equal(my_str + 7);
    Assert::that(libc::strstr(my_str, "hello")).is_equal(my_str + 10);
    Assert::that(libc::strstr(my_str, "hellow")).is_null();
    Assert::that(libc::strstr(my_str, "asdd")).is_null();

    char my_str1[] = "";
    Assert::that(libc::strstr(my_str1, "")).is_equal(my_str1);
    Assert::that(libc::strstr(my_str1, "asd")).is_null();
}

TEST(Memchr) {
    char my_str[] = "abcdefg123";

    Assert::that(libc::memchr(my_str, 'a', 0)).is_null();
    Assert::that(libc::memchr(my_str, 'a', 1)).is_equal(my_str);
    Assert::that(libc::memchr(my_str, 'd', 10)).is_equal(my_str + 3);
    Assert::that(libc::memchr(my_str, '3', 10)).is_equal(my_str + 9);
}

TEST(Memcmp) {
    char my_str[] = "abcd123";
    char my_str1[] = "abcd132";

    Assert::that(libc::memcmp(my_str, my_str1, 0)).is_equal(0);
    Assert::that(libc::memcmp(my_str, my_str1, 1)).is_equal(0);
    Assert::that(libc::memcmp(my_str, my_str1, 5)).is_equal(0);
    Assert::that(libc::memcmp(my_str, my_str1, 6)).is_equal('2' - '3');
    Assert::that(libc::memcmp(my_str1, my_str, 6)).is_equal('3' - '2');
}

TEST(Memset) {
    uint8_t my_array[3] = { 0xFF, 0xFF, 0xFF };

    libc::memset(my_array, 1, 1);
    Assert::that(my_array[0]).is_equal(1);
    Assert::that(my_array[1]).is_equal(0xFF);
    Assert::that(my_array[2]).is_equal(0xFF);

    my_array[0] = 0xFF;
    libc::memset(my_array, 1, 0);
    Assert::that(my_array[0]).is_equal(0xFF);
    Assert::that(my_array[1]).is_equal(0xFF);
    Assert::that(my_array[2]).is_equal(0xFF);
}

TEST(Memcpy) {
    uint8_t array[] = { 1, 2, 3, 4 };

    uint8_t new_array[4]{};

    libc::memcpy(new_array, array, 0);
    Assert::that(new_array[0]).is_equal(0);
    Assert::that(new_array[1]).is_equal(0);
    Assert::that(new_array[2]).is_equal(0);
    Assert::that(new_array[3]).is_equal(0);

    libc::memcpy(new_array, array, 2);
    Assert::that(new_array[0]).is_equal(1);
    Assert::that(new_array[1]).is_equal(2);
    Assert::that(new_array[2]).is_equal(0);
    Assert::that(new_array[3]).is_equal(0);

    new_array[0] = 0;
    new_array[1] = 0;
    libc::memcpy(new_array, array, 4);
    Assert::that(new_array[0]).is_equal(1);
    Assert::that(new_array[1]).is_equal(2);
    Assert::that(new_array[2]).is_equal(3);
    Assert::that(new_array[3]).is_equal(4);
}

TEST(Memmove) {
    uint8_t array[] = { 1, 2, 3, 4 };

    uint8_t new_array[4]{};

    libc::memmove(new_array, array, 0);
    Assert::that(new_array[0]).is_equal(0);
    Assert::that(new_array[1]).is_equal(0);
    Assert::that(new_array[2]).is_equal(0);
    Assert::that(new_array[3]).is_equal(0);

    libc::memmove(new_array, array, 2);
    Assert::that(new_array[0]).is_equal(1);
    Assert::that(new_array[1]).is_equal(2);
    Assert::that(new_array[2]).is_equal(0);
    Assert::that(new_array[3]).is_equal(0);

    new_array[0] = 0;
    new_array[1] = 0;
    libc::memmove(new_array, array, 4);
    Assert::that(new_array[0]).is_equal(1);
    Assert::that(new_array[1]).is_equal(2);
    Assert::that(new_array[2]).is_equal(3);
    Assert::that(new_array[3]).is_equal(4);

    libc::memmove(array, array + 1, 3);
    Assert::that(array[0]).is_equal(2);
    Assert::that(array[1]).is_equal(3);
    Assert::that(array[2]).is_equal(4);
    Assert::that(array[3]).is_equal(4);

    array[0] = 1;
    array[1] = 2;
    array[2] = 3;
    array[3] = 4;

    libc::memmove(array + 1, array, 3);
    Assert::that(array[0]).is_equal(1);
    Assert::that(array[1]).is_equal(1);
    Assert::that(array[2]).is_equal(2);
    Assert::that(array[3]).is_equal(3);
}

TEST(Strcasecmp) {
    Assert::that(libc::strcasecmp("Hell123WoRlZA", "hell123worlza")).is_equal(0);
    Assert::that(libc::strcasecmp("", "")).is_equal(0);
    Assert::that(libc::strcasecmp("a", "")).is_equal('a');
    Assert::that(libc::strcasecmp("", "a")).is_equal(-'a');
    Assert::that(libc::strcasecmp("A", "a")).is_equal(0);
}

TEST(Strncasecmp) {
    Assert::that(libc::strncasecmp("A", "b", 99)).is_equal(-1);
    Assert::that(libc::strncasecmp("b", "A", 99)).is_equal(1);
    Assert::that(libc::strncasecmp("a", "a", 99)).is_equal(0);
    Assert::that(libc::strncasecmp("", "", 99)).is_equal(0);
    Assert::that(libc::strncasecmp("asd", "", 99)).is_equal('a');
    Assert::that(libc::strncasecmp("", "Asd", 99)).is_equal(-'a');
    Assert::that(libc::strncasecmp("ASDasd", "asd", 3)).is_equal(0);
    Assert::that(libc::strncasecmp("asdasd", "ASD", 6)).is_equal('a');
}
