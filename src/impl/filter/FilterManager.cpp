#include "filter/FilterManager.hpp"
#include "filter/SQLBuilderVisitor.hpp"
#include "antlr4-runtime.h"
#include "PlanLexer.h"
#include "PlanParser.h"
#include <iostream>

using namespace atinyvectors::filter;

std::unique_ptr<FilterManager> FilterManager::instance = nullptr;
std::mutex FilterManager::instanceMutex;

FilterManager::FilterManager() = default;

std::string FilterManager::convertFilterToSQL(const std::string& filter) {
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

FilterManager& FilterManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance = std::unique_ptr<FilterManager>(new FilterManager());
    }
    return *instance;
}

std::string FilterManager::toSQL(const std::string& filter) {
    return convertFilterToSQL(filter);
}
