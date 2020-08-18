#pragma once

#include <iostream>
#include <string_view>
#include <string>
#include <vector>

class Test
{
public:
    virtual std::string_view name() const = 0;

    virtual bool run() = 0;

protected:
    Test();
};

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#define TEST(case_name)                                              \
    class Test##case_name : public Test {                            \
    public:                                                          \
        std::string_view name() const override {                     \
            return TO_STRING(case_name);                           \
        }                                                            \
        bool run() override;                                         \
    } static test_##case_name;                                       \
    bool Test##case_name::run()

class TestRunner {
    friend class Test;
public:
    static int run_all()
    {
        std::cout << "Running " << s_tests.size() << " tests..." << std::endl;

        for (auto test : s_tests) {
            std::cout << "Running test \"" << test->name() << "\"... ";
            bool result = test->run();

            if (result)
                 std::cout << "\u001b[32mPASSED!\u001b[0m" << std::endl;
            else
                std::cout << "\u001b[31mFAILED!\u001b[0m" << std::endl;
        }

        return 0;
    }

private:
    static void register_self(Test* self)
    {
        s_tests.push_back(self);
    }

private:
    inline static std::vector<Test*> s_tests;
};

inline Test::Test()
{
    TestRunner::register_self(this);
}

#define private public
