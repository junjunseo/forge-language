#ifndef IEUM_INTERPRETER_H
#define IEUM_INTERPRETER_H

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "ast.h"
#include "semantic.h"

enum class ExecutionEventKind {
    EnterFunction,
    CallFunction,
    ExitFunction
};

struct ExecutionEvent {
    ExecutionEventKind kind;
    std::string function;
    std::string target;
    std::size_t depth;
    int line;
};

struct ExecutionResult {
    bool success = false;
    std::string error;
    std::vector<ExecutionEvent> events;
    std::size_t functionsExecuted = 0;
    std::size_t callsExecuted = 0;
};

class Interpreter {
public:
    Interpreter(const Program& program, const SemanticResult& semantics)
        : program_(program), semantics_(semantics) {}

    ExecutionResult run(
        const std::string& moduleName,
        const std::string& functionName) const {
        ExecutionResult result;
        if (!semantics_.ok()) {
            result.error = "의미 오류가 있는 프로그램은 실행할 수 없습니다";
            return result;
        }

        const auto entry = findFunction(moduleName, functionName, result);
        if (!entry.has_value()) return result;

        const auto& function =
            program_.modules[entry->module].functions[entry->function];
        if (!function.parameters.empty()) {
            result.error = "진입 함수 '" + qualifiedName(*entry) +
                "'는 매개변수가 없어야 합니다";
            return result;
        }

        result.success = true;
        execute(*entry, 0, result);
        return result;
    }

private:
    static constexpr std::size_t kMaxCallDepth = 1024;

    const Program& program_;
    const SemanticResult& semantics_;

    std::optional<FunctionRef> findFunction(
        const std::string& moduleName,
        const std::string& functionName,
        ExecutionResult& result) const {
        std::optional<std::size_t> moduleIndex;
        for (std::size_t i = 0; i < program_.modules.size(); ++i) {
            if (program_.modules[i].name != moduleName) continue;
            if (moduleIndex.has_value()) {
                result.error = "실행 모듈 '" + moduleName + "'이 중복 선언되었습니다";
                return std::nullopt;
            }
            moduleIndex = i;
        }

        if (!moduleIndex.has_value()) {
            result.error = "실행 모듈 '" + moduleName + "'을 찾을 수 없습니다";
            return std::nullopt;
        }

        std::optional<std::size_t> functionIndex;
        const auto& module = program_.modules[*moduleIndex];
        for (std::size_t i = 0; i < module.functions.size(); ++i) {
            if (module.functions[i].name != functionName) continue;
            if (functionIndex.has_value()) {
                result.error = "진입 함수 '" + moduleName + "." + functionName +
                    "'가 중복 선언되었습니다";
                return std::nullopt;
            }
            functionIndex = i;
        }

        if (!functionIndex.has_value()) {
            result.error = "진입 함수 '" + moduleName + "." + functionName +
                "'을 찾을 수 없습니다";
            return std::nullopt;
        }
        return FunctionRef{*moduleIndex, *functionIndex};
    }

    void execute(
        const FunctionRef& functionRef,
        std::size_t depth,
        ExecutionResult& result) const {
        if (!result.success) return;
        if (depth > kMaxCallDepth) {
            result.success = false;
            result.error = "최대 함수 호출 깊이를 초과했습니다";
            return;
        }

        const auto& function =
            program_.modules[functionRef.module].functions[functionRef.function];
        const std::string name = qualifiedName(functionRef);
        result.events.push_back({
            ExecutionEventKind::EnterFunction,
            name,
            "",
            depth,
            function.line
        });
        result.functionsExecuted++;

        for (std::size_t statementIndex = 0;
             statementIndex < function.body.size();
             ++statementIndex) {
            const auto& statement = function.body[statementIndex];
            if (statement.kind != Statement::Kind::FunctionCall) continue;

            const auto* call = semantics_.findCall(functionRef, statementIndex);
            if (call == nullptr) {
                result.success = false;
                result.error = "해석되지 않은 호출이 실행 경로에 남아 있습니다";
                return;
            }

            const std::string target = qualifiedName(call->target);
            result.events.push_back({
                ExecutionEventKind::CallFunction,
                name,
                target,
                depth,
                statement.line
            });
            result.callsExecuted++;
            execute(call->target, depth + 1, result);
            if (!result.success) return;
        }

        result.events.push_back({
            ExecutionEventKind::ExitFunction,
            name,
            "",
            depth,
            function.line
        });
    }

    std::string qualifiedName(const FunctionRef& ref) const {
        const auto& module = program_.modules[ref.module];
        return module.name + "." + module.functions[ref.function].name;
    }
};

#endif // IEUM_INTERPRETER_H
