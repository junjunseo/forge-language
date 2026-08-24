#ifndef IEUM_PARSER_H
#define IEUM_PARSER_H

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "token.h"
#include "ast.h"

// ── 파서 ───────────────────────────────────────────────
// 토큰 스트림(렉서 출력)을 받아 Program AST를 만든다.
// 전체 문법은 docs/GRAMMAR.md에서 관리한다.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)) {}

    Program parse() {
        Program prog;
        while (!isAtEnd()) {
            // 빈 줄은 건너뛴다
            if (check(TokenType::NEWLINE)) { advance(); continue; }

            if (check(TokenType::MODULE)) {
                prog.modules.push_back(parseModule());
            } else if (check(TokenType::LAYER)) {
                prog.layers.push_back(parseLayer());
            } else {
                throw error("선언은 'module' 또는 'layer'로 시작해야 합니다");
            }
            consumeLineEnd();
        }
        return prog;
    }

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // ── 선언 파싱 ──────────────────────────────────────
    ModuleDecl parseModule() {
        Token kw = advance();                 // MODULE
        Token name = expect(TokenType::IDENTIFIER,
                            "module 다음에는 모듈 이름이 와야 합니다");
        ModuleDecl decl;
        decl.name = name.value;
        decl.line = kw.line;

        if (check(TokenType::DEPENDS)) {
            advance();                        // DEPENDS
            decl.deps.push_back(
                expect(TokenType::IDENTIFIER,
                       "depends 다음에는 의존 대상 이름이 와야 합니다").value);
            while (check(TokenType::COMMA)) {
                advance();                    // COMMA
                decl.deps.push_back(
                    expect(TokenType::IDENTIFIER,
                           "',' 다음에는 의존 대상 이름이 와야 합니다").value);
            }
        }

        if (check(TokenType::LEFT_BRACE)) {
            decl.hasBody = true;
            advance();
            parseModuleBody(decl);
        }
        return decl;
    }

    void parseModuleBody(ModuleDecl& module) {
        if (check(TokenType::RIGHT_BRACE)) {
            advance();
            return;
        }

        consumeBlockStart("모듈 본문의 여는 중괄호 뒤에는 줄바꿈이 필요합니다");
        skipNewlines();

        while (!check(TokenType::RIGHT_BRACE)) {
            if (isAtEnd()) {
                throw error("모듈 본문을 닫는 '}'가 필요합니다");
            }

            if (check(TokenType::LET)) {
                module.variables.push_back(parseVariable());
            } else if (check(TokenType::FN)) {
                module.functions.push_back(parseFunction());
            } else {
                throw error("모듈 본문에는 'let' 또는 'fn' 선언만 올 수 있습니다");
            }

            consumeBlockMemberEnd("모듈 본문의 선언 뒤에는 줄바꿈이 필요합니다");
            skipNewlines();
        }

        advance(); // RIGHT_BRACE
    }

    VariableDecl parseVariable() {
        Token kw = advance(); // LET
        Token name = expect(TokenType::IDENTIFIER,
                            "let 다음에는 변수 이름이 와야 합니다");
        return VariableDecl{name.value, kw.line};
    }

    FunctionDecl parseFunction() {
        Token kw = advance(); // FN
        Token name = expect(TokenType::IDENTIFIER,
                            "fn 다음에는 함수 이름이 와야 합니다");
        expect(TokenType::LEFT_PAREN, "함수 이름 뒤에는 '('가 필요합니다");

        FunctionDecl function;
        function.name = name.value;
        function.line = kw.line;
        if (!check(TokenType::RIGHT_PAREN)) {
            function.parameters = parseIdentifierList("함수 매개변수 이름이 필요합니다");
        }
        expect(TokenType::RIGHT_PAREN, "함수 매개변수 목록을 닫는 ')'가 필요합니다");
        expect(TokenType::LEFT_BRACE, "함수 본문을 여는 '{'가 필요합니다");
        parseFunctionBody(function);
        return function;
    }

    void parseFunctionBody(FunctionDecl& function) {
        if (check(TokenType::RIGHT_BRACE)) {
            advance();
            return;
        }

        consumeBlockStart("함수 본문의 여는 중괄호 뒤에는 줄바꿈이 필요합니다");
        skipNewlines();

        while (!check(TokenType::RIGHT_BRACE)) {
            if (isAtEnd()) {
                throw error("함수 본문을 닫는 '}'가 필요합니다");
            }

            if (check(TokenType::LET)) {
                const VariableDecl variable = parseVariable();
                function.body.push_back({
                    Statement::Kind::VariableDeclaration,
                    variable.name,
                    {},
                    variable.line
                });
            } else if (check(TokenType::CALL)) {
                function.body.push_back(parseCall());
            } else {
                throw error("함수 본문에는 'let' 또는 'call' 문장만 올 수 있습니다");
            }

            consumeBlockMemberEnd("함수 본문의 문장 뒤에는 줄바꿈이 필요합니다");
            skipNewlines();
        }

        advance(); // RIGHT_BRACE
    }

    Statement parseCall() {
        Token kw = advance(); // CALL
        Token callee = expect(TokenType::IDENTIFIER,
                              "call 다음에는 함수 이름이 와야 합니다");
        expect(TokenType::LEFT_PAREN, "호출할 함수 이름 뒤에는 '('가 필요합니다");

        std::vector<std::string> arguments;
        if (!check(TokenType::RIGHT_PAREN)) {
            arguments = parseIdentifierList("호출 인자 이름이 필요합니다");
        }
        expect(TokenType::RIGHT_PAREN, "호출 인자 목록을 닫는 ')'가 필요합니다");

        return Statement{
            Statement::Kind::FunctionCall,
            callee.value,
            std::move(arguments),
            kw.line
        };
    }

    std::vector<std::string> parseIdentifierList(const std::string& itemError) {
        std::vector<std::string> names;
        names.push_back(expect(TokenType::IDENTIFIER, itemError).value);
        while (check(TokenType::COMMA)) {
            advance();
            names.push_back(expect(TokenType::IDENTIFIER,
                                   "',' 다음에는 식별자가 와야 합니다").value);
        }
        return names;
    }

    LayerDecl parseLayer() {
        Token kw = advance();                 // LAYER
        Token upper = expect(TokenType::IDENTIFIER,
                             "layer 다음에는 계층 이름이 와야 합니다");
        expect(TokenType::ABOVE, "계층 선언에는 'above'가 필요합니다");
        Token lower = expect(TokenType::IDENTIFIER,
                             "above 다음에는 하위 계층 이름이 와야 합니다");
        LayerDecl decl;
        decl.upper = upper.value;
        decl.lower = lower.value;
        decl.line  = kw.line;
        return decl;
    }

    // ── 토큰 유틸 ──────────────────────────────────────
    bool isAtEnd() const { return peek().type == TokenType::END; }
    const Token& peek() const { return tokens_[pos_]; }
    bool check(TokenType t) const { return peek().type == t; }

    void skipNewlines() {
        while (check(TokenType::NEWLINE)) advance();
    }

    Token advance() {
        Token t = tokens_[pos_];
        if (!isAtEnd()) pos_++;
        return t;
    }

    Token expect(TokenType t, const std::string& msg) {
        if (check(t)) return advance();
        throw error(msg);
    }

    // 선언 끝: NEWLINE 또는 파일 끝
    void consumeLineEnd() {
        if (check(TokenType::NEWLINE)) { advance(); return; }
        if (isAtEnd()) return;
        throw error("한 줄에는 하나의 선언만 올 수 있습니다");
    }

    void consumeBlockStart(const std::string& message) {
        if (!check(TokenType::NEWLINE)) throw error(message);
        advance();
    }

    void consumeBlockMemberEnd(const std::string& message) {
        if (check(TokenType::NEWLINE)) {
            advance();
            return;
        }
        throw error(message);
    }

    std::runtime_error error(const std::string& msg) const {
        return std::runtime_error(
            "[" + std::to_string(peek().line) + "행] 파싱 오류: " + msg +
            " (현재 토큰: " + tokenTypeName(peek().type) +
            (peek().value.empty() ? "" : " '" + peek().value + "'") + ")");
    }
};

#endif // IEUM_PARSER_H
