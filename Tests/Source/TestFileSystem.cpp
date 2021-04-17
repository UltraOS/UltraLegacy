#include "TestRunner.h"
#include <array>

#include "FileSystem/Utilities.h"

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
    Assert::that(val_1.first().empty()).is_true();
    Assert::that(val_1.second()).is_equal(path_1);

    Assert::that(res_2.has_value()).is_true();
    auto& val_2 = res_2.value();
    Assert::that(val_2.first().empty()).is_true();
    Assert::that(val_2.second()).is_equal("/hello/world");

    Assert::that(res_3.has_value()).is_true();
    auto& val_3 = res_3.value();
    Assert::that(val_3.first()).is_equal("ASD");
    Assert::that(val_3.second()).is_equal("/hello/world");

    Assert::that(res_4.has_value()).is_false();
    Assert::that(res_5.has_value()).is_false();
    Assert::that(res_6.has_value()).is_false();

    Assert::that(res_7.has_value()).is_true();
    auto& val_7 = res_7.value();
    Assert::that(val_7.first()).is_equal("ASD");
    Assert::that(val_7.second()).is_equal("/::hello");

    Assert::that(res_8.has_value()).is_true();
    auto& val_8 = res_8.value();
    Assert::that(val_8.first().empty()).is_true();
    Assert::that(val_8.second()).is_equal("///ASD///hello world");

    Assert::that(res_9.has_value()).is_true();
    auto& val_9 = res_9.value();
    Assert::that(val_9.first().empty()).is_true();
    Assert::that(val_9.second()).is_equal("/:://::test");

    Assert::that(res_10.has_value()).is_false();

    Assert::that(res_11.has_value()).is_true();
    auto& val_11 = res_11.value();
    Assert::that(val_11.first().empty()).is_true();
    Assert::that(val_11.second()).is_equal("/hello/./world/../test/../something/..");
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
