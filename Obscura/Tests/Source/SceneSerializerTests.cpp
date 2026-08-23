#include <gtest/gtest.h>
#include <Scene/Scene.hpp>
#include <Assets/Serializer.hpp>
#include <filesystem>
#include <fstream>

TEST(SceneSerializerTests, StringSerializationAndDeserialization)
{
    Obscura::Scene originalScene;

    // Create Root Entity
    auto rootEntity = originalScene.CreateEntityWithUUID(1001, "Player", 0);
    auto& rootTransform = originalScene.GetComponent<Obscura::Transform>(rootEntity);
    rootTransform.position = { 10.5f, -2.0f, 3.14f };
    rootTransform.rotation = { 0.1f, 0.2f, 0.3f };
    rootTransform.scale    = { 2.0f, 2.0f, 1.0f };

    auto& rootSprite = originalScene.AddComponent<Obscura::Sprite2D>(rootEntity);
    rootSprite.color = { 0.8f, 0.2f, 0.4f, 1.0f };
    rootSprite.uvOffset = { 0.25f, 0.5f };
    rootSprite.uvScale = { 0.5f, 0.5f };
    rootSprite.texturePath = "Assets/Textures/Player.png";
    rootSprite.visible = true;

    // Create Child Entity
    auto childEntity = originalScene.CreateEntityWithUUID(1002, "Weapon", 1001);
    auto& childTransform = originalScene.GetComponent<Obscura::Transform>(childEntity);
    childTransform.position = { 1.0f, 0.5f, 0.0f };

    auto& childSprite = originalScene.AddComponent<Obscura::Sprite2D>(childEntity);
    childSprite.color = { 1.0f, 1.0f, 0.0f, 0.9f };
    childSprite.visible = false;

    // Create Empty Entity
    originalScene.CreateEntityWithUUID(1003, "Marker", 0);

    // Serialize
    Obscura::SceneSerializer serializer(&originalScene);
    std::string jsonString;
    ASSERT_TRUE(serializer.SerializeToString(jsonString));
    EXPECT_FALSE(jsonString.empty());

    // Deserialize into fresh Scene
    Obscura::Scene loadedScene;
    Obscura::SceneSerializer deserializer(&loadedScene);
    ASSERT_TRUE(deserializer.DeserializeFromString(jsonString));

    // Verify Root Entity
    auto foundRoot = loadedScene.FindEntityByUUID(1001);
    ASSERT_NE(foundRoot, entt::null);
    EXPECT_EQ(loadedScene.GetComponent<Obscura::IDComponent>(foundRoot).name, "Player");
    EXPECT_EQ(loadedScene.GetComponent<Obscura::IDComponent>(foundRoot).parentUuid, 0u);

    const auto& loadedRootTf = loadedScene.GetComponent<Obscura::Transform>(foundRoot);
    EXPECT_FLOAT_EQ(loadedRootTf.position.x, 10.5f);
    EXPECT_FLOAT_EQ(loadedRootTf.position.y, -2.0f);
    EXPECT_FLOAT_EQ(loadedRootTf.position.z, 3.14f);
    EXPECT_FLOAT_EQ(loadedRootTf.rotation.x, 0.1f);
    EXPECT_FLOAT_EQ(loadedRootTf.rotation.y, 0.2f);
    EXPECT_FLOAT_EQ(loadedRootTf.rotation.z, 0.3f);
    EXPECT_FLOAT_EQ(loadedRootTf.scale.x, 2.0f);

    ASSERT_TRUE(loadedScene.HasComponent<Obscura::Sprite2D>(foundRoot));
    const auto& loadedRootSprite = loadedScene.GetComponent<Obscura::Sprite2D>(foundRoot);
    EXPECT_FLOAT_EQ(loadedRootSprite.color.r, 0.8f);
    EXPECT_FLOAT_EQ(loadedRootSprite.color.g, 0.2f);
    EXPECT_FLOAT_EQ(loadedRootSprite.color.b, 0.4f);
    EXPECT_FLOAT_EQ(loadedRootSprite.color.a, 1.0f);
    EXPECT_FLOAT_EQ(loadedRootSprite.uvOffset.x, 0.25f);
    EXPECT_FLOAT_EQ(loadedRootSprite.uvOffset.y, 0.5f);
    EXPECT_FLOAT_EQ(loadedRootSprite.uvScale.x, 0.5f);
    EXPECT_FLOAT_EQ(loadedRootSprite.uvScale.y, 0.5f);
    EXPECT_EQ(loadedRootSprite.texturePath, "Assets/Textures/Player.png");
    EXPECT_TRUE(loadedRootSprite.visible);

    // Verify Child Entity
    auto foundChild = loadedScene.FindEntityByUUID(1002);
    ASSERT_NE(foundChild, entt::null);
    EXPECT_EQ(loadedScene.GetComponent<Obscura::IDComponent>(foundChild).name, "Weapon");
    EXPECT_EQ(loadedScene.GetComponent<Obscura::IDComponent>(foundChild).parentUuid, 1001u);

    const auto& loadedChildSprite = loadedScene.GetComponent<Obscura::Sprite2D>(foundChild);
    EXPECT_FLOAT_EQ(loadedChildSprite.color.r, 1.0f);
    EXPECT_FLOAT_EQ(loadedChildSprite.color.g, 1.0f);
    EXPECT_FLOAT_EQ(loadedChildSprite.color.b, 0.0f);
    EXPECT_FLOAT_EQ(loadedChildSprite.color.a, 0.9f);
    EXPECT_FALSE(loadedChildSprite.visible);

    // Verify Empty Entity
    auto foundMarker = loadedScene.FindEntityByUUID(1003);
    ASSERT_NE(foundMarker, entt::null);
    EXPECT_EQ(loadedScene.GetComponent<Obscura::IDComponent>(foundMarker).name, "Marker");
    EXPECT_FALSE(loadedScene.HasComponent<Obscura::Sprite2D>(foundMarker));
}

TEST(SceneSerializerTests, FileSerializationAndDeserialization)
{
    std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / "obscura_test_scene.json";

    Obscura::Scene originalScene;
    auto e1 = originalScene.CreateEntityWithUUID(5001, "TestQuad", 0);
    auto& sp = originalScene.AddComponent<Obscura::Sprite2D>(e1);
    sp.color = { 0.1f, 0.2f, 0.3f, 0.4f };

    Obscura::SceneSerializer serializer(&originalScene);
    ASSERT_TRUE(serializer.Serialize(tempFilePath));
    EXPECT_TRUE(std::filesystem::exists(tempFilePath));

    Obscura::Scene loadedScene;
    Obscura::SceneSerializer deserializer(&loadedScene);
    ASSERT_TRUE(deserializer.Deserialize(tempFilePath));

    auto found = loadedScene.FindEntityByUUID(5001);
    ASSERT_NE(found, entt::null);
    EXPECT_EQ(loadedScene.GetComponent<Obscura::IDComponent>(found).name, "TestQuad");
    ASSERT_TRUE(loadedScene.HasComponent<Obscura::Sprite2D>(found));
    EXPECT_FLOAT_EQ(loadedScene.GetComponent<Obscura::Sprite2D>(found).color.r, 0.1f);

    std::filesystem::remove(tempFilePath);
}
