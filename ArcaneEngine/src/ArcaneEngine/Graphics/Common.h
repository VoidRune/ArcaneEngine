#pragma once

namespace Arc
{
    enum class PresentMode
    {
        Immediate = 0,
        Mailbox = 1,
        Fifo = 2,
        FifoRelaxed = 3,
    };

    enum class BufferUsage
    {
        TransferSrc = 0x00000001,
        TransferDst = 0x00000002,
        UniformTexelBuffer = 0x00000004,
        StorageTexelBuffer = 0x00000008,
        UniformBuffer = 0x00000010,
        StorageBuffer = 0x00000020,
        IndexBuffer = 0x00000040,
        VertexBuffer = 0x00000080,
        IndirectBuffer = 0x00000100,
        ShaderDeviceAddress = 0x00020000,
        AccelerationStructureBuildInputReadOnly = 0x00080000,
        AccelerationStructureStorage = 0x00100000,
    };
    inline BufferUsage operator|(BufferUsage a, BufferUsage b)
    {
        return static_cast<BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum struct MemoryProperty
    {
        DeviceLocal = 0x00000001,
        HostVisible = 0x00000002,
        HostCoherent = 0x00000004,
        HostCached = 0x00000008,
        LazilyAllocated = 0x00000010,
    };
    inline MemoryProperty operator|(MemoryProperty a, MemoryProperty b)
    {
        return static_cast<MemoryProperty>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum class Format
    {
        Undefined = 0,
        R8_Unorm = 9,
        R8_Snorm = 10,
        R8_Uint = 13,
        R8_Sint = 14,
        R8_Srgb = 15,
        R8G8_Unorm = 16,
        R8G8_Snorm = 17,
        R8G8_Uint = 20,
        R8G8_Sint = 21,
        R8G8_Srgb = 22, 
        R8G8B8_Unorm = 23,
        R8G8B8_Snorm = 24,
        R8G8B8_Uint = 27,
        R8G8B8_Sint = 28,
        R8G8B8_Srgb = 29,
        B8G8R8_Unorm = 30,
        B8G8R8_Snorm = 31,
        B8G8R8_Uint = 34,
        B8G8R8_Sint = 35,
        B8G8R8_Srgb = 36,
        R8G8B8A8_Unorm = 37,
        R8G8B8A8_Snorm = 38,
        R8G8B8A8_Uint = 41,
        R8G8B8A8_Sint = 42,
        R8G8B8A8_Srgb = 43,
        B8G8R8A8_Unorm = 44,
        B8G8R8A8_Snorm = 45,
        B8G8R8A8_Uint = 48,
        B8G8R8A8_Sint = 49,
        B8G8R8A8_Srgb = 50,
        R16_Unorm = 70,
        R16_Snorm = 71,
        R16_Uint = 74,
        R16_Sint = 75,
        R16_Sfloat = 76,
        R16G16_Unorm = 77,
        R16G16_Snorm = 78,
        R16G16_Uint = 81,
        R16G16_Sint = 82,
        R16G16_Sfloat = 83,
        R16G16B16_Unorm = 84,
        R16G16B16_Snorm = 85,
        R16G16B16_Uint = 88,
        R16G16B16_Sint = 89,
        R16G16B16_Sfloat = 90,
        R16G16B16A16_Unorm = 91,
        R16G16B16A16_Snorm = 92,
        R16G16B16A16_Uint = 95,
        R16G16B16A16_Sint = 96,
        R16G16B16A16_Sfloat = 97,
        R32_Uint = 98,
        R32_Sint = 99,
        R32_Sfloat = 100,
        R32G32_Uint = 101,
        R32G32_Sint = 102,
        R32G32_Sfloat = 103,
        R32G32B32_Uint = 104,
        R32G32B32_Sint = 105,
        R32G32B32_Sfloat = 106,
        R32G32B32A32_Uint = 107,
        R32G32B32A32_Sint = 108,
        R32G32B32A32_Sfloat = 109,
        R64_Uint = 110,
        R64_Sint = 111,
        R64_Sfloat = 112,
        R64G64_Uint = 113,
        R64G64_Sint = 114,
        R64G64_Sfloat = 115,
        R64G64B64_Uint = 116,
        R64G64B64_Sint = 117,
        R64G64B64_Sfloat = 118,
        R64G64B64A64_Uint = 119,
        R64G64B64A64_Sint = 120,
        R64G64B64A64_Sfloat = 121,
        D16_Unorm = 124,
        D32_Sfloat = 126,
        S8_Uint = 127,
        D16_Unorm_S8_Uint = 128,
        D24_Unorm_S8_Uint = 129,
        D32_Sfloat_S8_Uint = 130,
    };

    
    enum class ImageAspect
    {
        Color = 0x00000001,
        Depth = 0x00000002,
        Stencil = 0x00000004,
    };
    inline ImageAspect operator|(ImageAspect a, ImageAspect b)
    {
        return static_cast<ImageAspect>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum class ImageLayout
    {
        Undefined = 0,
        General = 1,
        ColorAttachmentOptimal = 2,
        DepthStencilAttachmentOptimal = 3,
        DepthStencilReadOnlyOptimal = 4,
        ShaderReadOnlyOptimal = 5,
        TransferSrcOptimal = 6,
        TransferDstOptimal = 7,
        Preinitialized = 8,
        DepthReadOnlyStencilAttachmentOptimal = 1000117000,
        DepthAttachmentStencilReadOnlyOptimal = 1000117001,
        DepthAttachmentOptimal = 1000241000,
        DepthReadOnlyOptimal = 1000241001,
        StencilAttachmentOptimal = 1000241002,
        StencilReadOnlyOptimal = 1000241003,
        ReadOnlyOptimal = 1000314000,
        AttachmentOptimal = 1000314001,
        PresentSrc = 1000001002,
    };

    enum class ImageUsage
    {
        TransferSrc = 0x00000001,
        TransferDst = 0x00000002,
        Sampled = 0x00000004,
        Storage = 0x00000008,
        ColorAttachment = 0x00000010,
        DepthStencilAttachment = 0x00000020,
        TransientAttachment = 0x00000040,
        InputAttachment = 0x00000080,
    };
    inline ImageUsage operator|(ImageUsage a, ImageUsage b)
    {
        return static_cast<ImageUsage>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum class ShaderStage
    {
        Vertex = 0x00000001,
        Fragment = 0x00000010,
        Compute = 0x00000020,
        RayGen = 0x00000100,
        RayAnyHit = 0x00000200,
        RayClosestHit = 0x00000400,
        RayMiss = 0x00000800,
    };
    inline ShaderStage operator|(ShaderStage a, ShaderStage b)
    {
        return static_cast<ShaderStage>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum class DescriptorType
    {
        Sampler = 0,
        CombinedImageSampler = 1,
        SampledImage = 2,
        StorageImage = 3,
        UniformTexelBuffer = 4,
        StorageTexelBuffer = 5,
        UniformBuffer = 6,
        StorageBuffer = 7,
        UniformBufferDynamic = 8,
        StorageBufferDynamic = 9,
        InputAttachment = 10,
        InlineUniformBlock = 1000138000,
        AccelerationStructure = 1000150000,
    };

    enum class DescriptorFlag
    {
        None = 0,
        Bindless = 1,
    };

    enum class PrimitiveTopology
    {
        PointList = 0,
        LineList = 1,
        LineStrip = 2,
        TriangleList = 3,
        TriangleStrip = 4,
        TriangleFan = 5,
        LineListWithAdjacency = 6,
        LineStripWithAdjacency = 7,
        TriangleListWithAdjacency = 8,
        TriangleStripWithAdjacency = 9,
        PatchList = 10,
    };

    enum class CullMode
    {
        None = 0,
        Front = 0x00000001,
        Back = 0x00000002,
        FrontAndBack = 0x00000003,
    };

    enum class CompareOperation
    {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessOrEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterOrEqual = 6,
        Always = 7,
    };

    enum class PipelineBindPoint
    {
        Graphics = 0,
        Compute = 1,
        RayTracing = 1000165000,
    };

    enum class AttachmentLoadOp
    {
        Load = 0,
        Clear = 1,
        DontCare = 2,
    };

    enum class AttachmentStoreOp
    {
        Store = 0,
        DontCare = 1,
    };

    enum class Filter
    {
        Nearest = 0,
        Linear = 1
    };

    enum class SamplerAddressMode
    {
        Repeat = 0,
        MirroredRepeat = 1,
        ClampToEdge = 2,
        ClampToBorder = 3,
        MirrorClampToEdge = 4,
    };

}