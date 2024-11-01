#include "filter/SQLBuilderVisitor.hpp"
#include "spdlog/spdlog.h"
#include <sstream>
#include <regex>

namespace atinyvectors
{
namespace filter
{

SQLBuilderVisitor::SQLBuilderVisitor() : conditionCount(0) {
}

std::string SQLBuilderVisitor::getSQL() const {
    spdlog::debug("Getting SQL expression: {}", sql_expression);
    return sql_expression;
}

bool SQLBuilderVisitor::isIdentifier(antlr4::ParserRuleContext* ctx) {
    return dynamic_cast<PlanParser::IdentifierContext*>(ctx) != nullptr;
}

std::string SQLBuilderVisitor::getIdentifier(antlr4::ParserRuleContext* ctx) {
    if (auto idCtx = dynamic_cast<PlanParser::IdentifierContext*>(ctx)) {
        return idCtx->getText();
    }
    return "";
}

std::string SQLBuilderVisitor::quoteString(const std::string& str) {
    std::string escaped = "'";
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped += c;
        }
    }
    escaped += "'";
    return escaped;
}

std::string SQLBuilderVisitor::generateValueCondition(const std::string& op, PlanParser::ExprContext* ctx) {
    return op + " " + std::any_cast<std::string>(visit(ctx));
}

std::string SQLBuilderVisitor::handleKeyValueCondition(const std::string& key, const std::string& condition) {
    std::stringstream ss;
    ss << "EXISTS (SELECT 1 FROM VectorMetadata vm" << conditionCount 
       << " WHERE vm" << conditionCount << ".vectorId = VectorMetadata.vectorId "
       << "AND vm" << conditionCount << ".key = " << quoteString(key) 
       << " AND vm" << conditionCount << ".value " << condition << ")";
    conditionCount++;
    return ss.str();
}

std::any SQLBuilderVisitor::visitInteger(PlanParser::IntegerContext* ctx) {
    std::string value = ctx->getText();
    spdlog::debug("Visited Integer: {}", value);
    return std::any(value);
}

std::any SQLBuilderVisitor::visitFloating(PlanParser::FloatingContext* ctx) {
    std::string value = ctx->getText();
    spdlog::debug("Visited Floating: {}", value);
    return std::any(value);
}

