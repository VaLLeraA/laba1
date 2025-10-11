#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define EXECUTABLE_EXTENSION ".exe"
#else
#define EXECUTABLE_EXTENSION ""
#endif

namespace fs = std::filesystem;

static void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
            s.end());
}

struct TestCase {
    std::string name;
    std::string templateFile;
    std::string dataFile;
    std::string expectedFile;
    bool isNegativeTest;

    TestCase(const std::string &testName, bool isNegative = false)
        : name(testName), templateFile(fs::absolute(fs::path("test_cases") / testName / "template.txt").string()), dataFile(fs::absolute(fs::path("test_cases") / testName / "data.dat").string()), expectedFile(fs::absolute(fs::path("test_cases") / testName / "expected.txt").string()), isNegativeTest(isNegative) {}
};

std::ostream &operator<<(std::ostream &os, const TestCase &test_case) {
    return os << fmt::format(
                   "TestCase(name: {}, template: {}, data: {}, expected: {}, isNegative: {})",
                   test_case.name, test_case.templateFile, test_case.dataFile, test_case.expectedFile,
                   test_case.isNegativeTest);
}

class GeneratorTest : public ::testing::TestWithParam<TestCase> {
protected:
    void TearDown() override {
        std::remove("test_output.txt");
    }

    std::string ReadFile(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    int RunGenerator(const std::string &args) {
        std::string exec_path = fs::absolute(fs::path("..") / ("first_lab" + std::string(EXECUTABLE_EXTENSION))).string();
        std::string command = exec_path + " " + args;

        int result = std::system(command.c_str());

        return result;
    }
};

TEST_P(GeneratorTest, ProcessTestCase) {
    const TestCase &test_case = GetParam();

    std::string args = fmt::format("--template=\"{}\" --data=\"{}\" --output=test_output.txt",
                                   test_case.templateFile, test_case.dataFile);

    int result = RunGenerator(args);

    if (test_case.isNegativeTest) {
        EXPECT_NE(result, 0) << "Negative test should fail: " << test_case.name;
    } else {
        EXPECT_EQ(result, 0) << "Positive test should succeed: " << test_case.name;

        std::string output = ReadFile("test_output.txt");
        std::string expected = ReadFile(test_case.expectedFile);

        EXPECT_FALSE(expected.empty()) << "Expected result file is empty for: " << test_case.name;

        rtrim(output);
        rtrim(expected);

        EXPECT_EQ(output, expected) << "Output doesn't match expected result for: " << test_case.name;
    }
}


void DiscoverTestCasesRecursive(const fs::path &base_path, std::vector<TestCase> &test_cases,
                                const std::string &prefix, bool require_expected, const fs::path &root_path = fs::path()) {
    fs::path actual_root_path = root_path.empty() ? base_path : root_path;

    for (const auto &entry: fs::directory_iterator(base_path)) {
        if (entry.is_directory()) {
            fs::path current_dir = entry.path();

            fs::path relative_path = fs::relative(current_dir, actual_root_path);
            std::string test_case_name = prefix + "/" + relative_path.string();

            bool is_negative = (prefix == "negative");
            test_cases.emplace_back(test_case_name, is_negative);

            DiscoverTestCasesRecursive(current_dir, test_cases, prefix, require_expected, actual_root_path);
        }
    }
}

std::vector<TestCase> DiscoverTestCases(const std::string &test_type, bool require_expected) {
    std::vector<TestCase> test_cases;

    fs::path test_path = fs::path("test_cases") / test_type;
    if (!fs::exists(test_path) || !fs::is_directory(test_path)) {
        return test_cases;
    }

    DiscoverTestCasesRecursive(test_path, test_cases, test_type, require_expected);

    std::sort(test_cases.begin(), test_cases.end(),
              [](const TestCase &a, const TestCase &b) {
                  return a.name < b.name;
              });

    return test_cases;
}


INSTANTIATE_TEST_SUITE_P(
        PositiveTests,
        GeneratorTest,
        ::testing::ValuesIn(DiscoverTestCases("positive", true)),
        [](const ::testing::TestParamInfo<TestCase> &info) {
            std::string test_name = info.param.name;
            for (auto &ch: test_name) {
                if (ch == '/' || ch == ' ' || ch == '-')
                    ch = '_';
            }
            test_name.erase(std::remove_if(test_name.begin(), test_name.end(),
                                           [](char c) {
                                               return !std::isalnum(static_cast<unsigned char>(c)) && c != '_';
                                           }),
                            test_name.end());
            return test_name;
        });

INSTANTIATE_TEST_SUITE_P(
        NegativeTests,
        GeneratorTest,
        ::testing::ValuesIn(DiscoverTestCases("negative", false)),
        [](const ::testing::TestParamInfo<TestCase> &info) {
            std::string test_name = info.param.name;
            for (auto &ch: test_name) {
                if (ch == '/' || ch == ' ' || ch == '-')
                    ch = '_';
            }
            test_name.erase(std::remove_if(test_name.begin(), test_name.end(),
                                           [](char c) {
                                               return !std::isalnum(static_cast<unsigned char>(c)) && c != '_';
                                           }),
                            test_name.end());
            return test_name;
        });
