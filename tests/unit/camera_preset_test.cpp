#include "../engine/include/camera_controller.h"
#include "../engine/include/scene_manager.h"
#include "../engine/include/config_defaults.h"
#include <iostream>
#include <cmath>
#include <cassert>

// Simple test framework
#define TEST(name) \
    void test_##name(); \
    void test_##name()

#define ASSERT_NEAR(a, b, tolerance) \
    if (std::abs(a - b) > tolerance) { \
        std::cerr << "Assertion failed: " << a << " != " << b << " (tolerance: " << tolerance << ")" << std::endl; \
        exit(1); \
    }

#define ASSERT_VEC3_NEAR(a, b, tolerance) \
    ASSERT_NEAR(a.x, b.x, tolerance); \
    ASSERT_NEAR(a.y, b.y, tolerance); \
    ASSERT_NEAR(a.z, b.z, tolerance);

TEST(preset_vectors_are_deterministic)
{
    CameraController camera1, camera2;
    SceneManager scene;
    
    // Create a simple test scene with a cube
    scene.loadObject("test_cube", "assets/models/cube.obj", glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
    
    const float tolerance = 0.001f;
    
    // Test that same preset yields same results
    camera1.setCameraPreset(CameraPreset::Front, scene, glm::vec3(0, 0, 0), Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    camera2.setCameraPreset(CameraPreset::Front, scene, glm::vec3(0, 0, 0), Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    
    ASSERT_VEC3_NEAR(camera1.getCameraState().position, camera2.getCameraState().position, tolerance);
    ASSERT_VEC3_NEAR(camera1.getCameraState().front, camera2.getCameraState().front, tolerance);
    ASSERT_VEC3_NEAR(camera1.getCameraState().up, camera2.getCameraState().up, tolerance);
    ASSERT_NEAR(camera1.getCameraState().fov, camera2.getCameraState().fov, tolerance);
    
    std::cout << "✓ Preset vectors are deterministic" << std::endl;
}

TEST(preset_orientations_are_correct)
{
    CameraController camera;
    SceneManager scene;
    
    // Create test scene
    scene.loadObject("test_cube", "assets/models/cube.obj", glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
    
    const float tolerance = 0.1f;
    const glm::vec3 target(0, 0, 0);
    
    // Test Front preset (should look toward negative Z)
    camera.setCameraPreset(CameraPreset::Front, scene, target, Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().front.z, -1.0f, tolerance);
    std::cout << "✓ Front preset orientation correct" << std::endl;
    
    // Test Back preset (should look toward positive Z)
    camera.setCameraPreset(CameraPreset::Back, scene, target, Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().front.z, 1.0f, tolerance);
    std::cout << "✓ Back preset orientation correct" << std::endl;
    
    // Test Left preset (should look toward positive X)
    camera.setCameraPreset(CameraPreset::Left, scene, target, Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().front.x, 1.0f, tolerance);
    std::cout << "✓ Left preset orientation correct" << std::endl;
    
    // Test Right preset (should look toward negative X)
    camera.setCameraPreset(CameraPreset::Right, scene, target, Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().front.x, -1.0f, tolerance);
    std::cout << "✓ Right preset orientation correct" << std::endl;
    
    // Test Top preset (should look down toward negative Y)
    camera.setCameraPreset(CameraPreset::Top, scene, target, Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().front.y, -1.0f, tolerance);
    std::cout << "✓ Top preset orientation correct" << std::endl;
    
    // Test Bottom preset (should look up toward positive Y)
    camera.setCameraPreset(CameraPreset::Bottom, scene, target, Defaults::CameraPresetFovDeg, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().front.y, 1.0f, tolerance);
    std::cout << "✓ Bottom preset orientation correct" << std::endl;
}

TEST(hotkey_mapping_consistency)
{
    // Test that hotkey mappings are consistent
    assert(CameraController::presetFromHotkey(1) == CameraPreset::Front);
    assert(CameraController::presetFromHotkey(2) == CameraPreset::Back);
    assert(CameraController::presetFromHotkey(3) == CameraPreset::Left);
    assert(CameraController::presetFromHotkey(4) == CameraPreset::Right);
    assert(CameraController::presetFromHotkey(5) == CameraPreset::Top);
    assert(CameraController::presetFromHotkey(6) == CameraPreset::Bottom);
    assert(CameraController::presetFromHotkey(7) == CameraPreset::IsoFL);
    assert(CameraController::presetFromHotkey(8) == CameraPreset::IsoBR);
    
    std::cout << "✓ Hotkey mappings are consistent (1-8)" << std::endl;
}

TEST(preset_names_are_correct)
{
    // Test that preset names match expected values
    assert(std::string(CameraController::presetName(CameraPreset::Front)) == "Front");
    assert(std::string(CameraController::presetName(CameraPreset::Back)) == "Back");
    assert(std::string(CameraController::presetName(CameraPreset::Left)) == "Left");
    assert(std::string(CameraController::presetName(CameraPreset::Right)) == "Right");
    assert(std::string(CameraController::presetName(CameraPreset::Top)) == "Top");
    assert(std::string(CameraController::presetName(CameraPreset::Bottom)) == "Bottom");
    assert(std::string(CameraController::presetName(CameraPreset::IsoFL)) == "Iso Front-Left");
    assert(std::string(CameraController::presetName(CameraPreset::IsoBR)) == "Iso Back-Right");
    
    std::cout << "✓ Preset names are correct" << std::endl;
}

TEST(fov_and_margin_parameters)
{
    CameraController camera;
    SceneManager scene;
    
    // Create test scene
    scene.loadObject("test_cube", "assets/models/cube.obj", glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
    
    // Test custom FOV
    camera.setCameraPreset(CameraPreset::Front, scene, glm::vec3(0, 0, 0), 60.0f, Defaults::CameraPresetMargin);
    ASSERT_NEAR(camera.getCameraState().fov, 60.0f, 0.001f);
    
    // Test that different margins produce different distances but same direction
    CameraController camera1, camera2;
    camera1.setCameraPreset(CameraPreset::Front, scene, glm::vec3(0, 0, 0), Defaults::CameraPresetFovDeg, 0.2f);
    camera2.setCameraPreset(CameraPreset::Front, scene, glm::vec3(0, 0, 0), Defaults::CameraPresetFovDeg, 0.3f);
    
    // Same direction
    ASSERT_VEC3_NEAR(camera1.getCameraState().front, camera2.getCameraState().front, 0.01f);
    
    // Different distances (camera2 should be further away)
    float dist1 = glm::length(camera1.getCameraState().position);
    float dist2 = glm::length(camera2.getCameraState().position);
    assert(dist2 > dist1);
    
    std::cout << "✓ FOV and margin parameters work correctly" << std::endl;
}

int main()
{
    std::cout << "Running Camera Preset Tests..." << std::endl;
    
    try {
        test_preset_vectors_are_deterministic();
        test_preset_orientations_are_correct(); 
        test_hotkey_mapping_consistency();
        test_preset_names_are_correct();
        test_fov_and_margin_parameters();
        
        std::cout << "\n✅ All camera preset tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
