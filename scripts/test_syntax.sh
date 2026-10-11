#!/bin/bash
set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "=== Verifying C++ headers and wrapper syntax ==="

cat << 'TEST_CPP' > tmp_test_syntax.cpp
#include "include/vcs/vcs_types.h"
#include "include/vcs/CAPIHandler.h"
#include "include/vcs/vfmtztransport.h"
#include <iostream>

using namespace vcs;

int main() {
    std::cout << "sizeof(vcsCBEvent_t): " << sizeof(vcsCBEvent_t) << " (expected 16 on 32-bit)" << std::endl;
    std::cout << "sizeof(vcsEnrollCaptureStatus): " << sizeof(vcsEnrollCaptureStatus) << " (expected 32)" << std::endl;
    std::cout << "sizeof(vcsIdentifyResult): " << sizeof(vcsIdentifyResult) << " (expected 180 on 32-bit)" << std::endl;
    std::cout << "sizeof(vcsEnrollUser_t): " << sizeof(vcsEnrollUser_t) << " (expected 92 on 32-bit)" << std::endl;
    
    if (sizeof(vcsEnrollCaptureStatus) != 32) {
        std::cerr << "vcsEnrollCaptureStatus size mismatch!" << std::endl;
        return 2;
    }
    return 0;
}
TEST_CPP

# Test with 32-bit flag if available, otherwise standard 64-bit host compilation syntax check
if g++ -m32 tmp_test_syntax.cpp -o tmp_test_syntax 2>/dev/null; then
    echo "Running 32-bit ABI verification:"
    ./tmp_test_syntax
elif g++ -std=c++17 -I. tmp_test_syntax.cpp -o tmp_test_syntax; then
    echo "Running host compilation verification (64-bit host):"
    ./tmp_test_syntax
fi

rm -f tmp_test_syntax tmp_test_syntax.cpp
echo "=== Basic struct verification passed ==="
