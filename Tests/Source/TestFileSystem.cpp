#include "TestRunner.h"
#include <array>

#include "FileSystem/Utilities.h"
#include "FileSystem/FAT32/Utilities.h"

TEST(SplitPrefxAndPath) {

    using sv = kernel::StringView;

    sv path_1 = "/hello/word/";
    sv path_2 = "::/hello/world";
    sv path_3 = "ASD::/hello/world";
    sv path_4 = "::ASD::/hello/world";
    sv path_5 = "::ASD/hello/world";
    sv path_6 = "ASD/hello/world";
    sv path_7 = "ASD::/::hello";
    sv path_8 = "::///ASD///hello world";
    sv path_9 = "::/:://::test";
    sv path_10 = "/hello/.../world/";
    sv path_11 = "/hello/./world/../test/../something/..";

    auto res_1 = kernel::split_prefix_and_path(path_1);
    auto res_2 = kernel::split_prefix_and_path(path_2);
    auto res_3 = kernel::split_prefix_and_path(path_3);
    auto res_4 = kernel::split_prefix_and_path(path_4);
    auto res_5 = kernel::split_prefix_and_path(path_5);
    auto res_6 = kernel::split_prefix_and_path(path_6);
    auto res_7 = kernel::split_prefix_and_path(path_7);
    auto res_8 = kernel::split_prefix_and_path(path_8);
    auto res_9 = kernel::split_prefix_and_path(path_9);
    auto res_10 = kernel::split_prefix_and_path(path_10);
    auto res_11 = kernel::split_prefix_and_path(path_11);


    Assert::that(res_1.has_value()).is_true();
    auto& val_1 = res_1.value();
    Assert::that(val_1.first.empty()).is_true();
    Assert::that(val_1.second).is_equal(path_1);

    Assert::that(res_2.has_value()).is_true();
    auto& val_2 = res_2.value();
    Assert::that(val_2.first.empty()).is_true();
    Assert::that(val_2.second).is_equal("/hello/world");

    Assert::that(res_3.has_value()).is_true();
    auto& val_3 = res_3.value();
    Assert::that(val_3.first).is_equal("ASD");
    Assert::that(val_3.second).is_equal("/hello/world");

    Assert::that(res_4.has_value()).is_false();
    Assert::that(res_5.has_value()).is_false();
    Assert::that(res_6.has_value()).is_false();

    Assert::that(res_7.has_value()).is_true();
    auto& val_7 = res_7.value();
    Assert::that(val_7.first).is_equal("ASD");
    Assert::that(val_7.second).is_equal("/::hello");

    Assert::that(res_8.has_value()).is_true();
    auto& val_8 = res_8.value();
    Assert::that(val_8.first.empty()).is_true();
    Assert::that(val_8.second).is_equal("///ASD///hello world");

    Assert::that(res_9.has_value()).is_true();
    auto& val_9 = res_9.value();
    Assert::that(val_9.first.empty()).is_true();
    Assert::that(val_9.second).is_equal("/:://::test");

    Assert::that(res_10.has_value()).is_false();

    Assert::that(res_11.has_value()).is_true();
    auto& val_11 = res_11.value();
    Assert::that(val_11.first.empty()).is_true();
    Assert::that(val_11.second).is_equal("/hello/./world/../test/../something/..");
}

TEST(NextNode) {
    using namespace kernel;

    StringView path_1 = "/hello/world/./../A/B/C"_sv;

    const std::array<std::string, 7> nodes_1 = {
        "hello", "world", ".", "..", "A", "B", "C"
    };

    size_t index_1 = 0;
    for (auto c : IterablePath(path_1)) {
        Assert::that(index_1).is_less_than(7);
        Assert::that(nodes_1[index_1++]).is_equal(std::string(c.begin(), c.end()));
    }

    StringView path_2 = "/test/1/"_sv;

    const std::array<std::string, 2> nodes_2 = {
        "test", "1"
    };

    size_t index_2 = 0;
    for (auto c : IterablePath(path_2)) {
        Assert::that(index_2).is_less_than(2);
        Assert::that(nodes_2[index_2++]).is_equal(std::string(c.begin(), c.end()));
    }

    StringView path_3 = "///test////1//h/123//////"_sv;

    const std::array<std::string, 4> nodes_3 = {
        "test", "1", "h", "123"
    };

    size_t index_3 = 0;
    for (auto c : IterablePath(path_3)) {
        Assert::that(index_3).is_less_than(4);
        Assert::that(nodes_3[index_3++]).is_equal(std::string(c.begin(), c.end()));
    }
}

