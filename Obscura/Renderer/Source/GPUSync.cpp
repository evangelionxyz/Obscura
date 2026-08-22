#include "GPUSync.hpp"
#include <Obscura/Logger.hpp>

namespace Obscura
{
    void RenderCommandQueue::PushCommand(std::function<void(IRHI*)> command)
    {
        if (command)
        {
            m_StagingPacket.commands.push_back(std::move(command));
        }
    }

    void RenderCommandQueue::SubmitPacket(std::uint32_t frameNumber, float deltaTime)
    {
        m_StagingPacket.frameNumber = frameNumber;
        m_StagingPacket.deltaTime = deltaTime;

        auto& writeBuffer = m_TripleBuffer.GetWriteBuffer();
        writeBuffer.frameNumber = m_StagingPacket.frameNumber;
        writeBuffer.deltaTime = m_StagingPacket.deltaTime;
        writeBuffer.commands = std::move(m_StagingPacket.commands);

        m_TripleBuffer.PublishWriteBuffer();
        m_StagingPacket.commands.clear();
    }

    bool RenderCommandQueue::ExecutePendingPacket(IRHI* rhi)
    {
        if (!rhi) return false;

        RenderFramePacket* packet = nullptr;
        bool hasNew = m_TripleBuffer.AcquireLatestReadBuffer(packet);
        if (!hasNew || !packet || packet->commands.empty())
        {
            return false;
        }

        for (auto& cmd : packet->commands)
        {
            try
            {
                cmd(rhi);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("[RenderCommandQueue] Exception in render command: {}", e.what());
            }
        }

        packet->commands.clear();
        return true;
    }

    std::size_t RenderCommandQueue::GetPendingCommandCount() const noexcept
    {
        return m_StagingPacket.commands.size();
    }
}
