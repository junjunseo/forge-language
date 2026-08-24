#ifndef IEUM_AST_H
#define IEUM_AST_H

#include <string>
#include <vector>

// ── 구조 선언과 F1 모듈 본문 AST ───────────────────────

struct VariableDecl {
    std::string name;
    int line;
};

struct Statement {
    enum class Kind {
        VariableDeclaration,
        FunctionCall
    };

    Kind kind;
    std::string name;                  // 변수 이름 또는 호출 대상
    std::vector<std::string> arguments; // 호출이 아니면 비어 있음
    int line;
};

struct FunctionDecl {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<Statement> body;
    int line;
};

// module <name> [depends <dep1>, <dep2>, ...] [moduleBody]
struct ModuleDecl {
    std::string name;
    std::vector<std::string> deps;  // depends 가 없으면 비어 있음
    bool hasBody = false;
    std::vector<VariableDecl> variables;
    std::vector<FunctionDecl> functions;
    int line;
};

// layer <upper> above <lower>
struct LayerDecl {
    std::string upper;
    std::string lower;
    int line;
};

// 한 소스 파일 전체 = 모듈 선언들 + 계층 선언들
struct Program {
    std::vector<ModuleDecl> modules;
    std::vector<LayerDecl> layers;
};

#endif // IEUM_AST_H
