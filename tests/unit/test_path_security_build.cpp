/**
 * Simple test to validate path security implementation compiles and works
 * This ensures the implementation is solid before CI runs
 */

#include <iostream>
#include <filesystem>
#include <cassert>

// Include the path security header directly
#include "engine/include/path_security.h"

int main() {
    std::cout << "Testing Path Security Implementation..." << std::endl;
    
    // Test 1: Basic functionality without asset root
    std::cout << "Test 1: No asset root set" << std::endl;
    assert(!PathSecurity::isAssetRootSet());
    assert(PathSecurity::getAssetRoot().empty());
    assert(PathSecurity::validatePath("test.obj") == PathSecurity::ValidationResult::RootNotSet);
    std::cout << "✅ No asset root behavior correct" << std::endl;
    
    // Test 2: Set valid asset root
    std::cout << "Test 2: Set asset root" << std::endl;
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "path_security_test";
    std::filesystem::create_directories(tempDir);
    
    bool rootSet = PathSecurity::setAssetRoot(tempDir.string());
    assert(rootSet);
    assert(PathSecurity::isAssetRootSet());
    assert(!PathSecurity::getAssetRoot().empty());
    std::cout << "✅ Asset root set successfully: " << PathSecurity::getAssetRoot() << std::endl;
    
    // Test 3: Valid path validation
    std::cout << "Test 3: Valid path validation" << std::endl;
    assert(PathSecurity::validatePath("model.obj") == PathSecurity::ValidationResult::Valid);
    assert(PathSecurity::validatePath("textures/diffuse.png") == PathSecurity::ValidationResult::Valid);
    std::cout << "✅ Valid paths accepted" << std::endl;
    
    // Test 4: Path traversal detection
    std::cout << "Test 4: Path traversal detection" << std::endl;
    assert(PathSecurity::validatePath("../../../etc/passwd") == PathSecurity::ValidationResult::InvalidTraversal);
    assert(PathSecurity::validatePath("..\\..\\system32\\config") == PathSecurity::ValidationResult::InvalidTraversal);
    assert(PathSecurity::validatePath("models/../../../secret.txt") == PathSecurity::ValidationResult::InvalidTraversal);
    std::cout << "✅ Path traversal attempts blocked" << std::endl;
    
    // Test 5: Path resolution
    std::cout << "Test 5: Path resolution" << std::endl;
    std::string resolved = PathSecurity::resolvePath("model.obj");
    assert(!resolved.empty());
    std::cout << "✅ Path resolution working: " << resolved << std::endl;
    
    // Test 6: Invalid path resolution should return empty
    std::cout << "Test 6: Invalid path resolution" << std::endl;
    std::string invalidResolved = PathSecurity::resolvePath("../../../malicious.exe");
    assert(invalidResolved.empty());
    std::cout << "✅ Invalid path resolution returns empty" << std::endl;
    
    // Test 7: Error messages
    std::cout << "Test 7: Error messages" << std::endl;
    std::string errorMsg = PathSecurity::getErrorMessage(PathSecurity::ValidationResult::InvalidTraversal);
    assert(!errorMsg.empty());
    assert(errorMsg.find("traversal") != std::string::npos);
    std::cout << "✅ Error message: " << errorMsg << std::endl;
    
    // Test 8: Clear asset root
    std::cout << "Test 8: Clear asset root" << std::endl;
    PathSecurity::clearAssetRoot();
    assert(!PathSecurity::isAssetRootSet());
    assert(PathSecurity::getAssetRoot().empty());
    std::cout << "✅ Asset root cleared" << std::endl;
    
    // Cleanup
    std::filesystem::remove_all(tempDir);
    
    std::cout << "\n🎉 All Path Security tests passed!" << std::endl;
    std::cout << "Implementation is solid and ready for CI." << std::endl;
    
    return 0;
}