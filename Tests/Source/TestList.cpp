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
