#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>

// Include the path security header
#include "../engine/include/path_security.h"

namespace fs = std::filesystem;

class PathSecurityTest {
public:
    static void runAllTests() {
        std::cout << "Running Path Security Tests...\n";
        
        testBasicValidation();
        testTraversalAttempts();
        testAbsolutePathHandling();
        testEdgeCases();
        testAssetRootConfiguration();
        
        std::cout << "All Path Security Tests Passed!\n";
    }

private:
    static void testBasicValidation() {
        std::cout << "  Testing basic validation...\n";
        
        // Create a temporary test directory
        fs::path tempDir = fs::temp_directory_path() / "glint_test_assets";
        fs::create_directories(tempDir);
        
        // Set asset root
        assert(PathSecurity::setAssetRoot(tempDir.string()));
        assert(PathSecurity::isAssetRootSet());
        assert(PathSecurity::getAssetRoot() == tempDir.string());
        
        // Test valid paths
        assert(PathSecurity::validatePath("model.obj") == PathSecurity::ValidationResult::Valid);
        assert(PathSecurity::validatePath("textures/diffuse.png") == PathSecurity::ValidationResult::Valid);
        assert(PathSecurity::validatePath("models/character.fbx") == PathSecurity::ValidationResult::Valid);
        
        // Clean up
        fs::remove_all(tempDir);
        PathSecurity::clearAssetRoot();
    }
    
    static void testTraversalAttempts() {
        std::cout << "  Testing path traversal attempts...\n";
        
        // Create a temporary test directory
        fs::path tempDir = fs::temp_directory_path() / "glint_test_assets";
        fs::create_directories(tempDir);
        PathSecurity::setAssetRoot(tempDir.string());
        
        // Test various traversal attempts
        assert(PathSecurity::validatePath("../../../etc/passwd") == PathSecurity::ValidationResult::InvalidTraversal);
        assert(PathSecurity::validatePath("..\\..\\windows\\system32\\config\\sam") == PathSecurity::ValidationResult::InvalidTraversal);
        assert(PathSecurity::validatePath("models/../../../secret.txt") == PathSecurity::ValidationResult::InvalidTraversal);
        assert(PathSecurity::validatePath("textures/..\\..\\config.ini") == PathSecurity::ValidationResult::InvalidTraversal);
        assert(PathSecurity::validatePath("..") == PathSecurity::ValidationResult::InvalidTraversal);
        assert(PathSecurity::validatePath("../") == PathSecurity::ValidationResult::InvalidTraversal);
        assert(PathSecurity::validatePath("folder/../../other") == PathSecurity::ValidationResult::InvalidTraversal);
        
        // Test URL-encoded traversal attempts
        assert(PathSecurity::validatePath("%2e%2e%2f") == PathSecurity::ValidationResult::InvalidCharacters);
        
        // Clean up
        fs::remove_all(tempDir);
        PathSecurity::clearAssetRoot();
    }
    
    static void testAbsolutePathHandling() {
        std::cout << "  Testing absolute path handling...\n";
        
        // Create a temporary test directory
        fs::path tempDir = fs::temp_directory_path() / "glint_test_assets";
        fs::create_directories(tempDir);
        PathSecurity::setAssetRoot(tempDir.string());
        
        // Test absolute paths within the asset root
        fs::path validAbsolute = tempDir / "model.obj";
        assert(PathSecurity::validatePath(validAbsolute.string()) == PathSecurity::ValidationResult::Valid);
        
        // Test absolute paths outside the asset root
        fs::path invalidAbsolute = fs::temp_directory_path() / "malicious.exe";
        assert(PathSecurity::validatePath(invalidAbsolute.string()) == PathSecurity::ValidationResult::OutsideRoot);
        
        // Test system paths
        #ifdef _WIN32
        assert(PathSecurity::validatePath("C:\\Windows\\System32\\calc.exe") == PathSecurity::ValidationResult::OutsideRoot);
        #else
        assert(PathSecurity::validatePath("/etc/passwd") == PathSecurity::ValidationResult::OutsideRoot);
        #endif
        
        // Clean up
        fs::remove_all(tempDir);
        PathSecurity::clearAssetRoot();
    }
    
