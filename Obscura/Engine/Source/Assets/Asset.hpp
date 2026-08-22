#pragma once

#include <Obscura/API.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Obscura
{
    enum class AssetType : std::uint8_t
    {
        None = 0,
        Texture2D,
        Shader,
        Mesh,
        Material
    };

    enum class AssetState : std::uint8_t
    {
        Unloaded = 0,
        Loading,
        Ready,
        Failed
    };

    struct OBSCURA_ENGINE_API TextureData
    {
        std::vector<std::uint8_t> pixels;
        std::uint32_t             width    = 0;
        std::uint32_t             height   = 0;
        std::uint32_t             channels = 0;
        bool                      isValid  = false;

        [[nodiscard]] std::size_t GetByteSize() const noexcept
        {
            return pixels.size();
        }
    };

    class OBSCURA_ENGINE_API Asset
    {
    public:
        Asset(std::uint64_t uuid, std::string path, AssetType type)
            : m_UUID(uuid), m_Path(std::move(path)), m_Type(type), m_State(AssetState::Unloaded)
        {
        }

        virtual ~Asset() = default;

        [[nodiscard]] std::uint64_t GetUUID() const noexcept { return m_UUID; }
        [[nodiscard]] const std::string& GetPath() const noexcept { return m_Path; }
        [[nodiscard]] AssetType GetType() const noexcept { return m_Type; }
        [[nodiscard]] AssetState GetState() const noexcept { return m_State.load(std::memory_order_relaxed); }
        void SetState(AssetState state) noexcept { m_State.store(state, std::memory_order_relaxed); }

    protected:
        std::uint64_t            m_UUID = 0;
        std::string              m_Path;
        AssetType                m_Type = AssetType::None;
        std::atomic<AssetState>  m_State{AssetState::Unloaded};
    };

    class OBSCURA_ENGINE_API TextureAsset : public Asset
    {
    public:
        TextureAsset(std::uint64_t uuid, std::string path)
            : Asset(uuid, std::move(path), AssetType::Texture2D)
        {
        }

        [[nodiscard]] const TextureData& GetData() const noexcept { return m_Data; }
        void SetData(TextureData data)
        {
            m_Data = std::move(data);
            if (m_Data.isValid)
            {
                SetState(AssetState::Ready);
            }
            else
            {
                SetState(AssetState::Failed);
            }
        }

        [[nodiscard]] std::uint32_t GetGpuSlot() const noexcept { return m_GpuSlot; }
        void SetGpuSlot(std::uint32_t slot) noexcept
        {
            m_GpuSlot = slot;
            m_IsGpuUploaded = true;
        }

        [[nodiscard]] bool IsGpuUploaded() const noexcept { return m_IsGpuUploaded; }

    private:
        TextureData   m_Data;
        std::uint32_t m_GpuSlot = 0;
        bool          m_IsGpuUploaded = false;
    };
}
