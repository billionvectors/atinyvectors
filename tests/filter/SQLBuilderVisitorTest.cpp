// tests/test_sql_builder.cpp

#include "gtest/gtest.h"
#include "PlanLexer.h"
#include "PlanParser.h"
#include "filter/SQLBuilderVisitor.hpp"
#include <antlr4-runtime.h>
#include "spdlog/spdlog.h"

using namespace atinyvectors;
using namespace atinyvectors::filter;

class SQLBuilderVisitorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

std::string convertFilterToSQL(const std::string& filter) {
    antlr4::ANTLRInputStream input(filter);
    PlanLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    PlanParser parser(&tokens);

    PlanParser::ExprContext* tree = parser.expr();

    SQLBuilderVisitor visitor;
    std::any result = visitor.visit(tree);

    if (result.has_value()) {
        try {
            return std::any_cast<std::string>(result);
        } catch (const std::bad_any_cast& e) {
            std::cerr << "Error casting any to string: " << e.what() << std::endl;
            return "";
        }
    }
    return "";
}

TEST(SQLBuilderVisitorTest, IntegerComparison) {
    std::string filter = "age > 30";
    std::string expected_sql = "\"age\" > 30";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, MultipleConditionsAnd) {
    std::string filter = "age > 30 AND salary <= 50000";
    std::string expected_sql = "(\"age\" > 30 AND \"salary\" <= 50000)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, MultipleConditionsOr) {
    std::string filter = "age > 30 OR salary <= 50000";
    std::string expected_sql = "(\"age\" > 30 OR \"salary\" <= 50000)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, CombinedAndOr) {
    std::string filter = "age > 30 AND salary <= 50000 OR department == 'HR'";
    std::string expected_sql = "((\"age\" > 30 AND \"salary\" <= 50000) OR \"department\" = 'HR')";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, StringLike) {
    std::string filter = "name LIKE 'John%'";
    std::string expected_sql = "\"name\" LIKE 'John%'";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, Parentheses) {
    std::string filter = "(age > 30 AND salary <= 50000) OR department == 'HR'";
    std::string expected_sql = "((\"age\" > 30 AND \"salary\" <= 50000) OR \"department\" = 'HR')";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, BooleanValues) {
    std::string filter = "is_active == true AND is_manager == false";
    std::string expected_sql = "(\"is_active\" = 1 AND \"is_manager\" = 0)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, ArithmeticOperations) {
    std::string filter = "salary + bonus > 60000";
    std::string expected_sql = "\"salary\" + \"bonus\" > 60000";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, ComplexExpression) {
    std::string filter = "((age > 25 AND department == 'Engineering') OR (age > 30 AND department == 'HR')) AND is_active == true";
    std::string expected_sql = "(((\"age\" > 25 AND \"department\" = 'Engineering') OR (\"age\" > 30 AND \"department\" = 'HR')) AND \"is_active\" = 1)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, NotCondition) {
    std::string filter = "NOT is_active == true";
    std::string expected_sql = "(NOT \"is_active\" = 1)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, NotCombinedWithAnd) {
    std::string filter = "age > 30 AND NOT is_active == true";
    std::string expected_sql = "(\"age\" > 30 AND (NOT \"is_active\" = 1))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, NotCombinedWithOr) {
    std::string filter = "age > 30 OR NOT is_active == true";
    std::string expected_sql = "(\"age\" > 30 OR (NOT \"is_active\" = 1))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}