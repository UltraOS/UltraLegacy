#include "TestRunner.h"

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

    Assert::that(res_8.has_value()).is_false();
    Assert::that(res_9.has_value()).is_false();
    Assert::that(res_10.has_value()).is_false();

    Assert::that(res_11.has_value()).is_true();
    auto& val_11 = res_11.value();
    Assert::that(val_11.first().empty()).is_true();
    Assert::that(val_11.second()).is_equal("/hello/./world/../test/../something/..");
}