std::any SQLBuilderVisitor::visitBoolean(PlanParser::BooleanContext* ctx) {
    std::string value = ctx->getText();
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    if (value == "true" || value == "false") {
        std::string boolean_value = (value == "true") ? "1" : "0";
        spdlog::debug("Visited Boolean constant: {}", boolean_value);
        return std::any(boolean_value);
    }

    std::string expr = "\"" + ctx->getText() + "\"";
    spdlog::debug("Visited Boolean variable: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitString(PlanParser::StringContext* ctx) {
    std::string value = ctx->getText();
    std::string escaped = "'";
    for (size_t i = 1; i < value.size() - 1; ++i) {
        if (value[i] == '\'') {
            escaped += "''";
        } else {
            escaped += value[i];
        }
    }
    escaped += "'";
    spdlog::debug("Visited String: {}", escaped);
    return std::any(escaped);
}

std::any SQLBuilderVisitor::visitParens(PlanParser::ParensContext* ctx) {
    std::string expr = std::any_cast<std::string>(visit(ctx->expr()));

    if (expr.front() != '(' || expr.back() != ')') {
        expr = "(" + expr + ")";
    }
    spdlog::debug("Visited Parens: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitLike(PlanParser::LikeContext* ctx) {
    if (isIdentifier(ctx->expr())) {
        std::string key = getIdentifier(ctx->expr());
        std::string pattern = ctx->StringLiteral()->getText();
        std::string condition = "LIKE " + pattern;
        std::string expr = handleKeyValueCondition(key, condition);
        spdlog::debug("Visited Like: {}", expr);
        return std::any(expr);
    } else {
        std::string field = std::any_cast<std::string>(visit(ctx->expr()));
        std::string pattern = ctx->StringLiteral()->getText();
        std::string expr = field + " LIKE " + pattern;
        spdlog::debug("Visited Like: {}", expr);
        return std::any(expr);
    }
}

std::any SQLBuilderVisitor::visitAddSub(PlanParser::AddSubContext* ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expr(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expr(1)));
    std::string op = ctx->op->getText();
    std::string expr = left + " " + op + " " + right;
    spdlog::debug("Visited AddSub: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitEquality(PlanParser::EqualityContext* ctx) {
    if (isIdentifier(ctx->expr(0))) {
        std::string key = getIdentifier(ctx->expr(0));
        std::string op = ctx->op->getText();
        if (op == "==") {
            op = "=";
        } else if (op == "!=") {
            op = "<>";
        }
        std::string value_condition = generateValueCondition(op, ctx->expr(1));
        std::string expr = handleKeyValueCondition(key, value_condition);
        spdlog::debug("Visited Equality: {}", expr);
        return std::any(expr);
    } else {
        std::string left = std::any_cast<std::string>(visit(ctx->expr(0)));
        std::string right = std::any_cast<std::string>(visit(ctx->expr(1)));
        std::string op = ctx->op->getText();

        if (op == "==") {
            op = "=";
        } else if (op == "!=") {
            op = "<>";
        }

        std::string expr = left + " " + op + " " + right;
        spdlog::debug("Visited Equality: {}", expr);
        return std::any(expr);
    }
}

std::any SQLBuilderVisitor::visitLogicalAnd(PlanParser::LogicalAndContext* ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expr(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expr(1)));
    std::string expr = "(" + left + " AND " + right + ")";
    spdlog::debug("Visited LogicalAnd: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitLogicalOr(PlanParser::LogicalOrContext* ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expr(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expr(1)));

    std::string expr = "(" + left + " OR " + right + ")";
    spdlog::debug("Visited LogicalOr: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitRelational(PlanParser::RelationalContext* ctx) {
    if (isIdentifier(ctx->expr(0))) {
        std::string key = getIdentifier(ctx->expr(0));
        std::string op = ctx->op->getText();
        if (op == "==") {
            op = "=";
        }
        std::string value_condition = generateValueCondition(op, ctx->expr(1));
        std::string expr = handleKeyValueCondition(key, value_condition);
        spdlog::debug("Visited Relational: {}", expr);
        return std::any(expr);
    } else {
        std::string left = std::any_cast<std::string>(visit(ctx->expr(0)));
        std::string right = std::any_cast<std::string>(visit(ctx->expr(1)));
        std::string op = ctx->op->getText();
        if (op == "==") {
            op = "=";
        }
        std::string expr = left + " " + op + " " + right;
        spdlog::debug("Visited Relational: {}", expr);
        return std::any(expr);
    }
}

std::any SQLBuilderVisitor::visitUnary(PlanParser::UnaryContext* ctx) {
    std::string op = ctx->op->getText();

    std::string expr;
    try {
        expr = std::any_cast<std::string>(visit(ctx->expr()));
    } catch (const std::bad_any_cast& e) {
        spdlog::error("Failed to cast expression in visitUnary: {}", e.what());
        return std::any("");
    }

    if (op == "NOT" || op == "!") {
        expr = "(NOT " + expr + ")";
    } else {
        expr = op + expr;
    }

    spdlog::debug("Visited Unary: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitMulDivMod(PlanParser::MulDivModContext* ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expr(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expr(1)));
    std::string op = ctx->op->getText();
    std::string expr = left + " " + op + " " + right;
    spdlog::debug("Visited MulDivMod: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitIdentifier(PlanParser::IdentifierContext* ctx) {
    std::string id = ctx->getText();
    std::string expr = "\"" + id + "\"";
    spdlog::debug("Visited Identifier: {}", expr);
    return std::any(expr);
}

std::any SQLBuilderVisitor::visitJSONIdentifier(PlanParser::JSONIdentifierContext* ctx) {
    std::string json_id = ctx->getText();
    spdlog::debug("Visited JSONIdentifier: {}", json_id);
    return std::any(json_id);
}

std::any SQLBuilderVisitor::visitTerm(PlanParser::TermContext* ctx) {
    std::string op = ctx->op->getText();
    std::string key = getIdentifier(ctx->expr(0));
    std::string in_values;

    for (size_t i = 1; i < ctx->expr().size(); ++i) {
        if (i > 1) {
            in_values += ", ";
        }
        try {
            in_values += std::any_cast<std::string>(visit(ctx->expr(i)));
        } catch (const std::bad_any_cast&) {
            spdlog::error("Failed to cast expression in visitTerm");
            in_values += "''";
        }
    }

    // "value" 제외하고 단순히 IN (...) 형식만 추가
    std::string condition = op + " (" + in_values + ")";
    std::string expr = handleKeyValueCondition(key, condition);
    spdlog::debug("Visited Term ({}): {}", op, expr);
    return std::any(expr);
}


std::any SQLBuilderVisitor::visitEmptyTerm(PlanParser::EmptyTermContext* ctx) {
    std::string op = ctx->op->getText();
    std::string key = getIdentifier(ctx->expr());

    if (op == "IN" || op == "in") {
        std::string condition = "\"value\" IS NOT NULL";
        std::string expr = handleKeyValueCondition(key, condition);
        spdlog::debug("Visited EmptyTerm (IN): {}", expr);
        return std::any(expr);
    } else if (op == "NIN" || op == "not in" || op == "NOT IN") {
        std::string condition = "\"value\" IS NULL";
        std::string expr = handleKeyValueCondition(key, condition);
        spdlog::debug("Visited EmptyTerm (NOT IN): {}", expr);
        return std::any(expr);
    } else {
        spdlog::error("Unsupported operator in visitEmptyTerm: {}", op);
        return std::any("");
    }
}

}; // namespace filter
}; // namespace atinyvectors
