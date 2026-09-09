#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "lexer.h"
#include "parser.h"
#include "checker.h"
#include "graph.h"

namespace {

int passed = 0;
int failed = 0;

struct GraphResult {
    std::vector<Violation> violations;
    std::string dot;
};

GraphResult graph(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    Program program = parser.parse();
    Checker checker(program);
    GraphResult result;
    result.violations = checker.check();
    result.dot = DependencyGraphExporter::toDot(program, result.violations);
    return result;
}

void assertTrue(bool condition, const std::string& name) {
    if (condition) {
        std::cout << "[PASS] " << name << "\n";
        passed++;
        return;
    }

    std::cerr << "[FAIL] " << name << "\n";
    failed++;
    assert(condition);
}

bool contains(const std::string& text, const std::string& expected) {
    return text.find(expected) != std::string::npos;
}

void assertTestCount(int expected) {
    if (passed != expected) {
        std::cerr << "[FAIL] expected " << expected << " graph assertions, got "
                  << passed << "\n";
        failed++;
        assert(passed == expected);
    }
}

} // namespace

int main() {
    const auto valid = graph(
        "module data\n"
        "module service depends data\n"
        "module ui depends service\n"
        "layer ui above service\n"
        "layer service above data\n");
    assertTrue(valid.violations.empty(), "exports structurally valid program");
    assertTrue(contains(valid.dot, "\"service\" -> \"data\" [label=\"depends\", color=\"#2563eb\"]"),
               "renders a normal dependency edge");
    assertTrue(contains(valid.dot, "\"ui\" -> \"service\" [label=\"above\", style=\"dashed\""),
               "renders a layer edge separately");

    const auto reordered = graph(
        "module ui depends service\n"
        "module service depends data\n"
        "module data\n"
        "layer service above data\n"
        "layer ui above service\n");
    assertTrue(valid.dot == reordered.dot,
               "sorts nodes and edges independently from declaration order");
    assertTrue(valid.dot == DependencyGraphExporter::toDot(
                                Program{
                                    {
                                        {"ui", {"service"}, false, {}, {}, 1},
                                        {"service", {"data"}, false, {}, {}, 2},
                                        {"data", {}, false, {}, {}, 3}
                                    },
                                    {
                                        {"service", "data", 4},
                                        {"ui", "service", 5}
                                    }
                                },
                                reordered.violations),
               "produces deterministic DOT output");

    const auto missing = graph("module ui depends notification\n");
    assertTrue(contains(missing.dot, "notification\\n(undefined)"),
               "renders undefined dependency target");
    assertTrue(contains(missing.dot, "\"ui\" -> \"notification\" [label=\"depends\", color=\"#dc2626\", penwidth=2.5]"),
               "highlights undefined dependency edge");

    const auto cyclic = graph(
        "module order depends payment\n"
        "module payment depends order\n");
    assertTrue(contains(cyclic.dot, "\"order\" -> \"payment\" [label=\"depends\", color=\"#dc2626\", penwidth=2.5]"),
               "highlights first cyclic edge");
    assertTrue(contains(cyclic.dot, "\"payment\" -> \"order\" [label=\"depends\", color=\"#dc2626\", penwidth=2.5]"),
               "highlights closing cyclic edge");

    const auto layered = graph(
        "module ui\n"
        "module helper depends ui\n"
        "module data depends helper\n"
        "layer ui above data\n");
    assertTrue(contains(layered.dot, "\"data\" -> \"helper\" [label=\"depends\", color=\"#dc2626\", penwidth=2.5]"),
               "highlights first indirect layer-violation edge");
    assertTrue(contains(layered.dot, "\"helper\" -> \"ui\" [label=\"depends\", color=\"#dc2626\", penwidth=2.5]"),
               "highlights full indirect layer-violation path");

    const auto invalid = graph(
        "module ui\n"
        "module ui\n"
        "layer ui above missing\n"
        "layer ui above ui\n");
    assertTrue(contains(invalid.dot, "ui\\n(duplicate)"),
               "marks duplicate module node");
    assertTrue(contains(invalid.dot, "missing\\n(undefined)"),
               "marks undefined layer node");
    assertTrue(contains(invalid.dot, "\"ui\" -> \"ui\" [label=\"above\", style=\"dashed\", color=\"#dc2626\""),
               "highlights invalid self-layer edge");

    assertTestCount(14);

    std::cout << "\nGraph tests: " << passed << " passed, "
              << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
