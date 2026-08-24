#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "checker.h"
#include "lexer.h"
#include "parser.h"

namespace {

struct BenchmarkResult {
    std::size_t modules;
    std::size_t layers;
    std::size_t sourceBytes;
    std::size_t iterations;
    double minMs;
    double medianMs;
    double p95Ms;
    double maxMs;
};

std::size_t parsePositive(const char* value, const char* name) {
    std::string text(value);
    std::size_t consumed = 0;
    unsigned long long parsed = 0;

    try {
        parsed = std::stoull(text, &consumed);
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string(name) + " must be a positive integer");
    }

    if (consumed != text.size() || parsed == 0) {
        throw std::invalid_argument(std::string(name) + " must be a positive integer");
    }
    return static_cast<std::size_t>(parsed);
}

std::string moduleName(std::size_t index) {
    return "module_" + std::to_string(index);
}

std::string makeLayeredChain(std::size_t moduleCount) {
    std::ostringstream source;

    for (std::size_t i = 0; i < moduleCount; ++i) {
        source << "module " << moduleName(i);
        if (i + 1 < moduleCount) {
            source << " depends " << moduleName(i + 1);
        }
        source << '\n';
    }

    for (std::size_t i = 0; i + 1 < moduleCount; ++i) {
        source << "layer " << moduleName(i)
               << " above " << moduleName(i + 1) << '\n';
    }

    return source.str();
}

void runPipeline(const std::string& source, std::size_t expectedModules) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    Program program = parser.parse();

    if (program.modules.size() != expectedModules ||
        program.layers.size() + 1 != expectedModules) {
        throw std::runtime_error("benchmark corpus was parsed incorrectly");
    }

    Checker checker(program);
    const auto violations = checker.check();
    if (!violations.empty()) {
        throw std::runtime_error("valid benchmark corpus produced structural violations");
    }
}

BenchmarkResult benchmark(std::size_t moduleCount, std::size_t iterations) {
    const std::string source = makeLayeredChain(moduleCount);
    std::vector<double> samples;
    samples.reserve(iterations);

    runPipeline(source, moduleCount);  // Warm-up and correctness check.

    for (std::size_t i = 0; i < iterations; ++i) {
        const auto started = std::chrono::steady_clock::now();
        runPipeline(source, moduleCount);
        const auto finished = std::chrono::steady_clock::now();
        samples.push_back(
            std::chrono::duration<double, std::milli>(finished - started).count());
    }

    std::sort(samples.begin(), samples.end());
    const std::size_t medianIndex = samples.size() / 2;
    const double median =
        samples.size() % 2 == 0
            ? (samples[medianIndex - 1] + samples[medianIndex]) / 2.0
            : samples[medianIndex];
    const std::size_t p95Index = (samples.size() * 95 + 99) / 100 - 1;

    return BenchmarkResult{
        moduleCount,
        moduleCount - 1,
        source.size(),
        iterations,
        samples.front(),
        median,
        samples[p95Index],
        samples.back()
    };
}

void printResult(const BenchmarkResult& result) {
    const double modulesPerSecond =
        result.medianMs > 0.0
            ? static_cast<double>(result.modules) * 1000.0 / result.medianMs
            : 0.0;

    std::cout << std::fixed << std::setprecision(3)
              << "scenario=layered-valid-chain\n"
              << "modules=" << result.modules << '\n'
              << "layers=" << result.layers << '\n'
              << "source_bytes=" << result.sourceBytes << '\n'
              << "iterations=" << result.iterations << '\n'
              << "min_ms=" << result.minMs << '\n'
              << "median_ms=" << result.medianMs << '\n'
              << "p95_ms=" << result.p95Ms << '\n'
              << "max_ms=" << result.maxMs << '\n'
              << "median_modules_per_second=" << modulesPerSecond << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: benchmarkChecker <module-count> <iterations>\n";
        return 2;
    }

    try {
        const std::size_t moduleCount = parsePositive(argv[1], "module-count");
        const std::size_t iterations = parsePositive(argv[2], "iterations");
        if (moduleCount < 2) {
            throw std::invalid_argument("module-count must be at least 2");
        }

        printResult(benchmark(moduleCount, iterations));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "benchmark error: " << error.what() << '\n';
        return 2;
    }
}
