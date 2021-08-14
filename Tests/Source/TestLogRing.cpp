#include "TestRunner.h"

#include "Common/LogRing.h"

#define EXPECT_N_ITERATIONS_AND_RECORDS(N, ...)                         \
             size_t iterations = 0;                                     \
             std::vector<std::string> expected_records = __VA_ARGS__;   \
             std::vector<std::string> records;                          \
             records.reserve(N);                                        \
                                                                        \
             for (auto record : lr) {                                   \
                Assert::that(++iterations).is_less_than_or_equal(N);    \
                records.emplace_back(record.data(), record.size());     \
             }                                                          \
                                                                        \
             Assert::that(iterations).is_equal(N);                      \
                                                                        \
             for (size_t i = 0; i < N; ++i)                             \
                 Assert::that(records[i]).is_equal(expected_records[i]);


TEST(SingleRow) {
    kernel::LogRing<1, 10> lr;

    {
        EXPECT_N_ITERATIONS_AND_RECORDS(0, {});
    }
    {
        lr.write("hello");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hello" });
    }
    {
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hello" });
    }
    {
        lr.write("123\n");
        lr.write("324\n");
        lr.write("test\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "test" });
    }
    {
        lr.write("123");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "123" });
    }
}

TEST(PartialRecord) {
    kernel::LogRing<3, 10> lr;

    {
        lr.write("hello");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hello" });
    }
    {
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hello" });
    }
    {
        lr.write("a");
        EXPECT_N_ITERATIONS_AND_RECORDS(2, { "hello", "a" });
    }
    {
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(2, { "hello", "a" });
    }
    {
        lr.write("asd");
        EXPECT_N_ITERATIONS_AND_RECORDS(3, { "hello", "a", "asd" });
    }
    {
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(3, { "hello", "a", "asd" });
    }
    {
        lr.write("dsa");
        EXPECT_N_ITERATIONS_AND_RECORDS(3, { "a", "asd", "dsa" });
    }
}

TEST(NewLines) {
    kernel::LogRing<3, 10> lr;

    {
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(0, {});
    }
    {
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(0, {});
    }
    lr.write("hello");
    {
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hello" });
    }
    lr.write("world");
    {
        EXPECT_N_ITERATIONS_AND_RECORDS(2, { "hello", "world" });
    }
}

TEST(ColumnWidth)
{
    kernel::LogRing<3, 3> lr;

    {
        lr.write("hel");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hel" });
    }
    {
        lr.write("\n");
        EXPECT_N_ITERATIONS_AND_RECORDS(1, { "hel" });
    }
    {
        lr.write("wo");
        EXPECT_N_ITERATIONS_AND_RECORDS(2, { "hel", "wo" });
    }
    {
        lr.write("rld");
        EXPECT_N_ITERATIONS_AND_RECORDS(3, { "hel", "wor", "ld" });
    }
    {
        lr.write("\ndeadbeef!");
        EXPECT_N_ITERATIONS_AND_RECORDS(3, { "dea", "dbe", "ef!" });
    }
}
