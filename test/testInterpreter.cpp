#include <cassert>
#include <iostream>
#include <string>

#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

namespace {

int passed = 0;
int failed = 0;

struct ExecutableProgram {
    Program program;
    SemanticResult semantics;
};

ExecutableProgram prepare(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    Program program = parser.parse();
    SemanticAnalyzer analyzer(program);
    SemanticResult semantics = analyzer.analyze();
    return ExecutableProgram{std::move(program), std::move(semantics)};
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

void assertTestCount(int expected) {
    if (passed != expected) {
        std::cerr << "[FAIL] expected " << expected
                  << " interpreter assertions, got " << passed << "\n";
        failed++;
        assert(passed == expected);
    }
}

} // namespace

int main() {
    {
        const ExecutableProgram executable = prepare(
            "module data {\n"
            "  fn save(item) {}\n"
            "}\n"
            "module service depends data {\n"
            "  fn prepare(item) {\n"
            "    call save(item)\n"
            "  }\n"
            "  fn main() {\n"
            "    let unit\n"
            "    call prepare(unit)\n"
            "  }\n"
            "}\n");
        Interpreter interpreter(executable.program, executable.semantics);
        const ExecutionResult result = interpreter.run("service", "main");

        assertTrue(executable.semantics.ok(),
                   "prepares semantically valid executable program");
        assertTrue(result.success, "executes zero-parameter entry function");
        assertTrue(result.functionsExecuted == 3,
                   "counts entered functions");
        assertTrue(result.callsExecuted == 2,
                   "counts executed calls");
        assertTrue(result.events.size() == 8,
                   "records enter, call and exit events");
        assertTrue(result.events[0].kind == ExecutionEventKind::EnterFunction &&
                   result.events[0].function == "service.main" &&
                   result.events[0].depth == 0,
                   "trace starts at requested entry function");
        assertTrue(result.events[1].kind == ExecutionEventKind::CallFunction &&
                   result.events[1].target == "service.prepare",
                   "trace records local function call");
        assertTrue(result.events[3].kind == ExecutionEventKind::CallFunction &&
                   result.events[3].target == "data.save" &&
                   result.events[3].depth == 1,
                   "trace records dependency function call and depth");
        assertTrue(result.events.back().kind == ExecutionEventKind::ExitFunction &&
                   result.events.back().function == "service.main",
                   "trace ends after entry function exits");
    }

    {
        const ExecutableProgram executable = prepare(
            "module app {\n"
            "  fn main() {}\n"
            "}\n");
        Interpreter interpreter(executable.program, executable.semantics);
        const ExecutionResult result = interpreter.run("missing", "main");
        assertTrue(!result.success &&
                   result.error.find("missing") != std::string::npos,
                   "rejects missing entry module");
    }

    {
        const ExecutableProgram executable = prepare(
            "module app {\n"
            "  fn main() {}\n"
            "}\n");
        Interpreter interpreter(executable.program, executable.semantics);
        const ExecutionResult result = interpreter.run("app", "missing");
        assertTrue(!result.success &&
                   result.error.find("app.missing") != std::string::npos,
                   "rejects missing entry function");
    }

    {
        const ExecutableProgram executable = prepare(
            "module app {\n"
            "  fn main(input) {}\n"
            "}\n");
        Interpreter interpreter(executable.program, executable.semantics);
        const ExecutionResult result = interpreter.run("app", "main");
        assertTrue(!result.success &&
                   result.error.find("매개변수가 없어야") != std::string::npos,
                   "rejects entry function that requires parameters");
    }

    {
        const ExecutableProgram executable = prepare(
            "module app {\n"
            "  fn main() {\n"
            "    call missing()\n"
            "  }\n"
            "}\n");
        Interpreter interpreter(executable.program, executable.semantics);
        const ExecutionResult result = interpreter.run("app", "main");
        assertTrue(!executable.semantics.ok() && !result.success,
                   "refuses to execute program with semantic violations");
    }

    {
        const ExecutableProgram executable = prepare(
            "module app {\n"
            "  fn main() {}\n"
            "}\n");
        Interpreter interpreter(executable.program, executable.semantics);
        const ExecutionResult result = interpreter.run("app", "main");
        assertTrue(result.success && result.functionsExecuted == 1 &&
                   result.callsExecuted == 0,
                   "executes empty entry function");
        assertTrue(result.events.size() == 2 &&
                   result.events[0].kind == ExecutionEventKind::EnterFunction &&
                   result.events[1].kind == ExecutionEventKind::ExitFunction,
                   "records enter and exit for empty function");
    }

    assertTestCount(15);

    std::cout << "\nInterpreter tests: " << passed << " passed, "
              << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
