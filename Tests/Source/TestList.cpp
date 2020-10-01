#include "Common/Macros.h" // koenig lookup
#include "Common/List.h"

#include "TestRunner.h"

TEST(ListAppendFront) {
    using kernel::List;

    List<std::string> list;

    static constexpr size_t test_size = 100;

    for (size_t i = 0; i < 100; ++i) {
        std::string text = "some_string " + std::to_string(i);

        list.append_front(text);
        Assert::that(list.front()).is_equal(text);
    }

    Assert::that(list.size()).is_equal(test_size);
}

TEST(ListAppendBack) {
    using kernel::List;

    List<std::string> list;

    static constexpr size_t test_size = 100;

    for (size_t i = 0; i < 100; ++i) {
        std::string text = "some_string " + std::to_string(i);

        list.append_back(text);
        Assert::that(list.back()).is_equal(text);
    }

    Assert::that(list.size()).is_equal(test_size);
}

TEST(ListDestructor) {
    using kernel::List;

    static constexpr size_t test_size = 100;

    size_t deleted_counter = 0;

    {
        List<Deletable> list;

        for (size_t i = 0; i < 100; ++i) {
            list.append_back(deleted_counter);
        }
    }

    Assert::that(deleted_counter).is_equal(test_size);
}

TEST(ListSpliceSingleEndToBegin) {
    using kernel::List;

    List<int> list;
    list.append_back(1);
    list.append_back(2);
    list.append_back(3);
    list.append_back(4);

    list.splice(list.begin(), list, --list.end());
    list.splice(list.begin(), list, --list.end());
    list.splice(list.begin(), list, --list.end());

    auto itr = list.begin();

    Assert::that(list.size()).is_equal(4);
    Assert::that(*(itr++)).is_equal(2);
    Assert::that(*(itr++)).is_equal(3);
    Assert::that(*(itr++)).is_equal(4);
    Assert::that(*(itr++)).is_equal(1);

    // TODO: this should be usable within the asserter?
    bool is_end = itr == list.end();

    Assert::that(is_end).is_equal(true);
}

TEST(ListSpliceSingleBeginToEnd) {
    using kernel::List;

    List<int> list;
    list.append_back(4);
    list.append_back(3);
    list.append_back(2);
    list.append_back(1);

    list.splice(list.end(), list, list.begin());
    list.splice(list.end(), list, list.begin());
    list.splice(list.end(), list, list.begin());

    auto itr = list.begin();

    Assert::that(list.size()).is_equal(4);
    Assert::that(*(itr++)).is_equal(1);
    Assert::that(*(itr++)).is_equal(4);
    Assert::that(*(itr++)).is_equal(3);
    Assert::that(*(itr++)).is_equal(2);

    // TODO: this should be usable within the asserter?
    bool is_end = itr == list.end();

    Assert::that(is_end).is_equal(true);
}

TEST(ListMultipleSpliceFront) {
    using kernel::List;

    List<int> list_1;
    list_1.append_back(1);
    list_1.append_back(2);

    List<int> list_2;
    list_2.append_back(3);
    list_2.append_back(4);

    list_1.splice(list_1.begin(), list_2, list_2.begin());
    list_1.splice(list_1.begin(), list_2, list_2.begin());

    Assert::that(list_1.size()).is_equal(4);
    Assert::that(list_2.size()).is_equal(0);

    auto itr = list_1.begin();

    Assert::that(*(itr++)).is_equal(4);
    Assert::that(*(itr++)).is_equal(3);
    Assert::that(*(itr++)).is_equal(1);
    Assert::that(*(itr++)).is_equal(2);

    // TODO: this should be usable within the asserter?
    bool is_end = itr == list_1.end();

    Assert::that(is_end).is_equal(true);
}
