#include <gtest/gtest.h>
#include <Scene/Scene.hpp>
#include <unordered_set>
#include <cstdint>

TEST(SceneECSTest, EntityLifecycleAndComponents)
{
    Obscura::Scene scene;
    EXPECT_TRUE(scene.IsEmpty());

    auto e1 = scene.CreateEntity("Player");
    auto e2 = scene.CreateEntity("Enemy");

    EXPECT_FALSE(scene.IsEmpty());
    EXPECT_TRUE(scene.HasComponent<Obscura::IDComponent>(e1));
    EXPECT_TRUE(scene.HasComponent<Obscura::TagComponent>(e1));
    EXPECT_TRUE(scene.HasComponent<Obscura::Transform>(e1));
    EXPECT_EQ(scene.GetComponent<Obscura::TagComponent>(e1).tag, "Player");
    EXPECT_EQ(scene.GetComponent<Obscura::IDComponent>(e1).name, "Player");
    EXPECT_NE(scene.GetComponent<Obscura::IDComponent>(e1).uuid, 0u);
    EXPECT_NE(scene.GetComponent<Obscura::IDComponent>(e1).uuid, scene.GetComponent<Obscura::IDComponent>(e2).uuid);

    // Add Sprite2D
    auto& sprite = scene.AddComponent<Obscura::Sprite2D>(e1);
    sprite.color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);
    sprite.textureHandle = 0xABCD1234;
    sprite.texturePath = "test/path.png";

    EXPECT_TRUE(scene.HasComponent<Obscura::Sprite2D>(e1));
    EXPECT_FALSE(scene.HasComponent<Obscura::Sprite2D>(e2));

    const auto& s = scene.GetComponent<Obscura::Sprite2D>(e1);
    EXPECT_EQ(s.textureHandle, 0xABCD1234u);
    EXPECT_EQ(s.texturePath, "test/path.png");
    EXPECT_FLOAT_EQ(s.color.r, 1.0f);
    EXPECT_FLOAT_EQ(s.color.g, 0.5f);

    scene.DestroyEntity(e2);
    EXPECT_FALSE(scene.GetRegistry().valid(e2));

    scene.Clear();
    EXPECT_TRUE(scene.IsEmpty());
}

TEST(SceneECSTest, IDComponentAndUUIDUniqueness)
{
    Obscura::Scene scene;
    std::unordered_set<uint64_t> uuids;

    for (int i = 0; i < 100; ++i)
    {
        auto entity = scene.CreateEntity("Node_" + std::to_string(i));
        ASSERT_TRUE(scene.HasComponent<Obscura::IDComponent>(entity));
        const auto& idComp = scene.GetComponent<Obscura::IDComponent>(entity);
        EXPECT_NE(idComp.uuid, 0u);
        EXPECT_EQ(idComp.parentUuid, 0u);
        EXPECT_EQ(idComp.name, "Node_" + std::to_string(i));
        EXPECT_TRUE(uuids.insert(idComp.uuid).second); // Must be uniquely inserted
    }

    // Test Parent UUID relationship
    auto parent = scene.CreateEntity("ParentNode");
    uint64_t parentUuid = scene.GetComponent<Obscura::IDComponent>(parent).uuid;

    auto child = scene.CreateEntityWithUUID(0, "ChildNode", parentUuid);
    const auto& childId = scene.GetComponent<Obscura::IDComponent>(child);
    EXPECT_EQ(childId.parentUuid, parentUuid);

    // Test FindEntityByUUID
    EXPECT_EQ(scene.FindEntityByUUID(parentUuid), parent);
    EXPECT_EQ(scene.FindEntityByUUID(childId.uuid), child);
    EXPECT_EQ(scene.FindEntityByUUID(0xDEADBEEF), entt::null);
}

TEST(SceneECSTest, TransformMatrixComputation)
{
    Obscura::Transform transform;
    transform.position = glm::vec3(10.0f, 20.0f, 30.0f);
    transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.scale    = glm::vec3(2.0f, 3.0f, 4.0f);

    glm::mat4 mat = transform.GetTransformMatrix();
    glm::vec4 localPoint(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 worldPoint = mat * localPoint;

    EXPECT_FLOAT_EQ(worldPoint.x, 12.0f); // 1*2 + 10
    EXPECT_FLOAT_EQ(worldPoint.y, 23.0f); // 1*3 + 20
    EXPECT_FLOAT_EQ(worldPoint.z, 34.0f); // 1*4 + 30
    EXPECT_FLOAT_EQ(worldPoint.w, 1.0f);
}

TEST(SceneECSTest, EnTTRuntimeViewIteration)
{
    Obscura::Scene scene;
    for (int i = 0; i < 10; ++i)
    {
        auto entity = scene.CreateEntity("Sprite_" + std::to_string(i));
        auto& t = scene.GetComponent<Obscura::Transform>(entity);
        t.position.x = static_cast<float>(i * 10);

        if (i % 2 == 0)
        {
            auto& s = scene.AddComponent<Obscura::Sprite2D>(entity);
            s.textureHandle = static_cast<Obscura::AssetHandle>(i);
        }
    }

    const auto view = scene.GetRegistry().view<Obscura::Transform, Obscura::Sprite2D>();
    size_t count = 0;
    for (auto entity : view)
    {
        const auto& [t, s] = view.get<Obscura::Transform, Obscura::Sprite2D>(entity);
        EXPECT_EQ(s.textureHandle % 2, 0u);
        count++;
    }
    EXPECT_EQ(count, 5u);
}
