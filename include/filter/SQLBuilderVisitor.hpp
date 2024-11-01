#ifndef __ATINYVECTORS_SQL_BUILDER_VISITOR_HPP__
#define __ATINYVECTORS_SQL_BUILDER_VISITOR_HPP__

#include "PlanBaseVisitor.h"
#include "PlanParser.h"
#include <string>
#include <stack>
#include <algorithm>
#include <sstream>

namespace atinyvectors {
namespace filter {

class SQLBuilderVisitor : public PlanBaseVisitor {
public:
    SQLBuilderVisitor();

    std::string getSQL() const;

    virtual std::any visitInteger(PlanParser::IntegerContext* ctx) override;
    virtual std::any visitFloating(PlanParser::FloatingContext* ctx) override;
    virtual std::any visitBoolean(PlanParser::BooleanContext* ctx) override;
    virtual std::any visitString(PlanParser::StringContext* ctx) override;
    virtual std::any visitParens(PlanParser::ParensContext* ctx) override;
    virtual std::any visitLike(PlanParser::LikeContext* ctx) override;
    virtual std::any visitAddSub(PlanParser::AddSubContext* ctx) override;
    virtual std::any visitEquality(PlanParser::EqualityContext* ctx) override;
    virtual std::any visitLogicalAnd(PlanParser::LogicalAndContext* ctx) override;
    virtual std::any visitLogicalOr(PlanParser::LogicalOrContext* ctx) override;
    virtual std::any visitRelational(PlanParser::RelationalContext* ctx) override;
    virtual std::any visitUnary(PlanParser::UnaryContext* ctx) override;
    virtual std::any visitMulDivMod(PlanParser::MulDivModContext* ctx) override;
    virtual std::any visitIdentifier(PlanParser::IdentifierContext* ctx) override;
    virtual std::any visitJSONIdentifier(PlanParser::JSONIdentifierContext* ctx) override;
    virtual std::any visitTerm(PlanParser::TermContext* ctx) override;
    virtual std::any visitEmptyTerm(PlanParser::EmptyTermContext* ctx) override;

private:
    std::string sql_expression;

    int conditionCount;

    std::string handleKeyValueCondition(const std::string& key, const std::string& condition);

    bool isIdentifier(antlr4::ParserRuleContext* ctx);
    std::string getIdentifier(antlr4::ParserRuleContext* ctx);
    std::string quoteString(const std::string& str);
    std::string generateValueCondition(const std::string& op, PlanParser::ExprContext* ctx);

    std::string extractIdentifier(const std::string& expr);
};

};
};
#endif
