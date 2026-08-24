#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "token.h"
#include "ast.h"
#include "parser.h"

namespace {

int passed = 0;
int failed = 0;

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

Program parseTokens(const std::vector<Token>& tokens) {
    Parser parser(tokens);
    return parser.parse();
}

void assertThrows(const std::vector<Token>& tokens, const std::string& name) {
    try {
        parseTokens(tokens);
        assertTrue(false, name);
    } catch (const std::runtime_error&) {
        assertTrue(true, name);
    }
}

void assertTestCount(int expected) {
    if (passed != expected) {
        std::cerr << "[FAIL] expected " << expected << " parser assertions, got "
                  << passed << "\n";
        failed++;
        assert(passed == expected);
    }
}

} // namespace

int main() {
    const std::vector<Token> validTokens = {
        {TokenType::MODULE,     "module",  1}, {TokenType::IDENTIFIER, "domain", 1}, {TokenType::NEWLINE, "", 1},
        {TokenType::MODULE,     "module",  2}, {TokenType::IDENTIFIER, "ui",     2}, {TokenType::DEPENDS, "depends", 2}, {TokenType::IDENTIFIER, "domain", 2}, {TokenType::NEWLINE, "", 2},
        {TokenType::MODULE,     "module",  3}, {TokenType::IDENTIFIER, "infra",  3}, {TokenType::DEPENDS, "depends", 3}, {TokenType::IDENTIFIER, "domain", 3}, {TokenType::COMMA, ",", 3}, {TokenType::IDENTIFIER, "ui", 3}, {TokenType::NEWLINE, "", 3},
        {TokenType::LAYER,      "layer",   4}, {TokenType::IDENTIFIER, "ui",     4}, {TokenType::ABOVE, "above", 4}, {TokenType::IDENTIFIER, "domain", 4}, {TokenType::NEWLINE, "", 4},
        {TokenType::END,        "",        5},
    };

    try {
        Program prog = parseTokens(validTokens);
        assertTrue(prog.modules.size() == 3, "parses three module declarations");
        assertTrue(prog.modules[0].name == "domain", "preserves first module name");
        assertTrue(prog.modules[1].deps.size() == 1, "parses single dependency");
        assertTrue(prog.modules[1].deps[0] == "domain", "preserves single dependency target");
        assertTrue(prog.modules[2].deps.size() == 2, "parses multiple dependency targets");
        assertTrue(prog.modules[2].deps[1] == "ui", "preserves dependency target order");
        assertTrue(prog.modules[2].line == 3, "preserves module declaration line");
        assertTrue(prog.layers.size() == 1, "parses layer declaration");
        assertTrue(prog.layers[0].upper == "ui" && prog.layers[0].lower == "domain",
                   "preserves layer relation");
        assertTrue(prog.layers[0].line == 4, "preserves layer declaration line");
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] valid token parsing threw: " << e.what() << "\n";
        failed++;
        assert(false);
    }

    {
        const Program prog = parseTokens({
            {TokenType::NEWLINE, "", 1},
            {TokenType::NEWLINE, "", 2},
            {TokenType::END, "", 3},
        });
        assertTrue(prog.modules.empty() && prog.layers.empty(), "skips empty lines");
    }

    {
        const Program prog = parseTokens({
            {TokenType::MODULE, "module", 1},
            {TokenType::IDENTIFIER, "solo", 1},
            {TokenType::END, "", 1},
        });
        assertTrue(prog.modules.size() == 1 && prog.modules[0].name == "solo",
                   "parses final declaration without trailing newline");
    }

    {
        const Program prog = parseTokens({
            {TokenType::MODULE, "module", 1},
            {TokenType::IDENTIFIER, "app", 1},
            {TokenType::DEPENDS, "depends", 1},
            {TokenType::IDENTIFIER, "data", 1},
            {TokenType::LEFT_BRACE, "{", 1},
            {TokenType::NEWLINE, "", 1},
            {TokenType::LET, "let", 2},
            {TokenType::IDENTIFIER, "state", 2},
            {TokenType::NEWLINE, "", 2},
            {TokenType::FN, "fn", 3},
            {TokenType::IDENTIFIER, "run", 3},
            {TokenType::LEFT_PAREN, "(", 3},
            {TokenType::IDENTIFIER, "input", 3},
            {TokenType::COMMA, ",", 3},
            {TokenType::IDENTIFIER, "context", 3},
            {TokenType::RIGHT_PAREN, ")", 3},
            {TokenType::LEFT_BRACE, "{", 3},
            {TokenType::NEWLINE, "", 3},
            {TokenType::LET, "let", 4},
            {TokenType::IDENTIFIER, "local", 4},
            {TokenType::NEWLINE, "", 4},
            {TokenType::CALL, "call", 5},
            {TokenType::IDENTIFIER, "persist", 5},
            {TokenType::LEFT_PAREN, "(", 5},
            {TokenType::IDENTIFIER, "input", 5},
            {TokenType::COMMA, ",", 5},
            {TokenType::IDENTIFIER, "local", 5},
            {TokenType::RIGHT_PAREN, ")", 5},
            {TokenType::NEWLINE, "", 5},
            {TokenType::RIGHT_BRACE, "}", 6},
            {TokenType::NEWLINE, "", 6},
            {TokenType::RIGHT_BRACE, "}", 7},
            {TokenType::NEWLINE, "", 7},
            {TokenType::MODULE, "module", 8},
            {TokenType::IDENTIFIER, "data", 8},
            {TokenType::END, "", 8},
        });

        const ModuleDecl& app = prog.modules[0];
        const FunctionDecl& run = app.functions[0];
        assertTrue(prog.modules.size() == 2, "parses body module and following module");
        assertTrue(app.name == "app" && app.deps[0] == "data",
                   "preserves body module header");
        assertTrue(app.hasBody, "marks module body presence");
        assertTrue(app.variables.size() == 1 && app.variables[0].name == "state",
                   "parses module variable declaration");
        assertTrue(app.variables[0].line == 2, "preserves module variable line");
        assertTrue(app.functions.size() == 1 && run.name == "run",
                   "parses function declaration");
        assertTrue(run.line == 3, "preserves function line");
        assertTrue(run.parameters.size() == 2, "parses function parameters");
        assertTrue(run.parameters[0] == "input" && run.parameters[1] == "context",
                   "preserves function parameter order");
        assertTrue(run.body.size() == 2, "preserves function statement count");
        assertTrue(run.body[0].kind == Statement::Kind::VariableDeclaration &&
                   run.body[0].name == "local",
                   "parses local variable statement");
        assertTrue(run.body[0].line == 4, "preserves local variable line");
        assertTrue(run.body[1].kind == Statement::Kind::FunctionCall &&
                   run.body[1].name == "persist",
                   "parses function call statement");
        assertTrue(run.body[1].arguments.size() == 2 &&
                   run.body[1].arguments[0] == "input" &&
                   run.body[1].arguments[1] == "local" &&
                   run.body[1].line == 5,
                   "preserves call arguments and line");
        assertTrue(!prog.modules[1].hasBody, "distinguishes module without body");
    }

    {
        const Program prog = parseTokens({
            {TokenType::MODULE, "module", 1},
            {TokenType::IDENTIFIER, "empty", 1},
            {TokenType::LEFT_BRACE, "{", 1},
            {TokenType::RIGHT_BRACE, "}", 1},
            {TokenType::NEWLINE, "", 1},
            {TokenType::MODULE, "module", 2},
            {TokenType::IDENTIFIER, "worker", 2},
            {TokenType::LEFT_BRACE, "{", 2},
            {TokenType::NEWLINE, "", 2},
            {TokenType::FN, "fn", 3},
            {TokenType::IDENTIFIER, "noop", 3},
            {TokenType::LEFT_PAREN, "(", 3},
            {TokenType::RIGHT_PAREN, ")", 3},
            {TokenType::LEFT_BRACE, "{", 3},
            {TokenType::RIGHT_BRACE, "}", 3},
            {TokenType::NEWLINE, "", 3},
            {TokenType::RIGHT_BRACE, "}", 4},
            {TokenType::END, "", 4},
        });

        assertTrue(prog.modules.size() == 2, "parses multiple empty bodies");
        assertTrue(prog.modules[0].hasBody && prog.modules[0].variables.empty() &&
                   prog.modules[0].functions.empty(),
                   "parses empty module body");
        assertTrue(prog.modules[1].functions.size() == 1 &&
                   prog.modules[1].functions[0].body.empty(),
                   "parses empty function body");
    }

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects module declaration without name");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "ui", 1},
        {TokenType::DEPENDS, "depends", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects depends without target");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "ui", 1},
        {TokenType::DEPENDS, "depends", 1},
        {TokenType::IDENTIFIER, "domain", 1},
        {TokenType::COMMA, ",", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects comma without following dependency");

    assertThrows({
        {TokenType::LAYER, "layer", 1},
        {TokenType::IDENTIFIER, "ui", 1},
        {TokenType::IDENTIFIER, "domain", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects layer declaration without above");

    assertThrows({
        {TokenType::LAYER, "layer", 1},
        {TokenType::IDENTIFIER, "ui", 1},
        {TokenType::ABOVE, "above", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects layer declaration without lower target");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "ui", 1},
        {TokenType::DEPENDS, "depends", 1},
        {TokenType::IDENTIFIER, "domain", 1},
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "extra", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects multiple declarations on one line");

    assertThrows({
        {TokenType::IDENTIFIER, "ui", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::END, "", 2},
    }, "rejects declarations that do not start with keyword");

    assertThrows({
        {TokenType::UNKNOWN, "@", 1},
        {TokenType::END, "", 1},
    }, "rejects unknown token");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::LET, "let", 1},
        {TokenType::IDENTIFIER, "value", 1},
        {TokenType::RIGHT_BRACE, "}", 1},
        {TokenType::END, "", 1},
    }, "rejects non-empty module body on opening line");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::CALL, "call", 2},
        {TokenType::IDENTIFIER, "run", 2},
        {TokenType::LEFT_PAREN, "(", 2},
        {TokenType::RIGHT_PAREN, ")", 2},
        {TokenType::NEWLINE, "", 2},
        {TokenType::RIGHT_BRACE, "}", 3},
        {TokenType::END, "", 3},
    }, "rejects call directly in module body");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::FN, "fn", 2},
        {TokenType::IDENTIFIER, "run", 2},
        {TokenType::RIGHT_PAREN, ")", 2},
        {TokenType::END, "", 2},
    }, "rejects function without opening parenthesis");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::LET, "let", 2},
        {TokenType::IDENTIFIER, "value", 2},
        {TokenType::END, "", 2},
    }, "rejects unterminated module body");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::FN, "fn", 2},
        {TokenType::IDENTIFIER, "run", 2},
        {TokenType::LEFT_PAREN, "(", 2},
        {TokenType::RIGHT_PAREN, ")", 2},
        {TokenType::LEFT_BRACE, "{", 2},
        {TokenType::NEWLINE, "", 2},
        {TokenType::CALL, "call", 3},
        {TokenType::IDENTIFIER, "helper", 3},
        {TokenType::LEFT_PAREN, "(", 3},
        {TokenType::RIGHT_PAREN, ")", 3},
        {TokenType::END, "", 3},
    }, "rejects unterminated function body");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::FN, "fn", 2},
        {TokenType::IDENTIFIER, "run", 2},
        {TokenType::LEFT_PAREN, "(", 2},
        {TokenType::RIGHT_PAREN, ")", 2},
        {TokenType::LEFT_BRACE, "{", 2},
        {TokenType::NEWLINE, "", 2},
        {TokenType::CALL, "call", 3},
        {TokenType::IDENTIFIER, "helper", 3},
        {TokenType::NEWLINE, "", 3},
        {TokenType::RIGHT_BRACE, "}", 4},
        {TokenType::NEWLINE, "", 4},
        {TokenType::RIGHT_BRACE, "}", 5},
        {TokenType::END, "", 5},
    }, "rejects call without opening parenthesis");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::FN, "fn", 2},
        {TokenType::IDENTIFIER, "run", 2},
        {TokenType::LEFT_PAREN, "(", 2},
        {TokenType::IDENTIFIER, "value", 2},
        {TokenType::COMMA, ",", 2},
        {TokenType::RIGHT_PAREN, ")", 2},
        {TokenType::END, "", 2},
    }, "rejects missing parameter after comma");

    assertThrows({
        {TokenType::MODULE, "module", 1},
        {TokenType::IDENTIFIER, "app", 1},
        {TokenType::LEFT_BRACE, "{", 1},
        {TokenType::NEWLINE, "", 1},
        {TokenType::FN, "fn", 2},
        {TokenType::IDENTIFIER, "run", 2},
        {TokenType::LEFT_PAREN, "(", 2},
        {TokenType::RIGHT_PAREN, ")", 2},
        {TokenType::LEFT_BRACE, "{", 2},
        {TokenType::NEWLINE, "", 2},
        {TokenType::FN, "fn", 3},
        {TokenType::IDENTIFIER, "nested", 3},
        {TokenType::END, "", 3},
    }, "rejects nested function declaration");

    assertTestCount(46);

    std::cout << "\nParser tests: " << passed << " passed, "
              << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
