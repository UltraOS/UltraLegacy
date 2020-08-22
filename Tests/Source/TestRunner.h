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

class Fixture
{
public:
    virtual std::string_view name() const = 0;

    virtual void run() = 0;

protected:
    Fixture();
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

#define FIXTURE(fname)                                               \
    class Fixture##fname : public Fixture {                          \
    public:                                                          \
        std::string_view name() const override {                     \
            return TO_STRING(fname);                                 \
        }                                                            \
        void run() override;                                         \
    } static fixture_##fname;                                        \
    void Fixture##fname::run()


class FailedAssertion : public std::exception {
public:
    FailedAssertion(std::string_view message, std::string_view file, size_t line)
        : m_message(message), m_file(file), m_line(line) { }

    const char* what() const noexcept override { return m_message.data(); }
    const std::string& file()    const { return m_file; }
    size_t line()                const { return m_line; }

private:
    std::string m_message;
    std::string m_file;
    size_t      m_line;
};

class TestRunner {
    friend class Test;
    friend class Fixture;
public:
    static int run_all()
    {
        size_t fail_count = 0;
        size_t pass_count = 0;

        std::cout << "Running " << s_fixtures.size() << " fixtures..." << std::endl;

        for (auto fixture : s_fixtures) {
            try {
                std::cout << "Running fixture \"" << fixture->name() << "\"... ";
                fixture->run();
                std::cout << "\u001b[32mOK\u001b[0m" << std::endl;
            } catch (const FailedAssertion& ex) {
                std::cout << "\u001b[31mFAILED!\u001b[0m " << build_error_message(ex) << std::endl;
                std::cout << "Terminating..." << std::endl;
                return -1;
            } catch (const std::exception& ex) {
                std::cout << "\u001b[31mFAILED!\u001b[0m " << ex.what() << std::endl;
                std::cout << "Terminating..." << std::endl;
                return -1;
            }
        }

        std::cout << "Running " << s_tests.size() << " tests..." << std::endl;

        for (auto test : s_tests) {
            std::cout << "Running test \"" << test->name() << "\"... ";

            std::string failure_reason;

            try {
                test->run();
            } catch (const FailedAssertion& ex) {
                failure_reason = build_error_message(ex);
            } catch (const std::exception& ex) {
                failure_reason = ex.what();
            }

            if (failure_reason.empty()) {
                 std::cout << "\u001b[32mPASSED!\u001b[0m" << std::endl;
                 ++pass_count;
            } else {
                std::cout << "\u001b[31mFAILED!\u001b[0m" << " (" << failure_reason << ")" << std::endl;
                ++fail_count;
            }
        }

        std::cout << std::endl << "Results => "
                  << pass_count << " passed / "
                  << fail_count << " failed (" << pass_count << "/"
                  << s_tests.size()  << ")" << std::endl;

        return static_cast<int>(fail_count);
    }

private:
    static std::string build_error_message(const FailedAssertion& ex)
    {
        std::string error_message;

        error_message += "At ";
        error_message += ex.file();
        error_message += ":";
        error_message += std::to_string(ex.line());
        error_message += " -> ";
        error_message += ex.what();

        return error_message;
    }

    static void register_self(Test* self)
    {
        s_tests.push_back(self);
    }

    static void register_self(Fixture* self)
    {
        s_fixtures.push_back(self);
    }

private:
    inline static std::vector<Test*>    s_tests;
    inline static std::vector<Fixture*> s_fixtures;
};

inline Test::Test()
{
    TestRunner::register_self(this);
}

inline Fixture::Fixture()
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
        Asserter(T value, std::string_view file, size_t line) : m_value(value), m_file(file), m_line(line) { }

        void is_equal(const T& other) {
            if (m_value != other)
                throw FailedAssertion(std::to_string(m_value) + " != " + std::to_string(other), m_file, m_line);
        }

        void is_not_equal(const T& other) {
            if (m_value == other)
                throw FailedAssertion(std::to_string(m_value) + " == " + std::to_string(other), m_file, m_line);
        }

        void is_less_than(const T& other) {
            if (m_value > other)
                throw FailedAssertion(m_value + " > " + other, m_file, m_line);
        }

        void is_greater_than(const T& other) {
            if (m_value < other)
                throw FailedAssertion(m_value + " < " + other, m_file, m_line);
        }

    private:
        T m_value;

        std::string m_file;
        size_t      m_line;
    };

    template <typename T>
    class Asserter<T, typename std::enable_if<(std::is_pointer_v<T> ||
                                               std::is_null_pointer_v<T>) &&
                                              !std::is_same_v<T, const char*>>::type> {
    public:
        Asserter(T value, std::string_view file, size_t line) : m_value(value), m_file(file), m_line(line) { }

        void is_equal(const T& other) {
            if (m_value != other)
                throw FailedAssertion(to_hex_string(m_value) + " != " + to_hex_string(other), m_file, m_line);
        }

        void is_not_equal(const T& other) {
            if (m_value == other)
                throw FailedAssertion(to_hex_string(m_value) + " == " + to_hex_string(other), m_file, m_line);
        }

        void is_null() {
            if (m_value != nullptr)
                throw FailedAssertion(to_hex_string(m_value) + " != nullptr", m_file, m_line);
        }

        void is_not_null() {
            if (m_value == nullptr)
                throw FailedAssertion(to_hex_string(m_value) + " == nullptr", m_file, m_line);
        }

    private:
        T m_value;

        std::string m_file;
        size_t      m_line;
    };

    template <typename T>
    class Asserter<T, typename std::enable_if<std::is_same_v<T, std::string> ||
                                              std::is_same_v<T, const char*>>::type>
    {
    public:
        Asserter(std::string_view value, std::string_view file, size_t line) :
            m_value(value.data(), value.size()), m_file(file), m_line(line) { }

        void is_equal(const std::string& other) {
            if (m_value != other)
                throw FailedAssertion(m_value + " != " + other, m_file, m_line);
        }

        void is_not_equal(const std::string& other) {
            if (m_value == other)
                throw FailedAssertion(m_value + " == " + other, m_file, m_line);
        }

    private:
        std::string m_value;

        std::string m_file;
        size_t      m_line;
    };

    template <typename T>
    static Asserter<T> that(T value, std::string_view file, size_t line) { return Asserter<T>(value, file, line); }
    #define that(value) that(value, __FILE__, __LINE__)
};