    static void testEdgeCases() {
        std::cout << "  Testing edge cases...\n";
        
        // Create a temporary test directory
        fs::path tempDir = fs::temp_directory_path() / "glint_test_assets";
        fs::create_directories(tempDir);
        PathSecurity::setAssetRoot(tempDir.string());
        
        // Test empty paths
        assert(PathSecurity::validatePath("") == PathSecurity::ValidationResult::EmptyPath);
        
        // Test paths with control characters
        std::string pathWithControlChars = "model\x01.obj";
        assert(PathSecurity::validatePath(pathWithControlChars) == PathSecurity::ValidationResult::InvalidCharacters);
        
        // Test paths with null bytes
        std::string pathWithNull = std::string("model") + '\0' + ".obj";
        assert(PathSecurity::validatePath(pathWithNull) == PathSecurity::ValidationResult::InvalidCharacters);
        
        // Test very long paths
        std::string longPath(5000, 'a');
        longPath += ".obj";
        // This might be valid or invalid depending on filesystem limits, but shouldn't crash
        PathSecurity::ValidationResult longResult = PathSecurity::validatePath(longPath);
        assert(longResult == PathSecurity::ValidationResult::Valid || 
               longResult == PathSecurity::ValidationResult::InvalidCharacters);
        
        // Clean up
        fs::remove_all(tempDir);
        PathSecurity::clearAssetRoot();
    }
    
    static void testAssetRootConfiguration() {
        std::cout << "  Testing asset root configuration...\n";
        
        // Test with no asset root set
        PathSecurity::clearAssetRoot();
        assert(!PathSecurity::isAssetRootSet());
        assert(PathSecurity::validatePath("model.obj") == PathSecurity::ValidationResult::RootNotSet);
        
        // Test setting invalid asset root
        assert(!PathSecurity::setAssetRoot("/non/existent/directory/12345"));
        assert(!PathSecurity::isAssetRootSet());
        
        // Test setting empty asset root
        assert(!PathSecurity::setAssetRoot(""));
        
        // Test setting valid asset root
        fs::path tempDir = fs::temp_directory_path() / "glint_test_assets";
        fs::create_directories(tempDir);
        assert(PathSecurity::setAssetRoot(tempDir.string()));
        assert(PathSecurity::isAssetRootSet());
        
        // Clean up
        fs::remove_all(tempDir);
        PathSecurity::clearAssetRoot();
    }
};

// Test with JSON Ops integration
void testJsonOpsIntegration() {
    std::cout << "  Testing JSON Ops integration...\n";
    
    // This would require a full application setup, so we'll just test that our
    // path validation functions work correctly when called directly
    
    fs::path tempDir = fs::temp_directory_path() / "glint_test_assets";
    fs::create_directories(tempDir);
    PathSecurity::setAssetRoot(tempDir.string());
    
    // Create a test file
    fs::path testFile = tempDir / "test_model.obj";
    std::ofstream(testFile) << "# Test OBJ file\n";
    
    // Test that valid paths resolve correctly
    std::string resolved = PathSecurity::resolvePath("test_model.obj");
    assert(!resolved.empty());
    assert(fs::equivalent(resolved, testFile));
    
    // Test that invalid paths don't resolve
    assert(PathSecurity::resolvePath("../../../etc/passwd").empty());
    
    // Clean up
    fs::remove_all(tempDir);
    PathSecurity::clearAssetRoot();
}

int main() {
    try {
        PathSecurityTest::runAllTests();
        testJsonOpsIntegration();
        std::cout << "\n🛡️  All security tests passed successfully!\n";
        std::cout << "Path traversal protection is working correctly.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}