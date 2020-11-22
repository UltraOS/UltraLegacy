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
#include <unordered_map>
#include <vector>

template <size_t Size>
constexpr std::string_view deduce_test_subject(const char(&file_name)[Size])
{
    int offset_to_dot = static_cast<int>(Size) - 1;
    for (; offset_to_dot > 0; --offset_to_dot) {
        if (file_name[offset_to_dot] == '.')
            break;
    }

    auto offset_to_start = offset_to_dot;

    for (; offset_to_start > 0; --offset_to_start) {
        if (file_name[offset_to_start] == '\\' || file_name[offset_to_start] == '/')
            break;
    }

    return { file_name + offset_to_start + 4 + 1, static_cast<size_t>(offset_to_dot) - offset_to_start - 4 - 1 };
}

class Test
{
public:
    virtual std::string_view name() const = 0;

    virtual void run() = 0;

protected:
    template <size_t Size>
    Test(const char(&file_name)[Size]);
};

class Fixture
{
public:
    virtual std::string_view name() const = 0;

    virtual void run() = 0;

protected:
    template <size_t Size>
    Fixture(const char(&file_name)[Size]);
};

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#define TEST(case_name)                                              \
    namespace {                                                      \
    class Test##case_name : public Test {                            \
    public:                                                          \
        Test##case_name() : Test(__FILE__) { }                       \
        std::string_view name() const override {                     \
            return TO_STRING(case_name);                             \
        }                                                            \
        void run() override;                                         \
    } static test_##case_name;                                       \
    }                                                                \
    void Test##case_name::run()

#define FIXTURE(fname)                                               \
    namespace {                                                      \
    class Fixture##fname : public Fixture {                          \
    public:                                                          \
        Fixture##fname() : Fixture(__FILE__) { }                     \
        std::string_view name() const override {                     \
            return TO_STRING(fname);                                 \
        }                                                            \
        void run() override;                                         \
    } static fixture_##fname;                                        \
    }                                                                \
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
    static int run(int argc, char** argv)
    {
        if (argc > 1) {
            if (argc != 2) {
                std::cout << "Usage: " << argv[0] << " <test_subject>" << std::endl;
                return -1;
            }

            std::string_view test_to_run = argv[1];

            if (s_tests.count(test_to_run)) {
                run_tests_for(argv[1]);
            } else if (test_to_run == "all") {
                run_all();
            }
        } else {
            run_all();
        }

        std::cout << std::endl << "Results => "
                  << s_pass_count << " passed / "
                  << s_fail_count << " failed (" << s_pass_count << "/"
                  << s_ran_count << ")" << std::endl;

        return static_cast<int>(s_fail_count);
    }

private:
    static void run_all()
    {
        for (auto& subject : s_tests) {
            run_tests_for(subject.first);
        }
    }

    static void run_tests_for(std::string_view subject_name)
    {
        auto& tests = s_tests[subject_name];

        std::cout << "Running tests for \"" << subject_name << "\" (" << tests.size() << " tests)" << std::endl;

        if (s_fixtures.count(subject_name)) {
            for (auto fixture : s_fixtures[subject_name]) {
                try {
                    std::cout << "---- Running fixture \"" << fixture->name() << "\"... ";
                    fixture->run();
                    std::cout << "\u001b[32mOK\u001b[0m" << std::endl;
                }
                catch (const FailedAssertion& ex) {
                    std::cout << "\u001b[31mFAILED!\u001b[0m " << build_error_message(ex) << std::endl;
                    std::cout << "Terminating..." << std::endl;
                    exit(-1);
                }
                catch (const std::exception& ex) {
                    std::cout << "\u001b[31mFAILED!\u001b[0m " << ex.what() << std::endl;
                    std::cout << "Terminating..." << std::endl;
                    exit(-1);
                }
            }
        }

        for (auto test : tests) {
            std::cout << "---- Running test \"" << test->name() << "\"... ";

            std::string failure_reason;

            try {
                test->run();
            }
            catch (const FailedAssertion& ex) {
                failure_reason = build_error_message(ex);
            }
            catch (const std::exception& ex) {
                failure_reason = ex.what();
            }

            if (failure_reason.empty()) {
                std::cout << "\u001b[32mPASSED!\u001b[0m" << std::endl;
                ++s_pass_count;
            }
            else {
                std::cout << "\u001b[31mFAILED!\u001b[0m" << " (" << failure_reason << ")" << std::endl;
                ++s_fail_count;
            }

            s_ran_count++;
        }

        std::cout << std::endl;
    }

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

    static void register_self(Test* self, std::string_view subject)
    {
        s_tests[subject].push_back(self);
    }

    static void register_self(Fixture* self, std::string_view subject)
    {
        s_fixtures[subject].push_back(self);
    }

private:
    inline static std::unordered_map<std::string_view, std::vector<Test*>> s_tests;
    inline static std::unordered_map<std::string_view, std::vector<Fixture*>> s_fixtures;
    inline static size_t s_ran_count;
    inline static size_t s_pass_count;
    inline static size_t s_fail_count;
};

template <size_t Size>
Test::Test(const char(&file_name)[Size])
{
    TestRunner::register_self(this, deduce_test_subject(file_name));
}

template <size_t Size>
Fixture::Fixture(const char(&file_name)[Size])
{
    TestRunner::register_self(this, deduce_test_subject(file_name));
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
    class Asserter<T, typename std::enable_if<std::is_same_v<T, bool>>::type>
    {
    public:
        Asserter(bool value, std::string_view file, size_t line) :
            m_value(value), m_file(file), m_line(line) { }

        void is_true() {
            if (m_value == false)
                throw FailedAssertion("false != true", m_file, m_line);
        }

        void is_false() {
            if (m_value == true)
                throw FailedAssertion("true != false", m_file, m_line);
        }

    private:
        bool m_value;

        std::string m_file;
        size_t      m_line;
    };

    template <typename T>
    static Asserter<T> that(T value, std::string_view file, size_t line) { return Asserter<T>(value, file, line); }
    #define that(value) that(value, __FILE__, __LINE__)
};

class Deletable {
public:
    Deletable(size_t& counter) : m_counter(counter) {}
    ~Deletable() { m_counter++; }

private:
    size_t& m_counter;
};
