CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
VERSION := $(strip $(file <VERSION))
CPPFLAGS = -Isrc -DIEUM_VERSION=\"$(VERSION)\"

ifeq ($(OS),Windows_NT)
	EXE = .exe
endif

BUILD_DIR = build
TARGET = $(BUILD_DIR)/ieum$(EXE)
TEST_PARSER = $(BUILD_DIR)/testParser$(EXE)
TEST_PIPELINE = $(BUILD_DIR)/testPipeline$(EXE)
TEST_CHECKER = $(BUILD_DIR)/testChecker$(EXE)
BENCHMARK_CHECKER = $(BUILD_DIR)/benchmarkChecker$(EXE)
BENCHMARK_MODULES ?= 200
BENCHMARK_ITERATIONS ?= 7
HEADERS = src/token.h src/ast.h src/lexer.h src/parser.h src/checker.h src/version.h
INVALID_EXAMPLES = \
	examples/implicit_dependency.ieum \
	examples/cyclic_dependency.ieum \
	examples/layer_violation.ieum \
	examples/transitive_layer_violation.ieum \
	examples/invalid_declarations.ieum
VALID_EXAMPLES = \
	examples/valid.ieum \
	examples/module_body.ieum

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): src/main.cpp $(HEADERS) VERSION | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(TEST_PARSER): test/testParser.cpp src/token.h src/ast.h src/parser.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(TEST_PIPELINE): test/testPipeline.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(TEST_CHECKER): test/testChecker.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BENCHMARK_CHECKER): benchmark/benchmarkChecker.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O2 -DNDEBUG $< -o $@

run: $(TARGET)
	./$(TARGET) examples/valid.ieum

test: $(TEST_PARSER) $(TEST_PIPELINE) $(TEST_CHECKER) $(TARGET) $(BENCHMARK_CHECKER)
	./$(TEST_PARSER)
	./$(TEST_PIPELINE)
	./$(TEST_CHECKER)
	./$(BENCHMARK_CHECKER) 2 1
	test "$$(./$(TARGET) --version)" = "ieum $(VERSION)"
	@for example in $(VALID_EXAMPLES); do \
		./$(TARGET) $$example || exit 1; \
	done
	@for example in $(INVALID_EXAMPLES); do \
		./$(TARGET) $$example >/dev/null 2>&1; \
		status=$$?; \
		if [ $$status -eq 0 ]; then \
			echo "Expected structural violation for $$example"; \
			exit 1; \
		fi; \
		if [ $$status -ne 1 ]; then \
			echo "Expected exit code 1 for $$example, got $$status"; \
			exit 1; \
		fi; \
	done

benchmark: $(BENCHMARK_CHECKER)
	./$(BENCHMARK_CHECKER) $(BENCHMARK_MODULES) $(BENCHMARK_ITERATIONS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run test benchmark clean
