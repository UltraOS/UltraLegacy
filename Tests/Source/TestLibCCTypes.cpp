#include "TestRunner.h"

namespace libc {

#include "LibC/ctype.h"
#include "LibC/ctype.c"

}

TEST(Iscntrl) {
    for (size_t i = 0; i < 32; ++i)
        Assert::that(libc::iscntrl(i)).is_not_equal(0);
    for (size_t i = 32; i < 127; ++i)
        Assert::that(libc::iscntrl(i)).is_equal(0);

    Assert::that(libc::iscntrl(127)).is_not_equal(0);
}

TEST(Isprint) {
    for (size_t i = 0; i < 32; ++i)
        Assert::that(libc::isprint(i)).is_equal(0);
    for (size_t i = 32; i < 127; ++i)
        Assert::that(libc::isprint(i)).is_not_equal(0);

    Assert::that(libc::isprint(127)).is_equal(0);
}

TEST(Isspace) {
    for (size_t i = 0; i < 9; ++i)
        Assert::that(libc::isspace(i)).is_equal(0);
    for (size_t i = 9; i < 14; ++i)
        Assert::that(libc::isspace(i)).is_not_equal(0);
    for (size_t i = 14; i < 32; ++i)
        Assert::that(libc::isspace(i)).is_equal(0);

    Assert::that(libc::isspace(32)).is_not_equal(0);

    for (size_t i = 33; i < 128; ++i)
        Assert::that(libc::isspace(i)).is_equal(0);
}

TEST(Isblank) {
    for (size_t i = 0; i < 9; ++i)
        Assert::that(libc::isblank(i)).is_equal(0);
    for (size_t i = 9; i < 10; ++i)
        Assert::that(libc::isblank(i)).is_not_equal(0);
    for (size_t i = 10; i < 32; ++i)
        Assert::that(libc::isblank(i)).is_equal(0);

    Assert::that(libc::isblank(32)).is_not_equal(0);

    for (size_t i = 33; i < 128; ++i)
        Assert::that(libc::isblank(i)).is_equal(0);
}

TEST(Isgraph) {
    for (size_t i = 0; i < 33; ++i)
        Assert::that(libc::isgraph(i)).is_equal(0);
    for (size_t i = 33; i < 127; ++i)
        Assert::that(libc::isgraph(i)).is_not_equal(0);

    Assert::that(libc::isgraph(127)).is_equal(0);
}

TEST(Ispunct) {
    for (size_t i = 0; i < 33; ++i)
        Assert::that(libc::ispunct(i)).is_equal(0);
    for (size_t i = 33; i < 48; ++i)
        Assert::that(libc::ispunct(i)).is_not_equal(0);
    for (size_t i = 48; i < 58; ++i)
        Assert::that(libc::ispunct(i)).is_equal(0);
    for (size_t i = 58; i < 65; ++i)
        Assert::that(libc::ispunct(i)).is_not_equal(0);
    for (size_t i = 65; i < 91; ++i)
        Assert::that(libc::ispunct(i)).is_equal(0);
    for (size_t i = 91; i < 97; ++i)
        Assert::that(libc::ispunct(i)).is_not_equal(0);
    for (size_t i = 97; i < 123; ++i)
        Assert::that(libc::ispunct(i)).is_equal(0);
    for (size_t i = 123; i < 127; ++i)
        Assert::that(libc::ispunct(i)).is_not_equal(0);

    Assert::that(libc::ispunct(127)).is_equal(0);
}

TEST(Isalnum) {
    for (size_t i = 0; i < 48; ++i)
        Assert::that(libc::isalnum(i)).is_equal(0);
    for (size_t i = 48; i < 58; ++i)
        Assert::that(libc::isalnum(i)).is_not_equal(0);
    for (size_t i = 58; i < 65; ++i)
        Assert::that(libc::isalnum(i)).is_equal(0);
    for (size_t i = 65; i < 91; ++i)
        Assert::that(libc::isalnum(i)).is_not_equal(0);
    for (size_t i = 91; i < 97; ++i)
        Assert::that(libc::isalnum(i)).is_equal(0);
    for (size_t i = 97; i < 123; ++i)
        Assert::that(libc::isalnum(i)).is_not_equal(0);
    for (size_t i = 123; i < 128; ++i)
        Assert::that(libc::isalnum(i)).is_equal(0);
}

