// SQLBuilderVisitorTest.cpp

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
    std::string expected_sql = "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, MultipleConditionsAnd) {
    std::string filter = "age > 30 AND salary <= 50000";
    std::string expected_sql = 
        "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'salary' AND vm1.value <= 50000))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, MultipleConditionsOr) {
    std::string filter = "age > 30 OR salary <= 50000";
    std::string expected_sql = 
        "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) OR "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'salary' AND vm1.value <= 50000))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, CombinedAndOr) {
    std::string filter = "age > 30 AND salary <= 50000 OR department == 'HR'";
    std::string expected_sql = 
        "(("
        "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'salary' AND vm1.value <= 50000)"
        ") OR "
        "EXISTS (SELECT 1 FROM VectorMetadata vm2 WHERE vm2.vectorId = VectorMetadata.vectorId AND vm2.key = 'department' AND vm2.value = 'HR'))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, StringLike) {
    std::string filter = "name LIKE 'John%'";
    std::string expected_sql = "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'name' AND vm0.value LIKE 'John%')";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, Parentheses) {
    std::string filter = "(age > 30 AND salary <= 50000) OR department == 'HR'";
    std::string expected_sql = 
        "(("
        "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'salary' AND vm1.value <= 50000)"
        ") OR "
        "EXISTS (SELECT 1 FROM VectorMetadata vm2 WHERE vm2.vectorId = VectorMetadata.vectorId AND vm2.key = 'department' AND vm2.value = 'HR'))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, BooleanValues) {
    std::string filter = "is_active == true AND is_manager == false";
    std::string expected_sql = 
        "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'is_active' AND vm0.value = 1) AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'is_manager' AND vm1.value = 0))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, ComplexExpression) {
    std::string filter = "((age > 25 AND department == 'Engineering') OR (age > 30 AND department == 'HR')) AND is_active == true";
    std::string expected_sql = 
        "(("
        "("
        "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 25) AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'department' AND vm1.value = 'Engineering')"
        ") OR "
        "("
        "EXISTS (SELECT 1 FROM VectorMetadata vm2 WHERE vm2.vectorId = VectorMetadata.vectorId AND vm2.key = 'age' AND vm2.value > 30) AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm3 WHERE vm3.vectorId = VectorMetadata.vectorId AND vm3.key = 'department' AND vm3.value = 'HR')"
        ")"
        ") AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm4 WHERE vm4.vectorId = VectorMetadata.vectorId AND vm4.key = 'is_active' AND vm4.value = 1))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, NotCondition) {
    std::string filter = "NOT is_active == true";
    std::string expected_sql = "(NOT EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'is_active' AND vm0.value = 1))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, NotCombinedWithAnd) {
    std::string filter = "age > 30 AND NOT is_active == true";
    std::string expected_sql = 
        "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) AND "
        "(NOT EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'is_active' AND vm1.value = 1)))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, NotCombinedWithOr) {
    std::string filter = "age > 30 OR NOT is_active == true";
    std::string expected_sql = 
        "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) OR "
        "(NOT EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'is_active' AND vm1.value = 1)))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, InCondition) {
    std::string filter = "key2 IN (1, 2, 3)";
    std::string expected_sql = "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'key2' AND vm0.value IN (1, 2, 3))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, CombinedLikeAndIn) {
    std::string filter = "metakey LIKE '%data%' AND key2 IN (1, 2, 3)";
    std::string expected_sql = 
        "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'metakey' AND vm0.value LIKE '%data%') AND "
        "EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'key2' AND vm1.value IN (1, 2, 3)))";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}

TEST(SQLBuilderVisitorTest, GreaterThanCondition) {
    std::string filter = "key3 > 1000";
    std::string expected_sql = "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'key3' AND vm0.value > 1000)";
    std::string actual_sql = convertFilterToSQL(filter);
    EXPECT_EQ(actual_sql, expected_sql);
}
