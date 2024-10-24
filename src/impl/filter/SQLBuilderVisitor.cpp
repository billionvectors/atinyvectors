#include "filter/SQLBuilderVisitor.hpp"

namespace atinyvectors
{
namespace filter
{

SQLBuilderVisitor::SQLBuilderVisitor() {}

std::string SQLBuilderVisitor::getSQL() const {
    if (!sql_stack.empty()) {
        return sql_stack.top();
    }
    return "";
}

std::any SQLBuilderVisitor::visitInteger(PlanParser::IntegerContext* ctx) {
    std::string value = ctx->getText();
    sql_stack.push(value);
    return nullptr;
}

std::any SQLBuilderVisitor::visitFloating(PlanParser::FloatingContext* ctx) {
    std::string value = ctx->getText();
    sql_stack.push(value);
    return nullptr;
}

std::any SQLBuilderVisitor::visitBoolean(PlanParser::BooleanContext* ctx) {
    std::string value = ctx->getText();
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    if (value == "true") {
        sql_stack.push("1");
    } else {
        sql_stack.push("0");
    }
    return nullptr;
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
    sql_stack.push(escaped);
    return nullptr;
}

std::any SQLBuilderVisitor::visitParens(PlanParser::ParensContext* ctx) {
    visit(ctx->expr());
    std::string expr = sql_stack.top();
    sql_stack.pop();
    expr = "(" + expr + ")";
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitLike(PlanParser::LikeContext* ctx) {
    visit(ctx->expr());
    visit(ctx->StringLiteral());

    std::string pattern = sql_stack.top();
    sql_stack.pop();
    std::string field = sql_stack.top();
    sql_stack.pop();
    std::string expr = field + " LIKE " + pattern;
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitAddSub(PlanParser::AddSubContext* ctx) {
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    std::string right = sql_stack.top();
    sql_stack.pop();
    std::string left = sql_stack.top();
    sql_stack.pop();
    std::string op = ctx->op->getText();
    std::string expr = left + " " + op + " " + right;
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitLogicalAnd(PlanParser::LogicalAndContext* ctx) {
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    std::string right = sql_stack.top();
    sql_stack.pop();
    std::string left = sql_stack.top();
    sql_stack.pop();
    std::string expr = "(" + left + " AND " + right + ")";
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitLogicalOr(PlanParser::LogicalOrContext* ctx) {
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    std::string right = sql_stack.top();
    sql_stack.pop();
    std::string left = sql_stack.top();
    sql_stack.pop();
    std::string expr = "(" + left + " OR " + right + ")";
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitRelational(PlanParser::RelationalContext* ctx) {
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    std::string right = sql_stack.top();
    sql_stack.pop();
    std::string left = sql_stack.top();
    sql_stack.pop();
    std::string op;
    switch (ctx->op->getType()) {
        case PlanParser::LT:
            op = "<";
            break;
        case PlanParser::LE:
            op = "<=";
            break;
        case PlanParser::GT:
            op = ">";
            break;
        case PlanParser::GE:
            op = ">=";
            break;
        case PlanParser::EQ:
            op = "=";
            break;
        case PlanParser::NE:
            op = "!=";
            break;
        default:
            op = ctx->op->getText();
    }
    std::string expr = left + " " + op + " " + right;
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitIdentifier(PlanParser::IdentifierContext* ctx) {
    std::string id = ctx->getText();

    std::string expr = "\"" + id + "\"";
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitMulDivMod(PlanParser::MulDivModContext* ctx) {
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    std::string right = sql_stack.top();
    sql_stack.pop();
    std::string left = sql_stack.top();
    sql_stack.pop();
    std::string op;
    switch (ctx->op->getType()) {
        case PlanParser::MUL:
            op = "*";
            break;
        case PlanParser::DIV:
            op = "/";
            break;
        case PlanParser::MOD:
            op = "%";
            break;
        default:
            op = ctx->op->getText();
    }
    std::string expr = left + " " + op + " " + right;
    sql_stack.push(expr);
    return nullptr;
}

std::any SQLBuilderVisitor::visitEquality(PlanParser::EqualityContext* ctx) {
    visit(ctx->expr(0));
    visit(ctx->expr(1));
    std::string right = sql_stack.top();
    sql_stack.pop();
    std::string left = sql_stack.top();
    sql_stack.pop();
    std::string op = ctx->op->getText();
    if (op == "==") {
        op = "=";
    }
    std::string expr = left + " " + op + " " + right;
    sql_stack.push(expr);
    return nullptr;
}

std::string SQLBuilderVisitor::extractIdentifier(const std::string& expr) {
    if (expr.front() == '"' && expr.back() == '"') {
        return expr.substr(1, expr.size() - 2);
    }
    return expr;
}

};
};