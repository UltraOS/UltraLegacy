#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <string>
#include <stdexcept>
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
            return TO_STRING(case_name);                             \
        }                                                            \
        bool run() override;                                         \
    } static test_##case_name;                                       \
    bool Test##case_name::run()

class TestRunner {
    friend class Test;
public:
    static int run_all()
    {
        size_t fail_count = 0;
        size_t pass_count = 0;

        std::cout << "Running " << s_tests.size() << " tests..." << std::endl;

        for (auto test : s_tests) {
            std::cout << "Running test \"" << test->name() << "\"... ";

            bool result = false;
            std::string reason = "unknown";

            try {
                result = test->run();
            } catch (const std::exception& ex) {
                reason = ex.what();
            }

            if (result) {
                 std::cout << "\u001b[32mPASSED!\u001b[0m" << std::endl;
                 ++pass_count;
            } else {
                std::cout << "\u001b[31mFAILED!\u001b[0m" << " (Reason: " << reason << ")" << std::endl;
                ++fail_count;
            }
        }

        std::cout << std::endl << "Results => "
                  << pass_count << " passed / "
                  << fail_count << " failed (" << pass_count << "/"
                  << s_tests.size()  << ")" << std::endl;

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

template <typename T>
inline std::enable_if_t<std::is_integral_v<T>, std::string> to_hex_string(T value)
{
    std::stringstream ss;

    ss << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << value;

    return ss.str();
}

template <typename T>
inline std::enable_if_t<std::is_pointer_v<T>, std::string> to_hex_string(T ptr)
{
    return to_hex_string<size_t>(reinterpret_cast<size_t>(ptr));
}

class Assert {
public:
    template<typename T>
    class Asserter {
    public:
        Asserter(T value) : m_value(value) { }

        void is_equal(const T& other) {
            if (m_value != other)
                throw std::runtime_error(to_hex_string(m_value) + " != " + to_hex_string(other));
        }

        void is_not_equal(const T& other) {
            if (m_value == other)
                throw std::runtime_error(to_hex_string(m_value) + " == " + to_hex_string(other));
        }

        void is_null() {
            if (m_value != nullptr)
                throw std::runtime_error(to_hex_string(m_value) + " != nullptr");
        }

        void is_not_null() {
            if (m_value == nullptr)
                throw std::runtime_error(to_hex_string(m_value) + " == nullptr");
        }

        void is_less_than(const T& other) {
            if (m_value > other)
                throw std::runtime_error(to_hex_string(m_value) + " > " + to_hex_string(other));
        }

        void is_greater_than(const T& other) {
            if (m_value < other)
                throw std::runtime_error(to_hex_string(m_value) + " < " + to_hex_string(other));
        }

    private:
        T m_value;
    };

    template <typename T>
    static Asserter<T> that(T value) { return Asserter<T>(value); }
};
