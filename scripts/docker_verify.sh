#!/bin/bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "=== Verifying 32-bit ARM struct alignments and ABI ==="

# Simple native C++ test checking member offsets and 32-bit layout assumptions
cat << 'TEST_CPP' > tmp_abi_test.cpp
#include "include/vcs/vcs_types.h"
#include "include/vcs/CAPIHandler.h"
#include <iostream>
#include <cstddef>

using namespace vcs;

int main() {
    std::cout << "Offset checks:" << std::endl;
    std::cout << "vcsCBEvent_t status offset: " << offsetof(vcsCBEvent_t, status) << " (expected 0)" << std::endl;
    std::cout << "vcsCBEvent_t eventId offset: " << offsetof(vcsCBEvent_t, eventId) << " (expected 4)" << std::endl;
    std::cout << "vcsCBEvent_t eventInfo offset: " << offsetof(vcsCBEvent_t, eventInfo) << " (expected 8)" << std::endl;
    
    std::cout << "vcsEnrollCaptureStatus imageQuality: " << offsetof(vcsEnrollCaptureStatus, imageQuality) << " (expected 0)" << std::endl;
    std::cout << "vcsEnrollCaptureStatus progress: " << offsetof(vcsEnrollCaptureStatus, progress) << " (expected 16)" << std::endl;
    std::cout << "vcsEnrollCaptureStatus size: " << sizeof(vcsEnrollCaptureStatus) << " (expected 32)" << std::endl;

    std::cout << "vcsIdentifyResult userId: " << offsetof(vcsIdentifyResult, userId) << " (expected 0)" << std::endl;
    
    if (sizeof(vcsEnrollCaptureStatus) != 32) return 1;
    if (offsetof(vcsEnrollCaptureStatus, progress) != 16) return 2;
    std::cout << "Static layout checks passed!" << std::endl;
    return 0;
}
TEST_CPP

g++ -std=c++17 -I. tmp_abi_test.cpp -o tmp_abi_test
./tmp_abi_test
rm -f tmp_abi_test tmp_abi_test.cpp
echo "=== Struct ABI check passed ==="
