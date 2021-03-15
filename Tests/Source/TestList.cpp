#define FailedAssertion(ignored1, ignored2, ignored3) "FIXME :)"
#include "Common/Macros.h" // koenig lookup
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

TEST(Move) {
    size_t deleted_count = 0;

    auto* l = new kernel::List<Deletable>;

    for (size_t i = 0; i < 100; ++i)
        l->emplace_back(deleted_count);

    {
        kernel::List<Deletable> l2(move(*l));
        Assert::that(l2.size()).is_equal(100);
        Assert::that(l->empty()).is_true();

        for (auto& elem : *l)
            Assert::never_reached("too many elements");

        size_t count = 0;
        for (auto& elem : l2)
            ++count;

        Assert::that(count).is_equal(100);

        *l = move(l2);
    }

    Assert::that(l->size()).is_equal(100);

    size_t count = 0;
    for (auto& elem : *l)
        ++count;

    Assert::that(count).is_equal(100);
    delete l;

    Assert::that(deleted_count).is_equal(100);
}

TEST(Copy) {
    struct CopyableDeletable {
    public:
        CopyableDeletable(const CopyableDeletable& cd)
        {
        }

        CopyableDeletable(CopyableDeletable&& cd)
        {
            std::swap(cd.m_counter, m_counter);
        }

        CopyableDeletable(size_t& counter)
        {
            m_counter = &counter;
        }

        ~CopyableDeletable()
        {
            if (m_counter)
                (*m_counter)++;
        }

    private:
        size_t* m_counter { nullptr };
    };

    size_t deleted_count = 0;

    {
        kernel::List<CopyableDeletable> l1;

        for (size_t i = 0; i < 100; ++i)
            l1.emplace_back(deleted_count);

        auto l2 = l1;
        kernel::List<CopyableDeletable> l3;
        l3 = l2;

        Assert::that(l1.size()).is_equal(100);
        Assert::that(l2.size()).is_equal(100);
        Assert::that(l3.size()).is_equal(100);

        size_t i = 0;
        for (auto& elem : l1)
            i++;

        Assert::that(i).is_equal(100);

        i = 0;
        for (auto& elem : l2)
            i++;

        Assert::that(i).is_equal(100);

        i = 0;
        for (auto& elem : l3)
            i++;

        Assert::that(i).is_equal(100);
    }

    Assert::that(deleted_count).is_equal(100);
}

struct thing : public kernel::StandaloneListNode<thing> {
    thing(int x) : x(x) {}

    int x;
};

TEST(StandaloneInsertPop) {
    auto list = kernel::List<thing>();

    auto t1 = thing(1);
    auto t2 = thing(2);
    auto t3 = thing(3);

    list.insert_back(t1);
    Assert::that(list.front().x).is_equal(1);

    list.insert_front(t2);
    Assert::that(list.front().x).is_equal(2);

    list.insert_back(t3);
    Assert::that(list.back().x).is_equal(3);

    Assert::that(list.size()).is_equal(3);

    Assert::that(list.pop_front().x).is_equal(2);
    Assert::that(list.pop_back().x).is_equal(3);
    Assert::that(list.pop_front().x).is_equal(1);

    list.insert_back(t1);
    Assert::that(t1.is_on_a_list()).is_true();
    Assert::that(list.size()).is_equal(1);
    t1.pop_off();
    Assert::that(list.size()).is_equal(0);
    Assert::that(t1.is_on_a_list()).is_false();
}

TEST(StandaloneMove) {
    auto list = kernel::List<thing>();

    auto t1 = thing(1);
    auto t2 = thing(2);
    auto t3 = thing(3);

    list.insert_back(t1);
    list.insert_back(t2);
    list.insert_back(t3);

    {
        auto list2 = move(list);

        for (auto& elem : list)
            Assert::never_reached("too many elements");

        Assert::that(list2.size()).is_equal(3);
        Assert::that(t1.list()).is_equal(&list2);
        Assert::that(t2.list()).is_equal(&list2);
        Assert::that(t3.list()).is_equal(&list2);
        Assert::that(list.empty()).is_true();

        size_t i = 0;
        for (auto& elem : list2) {
            if (i == 0) Assert::that(&elem).is_equal(&t1);
            if (i == 1) Assert::that(&elem).is_equal(&t2);
            if (i == 2) Assert::that(&elem).is_equal(&t3);
            if (i > 2) Assert::never_reached("too many elements");

            ++i;
        }

        list = move(list2);
    }

    Assert::that(list.size()).is_equal(3);
    Assert::that(t1.list()).is_equal(&list);
    Assert::that(t2.list()).is_equal(&list);
    Assert::that(t3.list()).is_equal(&list);

    size_t i = 0;
    for (auto& elem : list) {
        if (i == 0) Assert::that(&elem).is_equal(&t1);
        if (i == 1) Assert::that(&elem).is_equal(&t2);
        if (i == 2) Assert::that(&elem).is_equal(&t3);
        if (i > 2) Assert::never_reached("too many elements");

        ++i;
    }

    auto empty_list_1 = kernel::List<thing>();
    auto empty_list_2 = kernel::List<thing>();

    empty_list_2 = move(empty_list_1);
    empty_list_1 = move(empty_list_2);

    Assert::that(empty_list_1.empty()).is_true();
    Assert::that(empty_list_2.empty()).is_true();
}
