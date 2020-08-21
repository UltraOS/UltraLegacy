#pragma once

// Koenig lookup workaround
#ifdef move
#undef move
#endif

#ifdef forward
#undef forward
#endif

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

    virtual void run() = 0;

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
        void run() override;                                         \
    } static test_##case_name;                                       \
    void Test##case_name::run()

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

            std::string failure_reason;

            try {
                test->run();
            } catch (const std::exception& ex) {
                failure_reason = ex.what();
            }

            if (failure_reason.empty()) {
                 std::cout << "\u001b[32mPASSED!\u001b[0m" << std::endl;
                 ++pass_count;
            } else {
                std::cout << "\u001b[31mFAILED!\u001b[0m" << " (Reason: " << failure_reason << ")" << std::endl;
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
inline std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>, std::string> to_hex_string(T ptr)
{
    return to_hex_string<size_t>(reinterpret_cast<size_t>(ptr));
}

class Assert {
public:
    template<typename T, typename V = void>
    class Asserter {
    public:
        Asserter(T value) : m_value(value) { }

        void is_equal(const T& other) {
            if (m_value != other)
                throw std::runtime_error(std::to_string(m_value) + " != " + std::to_string(other));
        }

        void is_not_equal(const T& other) {
            if (m_value == other)
                throw std::runtime_error(std::to_string(m_value) + " == " + std::to_string(other));
        }

        void is_less_than(const T& other) {
            if (m_value > other)
                throw std::runtime_error(m_value + " > " + other);
        }

        void is_greater_than(const T& other) {
            if (m_value < other)
                throw std::runtime_error(m_value + " < " + other);
        }

    private:
        T m_value;
    };

    template <typename T>
    class Asserter<T, typename std::enable_if<(std::is_pointer_v<T> ||
                                               std::is_null_pointer_v<T>) &&
                                              !std::is_same_v<T, const char*>>::type> {
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

    private:
        T m_value;
    };

    template <typename T>
    class Asserter<T, typename std::enable_if<std::is_same_v<T, std::string> ||
                                              std::is_same_v<T, const char*>>::type>
    {
    public:
        Asserter(std::string_view value) : m_value(value.data(), value.size()) { }

        void is_equal(const std::string& other) {
            if (m_value != other)
                throw std::runtime_error(m_value + " != " + other);
        }

        void is_not_equal(const std::string& other) {
            if (m_value == other)
                throw std::runtime_error(m_value + " == " + other);
        }

    private:
        std::string m_value;
    };

    template <typename T>
    static Asserter<T> that(T value) { return Asserter<T>(value); }
};
