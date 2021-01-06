#include "Common/Macros.h" // koenig lookup
#define FailedAssertion(ignored1, ignored2, ignored3) "FIXME :)"
#include "Common/List.h"
#undef FailedAssertion

#include "TestRunner.h"

TEST(AppendFront) {
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

TEST(AppendBack) {
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

TEST(Destructor) {
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

TEST(SpliceSingleEndToBegin) {
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

    Assert::that(itr).is_equal(list.end());
}

TEST(SpliceSingleBeginToEnd) {
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

    Assert::that(itr).is_equal(list.end());
}

TEST(MultipleSpliceFront) {
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

    Assert::that(itr).is_equal(list_1.end());
}

TEST(Erase) {
    using kernel::List;

    List<int> l;
    l.append_back(1);
    l.append_back(2);
    l.append_back(3);

    l.erase(++l.begin());
    Assert::that(l.size()).is_equal(2);
    Assert::that(l.front()).is_equal(1);
    Assert::that(*(++l.begin())).is_equal(3);
    Assert::that(*(--(--l.end()))).is_equal(1);

    l.erase(--l.end());
    Assert::that(l.size()).is_equal(1);

    l.erase(l.begin());
    Assert::that(l.size()).is_equal(0);
}
