#!/bin/bash
set -e

echo "🧪 Running Glint3D Test Suite"
echo "=============================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results tracking
UNIT_TESTS_PASSED=0
INTEGRATION_TESTS_PASSED=0
SECURITY_TESTS_PASSED=0
GOLDEN_TESTS_PASSED=0

TEST_START_TIME=$(date +%s)

echo -e "${BLUE}Starting test run at $(date)${NC}"
echo

# Ensure we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}❌ Error: Run this script from the Glint3D root directory${NC}"
    exit 1
fi

# Build the project first if needed
if [ ! -f "builds/desktop/cmake/glint" ] && [ ! -f "builds/desktop/cmake/Release/glint.exe" ]; then
    echo -e "${YELLOW}🔨 Building project first...${NC}"
    mkdir -p builds/desktop/cmake
    cd builds/desktop/cmake
    cmake -S ../../.. -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
    cmake --build . --config Release -j
    cd ../../..
    echo -e "${GREEN}✅ Build completed${NC}"
    echo
fi

# Create results directory
mkdir -p tests/results/{unit,integration,security,golden,logs}

# 1. Unit Tests
echo -e "${BLUE}1. Running Unit Tests${NC}"
echo "---------------------"
if tests/scripts/run_unit_tests.sh > tests/results/logs/unit_tests.log 2>&1; then
    UNIT_TESTS_PASSED=1
    echo -e "${GREEN}✅ Unit tests passed${NC}"
else
    echo -e "${RED}❌ Unit tests failed${NC}"
    echo "Check tests/results/logs/unit_tests.log for details"
fi
echo

# 2. Integration Tests  
echo -e "${BLUE}2. Running Integration Tests${NC}"
echo "----------------------------"
if tests/scripts/run_integration_tests.sh > tests/results/logs/integration_tests.log 2>&1; then
    INTEGRATION_TESTS_PASSED=1
    echo -e "${GREEN}✅ Integration tests passed${NC}"
else
    echo -e "${RED}❌ Integration tests failed${NC}"
    echo "Check tests/results/logs/integration_tests.log for details"
fi
echo

# 3. Security Tests
echo -e "${BLUE}3. Running Security Tests${NC}"
echo "-------------------------"
if tests/scripts/run_security_tests.sh > tests/results/logs/security_tests.log 2>&1; then
    SECURITY_TESTS_PASSED=1
    echo -e "${GREEN}✅ Security tests passed${NC}"
else
    echo -e "${RED}❌ Security tests failed${NC}"
    echo "Check tests/results/logs/security_tests.log for details"
fi
echo

# 4. Golden Image Tests
echo -e "${BLUE}4. Running Golden Image Tests${NC}"
echo "-----------------------------"
if tests/scripts/run_golden_tests.sh > tests/results/logs/golden_tests.log 2>&1; then
    GOLDEN_TESTS_PASSED=1
    echo -e "${GREEN}✅ Golden image tests passed${NC}"
else
    echo -e "${RED}❌ Golden image tests failed${NC}"
    echo "Check tests/results/logs/golden_tests.log for details"
fi
echo

# Summary
TEST_END_TIME=$(date +%s)
TOTAL_TIME=$((TEST_END_TIME - TEST_START_TIME))

echo "=============================="
echo -e "${BLUE}🧪 Test Suite Summary${NC}"
echo "=============================="
echo -e "Unit Tests:        $([ $UNIT_TESTS_PASSED -eq 1 ] && echo -e "${GREEN}✅ PASS${NC}" || echo -e "${RED}❌ FAIL${NC}")"
echo -e "Integration Tests: $([ $INTEGRATION_TESTS_PASSED -eq 1 ] && echo -e "${GREEN}✅ PASS${NC}" || echo -e "${RED}❌ FAIL${NC}")"
echo -e "Security Tests:    $([ $SECURITY_TESTS_PASSED -eq 1 ] && echo -e "${GREEN}✅ PASS${NC}" || echo -e "${RED}❌ FAIL${NC}")"
echo -e "Golden Image Tests:$([ $GOLDEN_TESTS_PASSED -eq 1 ] && echo -e "${GREEN}✅ PASS${NC}" || echo -e "${RED}❌ FAIL${NC}")"
echo
echo "Total execution time: ${TOTAL_TIME}s"
echo "Results saved to: tests/results/"

# Exit with appropriate code
TOTAL_PASSED=$((UNIT_TESTS_PASSED + INTEGRATION_TESTS_PASSED + SECURITY_TESTS_PASSED + GOLDEN_TESTS_PASSED))
if [ $TOTAL_PASSED -eq 4 ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    FAILED_COUNT=$((4 - TOTAL_PASSED))
    echo -e "${RED}💥 ${FAILED_COUNT} test suite(s) failed${NC}"
    exit 1
fi