TEST(GenerateShortName) {
    kernel::String str1("Gamer.txt");
    auto short_name1 = kernel::generate_short_name(str1.to_view());
    Assert::that(short_name1).is_equal("GAMER   TXT");

    kernel::String str2("Very Long Gamer That Likes to Game.txt");
    auto short_name2 = kernel::generate_short_name(str2.to_view());
    Assert::that(short_name2).is_equal("VERYLO~1TXT");

    kernel::String str3("Long.And.With.A.Buch.Of.Dots");
    auto short_name3 = kernel::generate_short_name(str3.to_view());
    Assert::that(short_name3).is_equal("LONGAN~1DOT");

    kernel::String str4("A      Bunch    of spaces everywhere.lolAndLongExtension");
    auto short_name4 = kernel::generate_short_name(str4.to_view());
    Assert::that(short_name4).is_equal("ABUNCH~1LOL");
}

TEST(NextShortName) {
    kernel::String str1("A       TXT");

    bool ok = true;

    for (size_t i = 0; i < 999'999; i++) {
        str1 = kernel::next_short_name(str1.to_view(), ok);
        Assert::that(ok).is_true();

        switch (i) {
        case 0:
            Assert::that(str1).is_equal("A~1     TXT");
            break;
        case 9:
            Assert::that(str1).is_equal("A~10    TXT");
            break;
        case 99:
            Assert::that(str1).is_equal("A~100   TXT");
            break;
        case 999:
            Assert::that(str1).is_equal("A~1000  TXT");
            break;
        case 9999:
            Assert::that(str1).is_equal("A~10000 TXT");
            break;
        case 99999:
            Assert::that(str1).is_equal("A~100000TXT");
            break;
        default:
            break;
        }
    }

    Assert::that(str1).is_equal("A~999999TXT");
    kernel::next_short_name(str1.to_view(), ok);
    Assert::that(ok).is_false();

    kernel::String str2("ABCDEFG TXT");

    for (size_t i = 0; i < 999'999; i++) {
        str2 = kernel::next_short_name(str2.to_view(), ok);
        Assert::that(ok).is_true();

        switch (i) {
            case 0:
                Assert::that(str2).is_equal("ABCDEF~1TXT");
                break;
            case 9:
                Assert::that(str2).is_equal("ABCDE~10TXT");
                break;
            case 99:
                Assert::that(str2).is_equal("ABCD~100TXT");
                break;
            case 999:
                Assert::that(str2).is_equal("ABC~1000TXT");
                break;
            case 9999:
                Assert::that(str2).is_equal("AB~10000TXT");
                break;
            case 99999:
                Assert::that(str2).is_equal("A~100000TXT");
                break;
            default:
                break;
        }
    }

    Assert::that(str2).is_equal("A~999999TXT");

    kernel::String str3("HJKLMNO    ");

    for (size_t i = 0; i < 999'999; i++) {
        str3 = kernel::next_short_name(str3.to_view(), ok);
        Assert::that(ok).is_true();

        switch (i) {
            case 0:
                Assert::that(str3).is_equal("HJKLMN~1   ");
                break;
            case 9:
                Assert::that(str3).is_equal("HJKLM~10   ");
                break;
            case 99:
                Assert::that(str3).is_equal("HJKL~100   ");
                break;
            case 999:
                Assert::that(str3).is_equal("HJK~1000   ");
                break;
            case 9999:
                Assert::that(str3).is_equal("HJ~10000   ");
                break;
            case 99999:
                Assert::that(str3).is_equal("H~100000   ");
                break;
            default:
                break;
        }
    }

    Assert::that(str3).is_equal("H~999999   ");
    kernel::next_short_name(str3.to_view(), ok);
    Assert::that(ok).is_false();
}

TEST(ShortNameChecksum) {
    kernel::StringView name1 = "LONG-N~1TXT";
    Assert::that(kernel::generate_short_name_checksum(name1)).is_equal(0x10);

    kernel::StringView name2 = "OTHER-~1TXT";
    Assert::that(kernel::generate_short_name_checksum(name2)).is_equal(0xD1);
}
