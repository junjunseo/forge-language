#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include "token.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "checker.h"

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

Program parse(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    return parser.parse();
}

void assertParseThrows(const std::string& source, const std::string& name) {
    try {
        parse(source);
        assertTrue(false, name);
    } catch (const std::runtime_error&) {
        assertTrue(true, name);
    }
}

void assertTestCount(int expected) {
    if (passed != expected) {
        std::cerr << "[FAIL] expected " << expected << " pipeline assertions, got "
                  << passed << "\n";
        failed++;
        assert(passed == expected);
    }
}

} // namespace

int main() {
    const std::string validSource =
        "# comments and blank lines are accepted\n"
        "\n"
        "module domain\n"
        "module service depends domain\n"
        "module ui depends service\n"
        "layer ui above service\n"
        "layer service above domain\n";

    try {
        Program prog = parse(validSource);
        assertTrue(prog.modules.size() == 3, "lexer-parser pipeline parses modules");
        assertTrue(prog.layers.size() == 2, "lexer-parser pipeline parses layers");
        assertTrue(prog.modules[1].deps.size() == 1 &&
                   prog.modules[1].deps[0] == "domain",
                   "pipeline preserves dependency target");
        assertTrue(prog.modules[2].line == 5, "pipeline preserves source line numbers");

        Checker checker(prog);
        assertTrue(checker.check().empty(), "valid structure passes full pipeline");
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] valid pipeline threw: " << e.what() << "\n";
        failed++;
        assert(false);
    }

    try {
        Program prog = parse(
            "\xEF\xBB\xBF"
            "module data\n"
            "module service depends data\n");
        assertTrue(prog.modules.size() == 2 && prog.modules[0].name == "data",
                   "parses UTF-8 BOM input");
        assertTrue(prog.modules[1].deps.size() == 1 &&
                   prog.modules[1].deps[0] == "data",
                   "preserves dependency after UTF-8 BOM");
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] BOM input threw: " << e.what() << "\n";
        failed++;
        assert(false);
    }

    {
        Program prog = parse("module data # inline comment\nmodule service depends data\n");
        assertTrue(prog.modules.size() == 2, "ignores inline comments after declarations");
    }

    {
        Program prog = parse("module\tdata\r\nmodule service depends\tdata\r\n");
        assertTrue(prog.modules.size() == 2 && prog.modules[1].deps[0] == "data",
                   "accepts tabs and CRLF line endings");
    }

    {
        Program prog = parse("module api depends data, infra\nmodule data\nmodule infra\n");
        assertTrue(prog.modules[0].deps.size() == 2 &&
                   prog.modules[0].deps[1] == "infra",
                   "parses comma-separated dependencies with spaces");
    }

    {
        Program prog = parse("module _data1\nmodule service2 depends _data1\n");
        assertTrue(prog.modules[0].name == "_data1" &&
                   prog.modules[1].name == "service2",
                   "accepts ASCII identifiers with underscores and digits");
    }

    {
        Program prog = parse("\n\nmodule data\n\n");
        assertTrue(prog.modules.size() == 1 && prog.modules[0].line == 3,
                   "ignores leading and trailing blank lines");
    }

    try {
        const Program prog = parse(
            "module data {\n"
            "  let connection\n"
            "  fn persist(value) {\n"
            "    let copy\n"
            "    call save(value, copy)\n"
            "  }\n"
            "}\n"
            "module service depends data {\n"
            "  fn run(request) {\n"
            "    call persist(request)\n"
            "  }\n"
            "}\n"
            "layer service above data\n");

        const ModuleDecl& data = prog.modules[0];
        const FunctionDecl& persist = data.functions[0];
        const FunctionDecl& run = prog.modules[1].functions[0];
        assertTrue(prog.modules.size() == 2, "pipeline parses modules with bodies");
        assertTrue(data.hasBody, "pipeline marks module body presence");
        assertTrue(data.variables.size() == 1 &&
                   data.variables[0].name == "connection" &&
                   data.variables[0].line == 2,
                   "pipeline preserves module variable and line");
        assertTrue(persist.name == "persist" && persist.line == 3,
                   "pipeline preserves function and line");
        assertTrue(persist.parameters.size() == 1 &&
                   persist.parameters[0] == "value",
                   "pipeline parses function parameter");
        assertTrue(persist.body.size() == 2,
                   "pipeline preserves function statement order");
        assertTrue(persist.body[0].kind == Statement::Kind::VariableDeclaration &&
                   persist.body[1].kind == Statement::Kind::FunctionCall,
                   "pipeline distinguishes variable and call statements");
        assertTrue(persist.body[1].name == "save" && persist.body[1].line == 5,
                   "pipeline preserves call target and line");
        assertTrue(persist.body[1].arguments.size() == 2 &&
                   persist.body[1].arguments[0] == "value" &&
                   persist.body[1].arguments[1] == "copy",
                   "pipeline preserves call argument order");
        assertTrue(run.body.size() == 1 && run.body[0].name == "persist",
                   "pipeline parses body after dependency header");

        Checker checker(prog);
        assertTrue(checker.check().empty(),
                   "module bodies preserve structural checker result");
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] module body pipeline threw: " << e.what() << "\n";
        failed++;
        assert(false);
    }

    {
        const Program prog = parse(
            "module empty {}\n"
            "module worker {\n"
            "  fn noop() {}\n"
            "}\n");
        assertTrue(prog.modules[0].hasBody &&
                   prog.modules[0].variables.empty() &&
                   prog.modules[0].functions.empty(),
                   "lexer-parser accepts empty module body");
        assertTrue(prog.modules[1].functions.size() == 1 &&
                   prog.modules[1].functions[0].body.empty(),
                   "lexer-parser accepts empty function body");
    }

    assertParseThrows("module ui @ domain\n", "rejects unknown character in source");
    assertParseThrows("module ui depends\n", "rejects incomplete depends declaration");
    assertParseThrows("module 데이터\n", "rejects non-ASCII identifier");
    assertParseThrows("depends data\n", "rejects declaration without module or layer keyword");
    assertParseThrows("module app { let value\n}\n",
                      "rejects module content on opening line");
    assertParseThrows("module app {\n  let value\n",
                      "rejects missing module closing brace");
    assertParseThrows(
        "module app {\n  fn run() {\n    call helper(value\n  }\n}\n",
        "rejects missing call closing parenthesis");
    assertParseThrows("let value\n", "rejects variable at top level");
    assertParseThrows("module app {\n  call run()\n}\n",
                      "rejects call directly in module body");
    assertParseThrows("module app {\n  fn run() { call helper() }\n}\n",
                      "rejects function content on opening line");

    assertTestCount(35);

    std::cout << "\nPipeline tests: " << passed << " passed, "
              << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