TEST(Isalpha) {
    for (size_t i = 0; i < 65; ++i)
        Assert::that(libc::isalpha(i)).is_equal(0);
    for (size_t i = 65; i < 91; ++i)
        Assert::that(libc::isalpha(i)).is_not_equal(0);
    for (size_t i = 91; i < 97; ++i)
        Assert::that(libc::isalpha(i)).is_equal(0);
    for (size_t i = 97; i < 123; ++i)
        Assert::that(libc::isalpha(i)).is_not_equal(0);
    for (size_t i = 123; i < 128; ++i)
        Assert::that(libc::isalpha(i)).is_equal(0);
}

TEST(Isupper) {
    for (size_t i = 0; i < 65; ++i)
        Assert::that(libc::isupper(i)).is_equal(0);
    for (size_t i = 65; i < 91; ++i)
        Assert::that(libc::isupper(i)).is_not_equal(0);
    for (size_t i = 91; i < 128; ++i)
        Assert::that(libc::isupper(i)).is_equal(0);
}

TEST(Islower) {
    for (size_t i = 0; i < 97; ++i)
        Assert::that(libc::islower(i)).is_equal(0);
    for (size_t i = 97; i < 123; ++i)
        Assert::that(libc::islower(i)).is_not_equal(0);
    for (size_t i = 123; i < 128; ++i)
        Assert::that(libc::islower(i)).is_equal(0);
}

TEST(Isdigit) {
    for (size_t i = 0; i < 48; ++i)
        Assert::that(libc::isdigit(i)).is_equal(0);
    for (size_t i = 48; i < 58; ++i)
        Assert::that(libc::isdigit(i)).is_not_equal(0);
    for (size_t i = 58; i < 128; ++i)
        Assert::that(libc::isdigit(i)).is_equal(0);
}

TEST(Isxdigit) {
    for (size_t i = 0; i < 48; ++i)
        Assert::that(libc::isxdigit(i)).is_equal(0);
    for (size_t i = 48; i < 58; ++i)
        Assert::that(libc::isxdigit(i)).is_not_equal(0);
    for (size_t i = 58; i < 65; ++i)
        Assert::that(libc::isxdigit(i)).is_equal(0);
    for (size_t i = 65; i < 71; ++i)
        Assert::that(libc::isxdigit(i)).is_not_equal(0);
    for (size_t i = 71; i < 97; ++i)
        Assert::that(libc::isxdigit(i)).is_equal(0);
    for (size_t i = 97; i < 103; ++i)
        Assert::that(libc::isxdigit(i)).is_not_equal(0);
    for (size_t i = 103; i < 128; ++i)
        Assert::that(libc::isxdigit(i)).is_equal(0);
}

TEST(Tolower) {
    for (size_t i = 0; i < 'A'; ++i)
        Assert::that(libc::tolower(i)).is_equal(i);
    for (size_t i = 'A'; i <= 'Z'; ++i)
        Assert::that(libc::tolower(i)).is_equal(i + ('a' - 'A'));
    for (size_t i = 'Z' + 1; i < 128; ++i)
        Assert::that(libc::tolower(i)).is_equal(i);
}

TEST(Toupper) {
    for (size_t i = 0; i < 'a'; ++i)
        Assert::that(libc::toupper(i)).is_equal(i);
    for (size_t i = 'a'; i <= 'z'; ++i)
        Assert::that(libc::toupper(i)).is_equal(i - ('a' - 'A'));
    for (size_t i = 'z' + 1; i < 128; ++i)
        Assert::that(libc::toupper(i)).is_equal(i);
}

#ifndef BROKEN_NATIVE_LIBC
TEST(NativeLibC) {
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isalnum(i)).is_equal(!!std::isalnum(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isalpha(i)).is_equal(!!std::isalpha(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::islower(i)).is_equal(!!std::islower(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isupper(i)).is_equal(!!std::isupper(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isdigit(i)).is_equal(!!std::isdigit(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isxdigit(i)).is_equal(!!std::isxdigit(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::iscntrl(i)).is_equal(!!std::iscntrl(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isgraph(i)).is_equal(!!std::isgraph(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isspace(i)).is_equal(!!std::isspace(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isblank(i)).is_equal(!!std::isblank(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::isprint(i)).is_equal(!!std::isprint(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(!!libc::ispunct(i)).is_equal(!!std::ispunct(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(libc::tolower(i)).is_equal(std::tolower(i));
    for (size_t i = 0; i < 128; ++i)
        Assert::that(libc::toupper(i)).is_equal(std::toupper(i));
}
#endif
