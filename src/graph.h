#ifndef IEUM_GRAPH_H
#define IEUM_GRAPH_H

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "ast.h"
#include "checker.h"

// Program과 Checker의 구조화된 위반 경로를 결정적인 Graphviz DOT으로 만든다.
class DependencyGraphExporter {
public:
    static std::string toDot(
        const Program& program,
        const std::vector<Violation>& violations) {
        const auto modules = firstModuleDeclarations(program);
        const auto declared = declaredNames(modules);
        const auto missing = missingNames(program, declared);
        const auto duplicates = duplicateNames(program);
        const auto highlightedDependencies = violationDependencyEdges(violations);

        std::ostringstream out;
        out << "digraph ieum {\n"
            << "  graph [rankdir=LR, bgcolor=\"white\"];\n"
            << "  node [shape=box, style=\"rounded,filled\", fillcolor=\"#f8fafc\", color=\"#334155\", fontname=\"sans-serif\"];\n"
            << "  edge [fontname=\"sans-serif\", fontsize=10];\n\n";

        std::set<std::string> allNames = declared;
        allNames.insert(missing.begin(), missing.end());
        for (const auto& name : allNames) {
            out << "  \"" << escape(name) << "\"";
            if (missing.find(name) != missing.end()) {
                out << " [label=\"" << escape(name)
                    << "\\n(undefined)\", style=\"dashed\", color=\"#dc2626\", fontcolor=\"#dc2626\"]";
            } else if (duplicates.find(name) != duplicates.end()) {
                out << " [label=\"" << escape(name)
                    << "\\n(duplicate)\", color=\"#dc2626\", fontcolor=\"#dc2626\"]";
            }
            out << ";\n";
        }

        const auto dependencyEdges = dependencies(modules);
        if (!dependencyEdges.empty()) out << "\n";
        for (const auto& edge : dependencyEdges) {
            out << "  \"" << escape(edge.first) << "\" -> \""
                << escape(edge.second) << "\" [label=\"depends\", color=\"";
            if (highlightedDependencies.find(edge) != highlightedDependencies.end()) {
                out << "#dc2626\", penwidth=2.5";
            } else {
                out << "#2563eb\"";
            }
            out << "];\n";
        }

        const auto layerEdges = layers(program);
        if (!layerEdges.empty()) out << "\n";
        for (const auto& edge : layerEdges) {
            const bool invalid =
                edge.first == edge.second ||
                declared.find(edge.first) == declared.end() ||
                declared.find(edge.second) == declared.end();
            out << "  \"" << escape(edge.first) << "\" -> \""
                << escape(edge.second)
                << "\" [label=\"above\", style=\"dashed\", color=\""
                << (invalid ? "#dc2626" : "#64748b")
                << "\", arrowhead=\"empty\"";
            if (invalid) out << ", penwidth=2.5";
            out << "];\n";
        }

        out << "}\n";
        return out.str();
    }

private:
    using Edge = std::pair<std::string, std::string>;

    static std::map<std::string, const ModuleDecl*> firstModuleDeclarations(
        const Program& program) {
        std::map<std::string, const ModuleDecl*> modules;
        for (const auto& module : program.modules) {
            modules.emplace(module.name, &module);
        }
        return modules;
    }

    static std::set<std::string> declaredNames(
        const std::map<std::string, const ModuleDecl*>& modules) {
        std::set<std::string> names;
        for (const auto& [name, module] : modules) {
            (void)module;
            names.insert(name);
        }
        return names;
    }

    static std::set<std::string> missingNames(
        const Program& program,
        const std::set<std::string>& declared) {
        std::set<std::string> names;
        for (const auto& module : program.modules) {
            for (const auto& dep : module.deps) {
                if (declared.find(dep) == declared.end()) names.insert(dep);
            }
        }
        for (const auto& layer : program.layers) {
            if (declared.find(layer.upper) == declared.end()) names.insert(layer.upper);
            if (declared.find(layer.lower) == declared.end()) names.insert(layer.lower);
        }
        return names;
    }

    static std::set<std::string> duplicateNames(const Program& program) {
        std::set<std::string> seen;
        std::set<std::string> duplicates;
        for (const auto& module : program.modules) {
            if (!seen.insert(module.name).second) duplicates.insert(module.name);
        }
        return duplicates;
    }

    static std::set<Edge> dependencies(
        const std::map<std::string, const ModuleDecl*>& modules) {
        std::set<Edge> edges;
        for (const auto& [name, module] : modules) {
            for (const auto& dep : module->deps) edges.emplace(name, dep);
        }
        return edges;
    }

    static std::set<Edge> layers(const Program& program) {
        std::set<Edge> edges;
        for (const auto& layer : program.layers) {
            // 위에서 아래 방향으로 그려 계층 선언 문장과 방향을 일치시킨다.
            edges.emplace(layer.upper, layer.lower);
        }
        return edges;
    }

    static std::set<Edge> violationDependencyEdges(
        const std::vector<Violation>& violations) {
        std::set<Edge> edges;
        for (const auto& violation : violations) {
            if (violation.kind != Violation::Kind::ImplicitDependency &&
                violation.kind != Violation::Kind::CyclicDependency &&
                violation.kind != Violation::Kind::LayerViolation) {
                continue;
            }
            for (std::size_t i = 1; i < violation.path.size(); ++i) {
                edges.emplace(violation.path[i - 1], violation.path[i]);
            }
        }
        return edges;
    }

    static std::string escape(const std::string& value) {
        std::string escaped;
        for (const char ch : value) {
            if (ch == '\\' || ch == '"') escaped += '\\';
            if (ch == '\n') {
                escaped += "\\n";
            } else {
                escaped += ch;
            }
        }
        return escaped;
    }
};

#endif // IEUM_GRAPH_H
