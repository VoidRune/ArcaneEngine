#pragma once
#include <vulkan/vulkan.h>

namespace Arc
{
    enum class PresentMode
    {
        Immediate = VK_PRESENT_MODE_IMMEDIATE_KHR,
        Mailbox = VK_PRESENT_MODE_MAILBOX_KHR,
        Fifo = VK_PRESENT_MODE_FIFO_KHR,
        FifoRelaxed = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
    };

    enum class ShaderStage
    {
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute = VK_SHADER_STAGE_COMPUTE_BIT,
        All = VK_SHADER_STAGE_ALL,
        AllGraphics = VK_SHADER_STAGE_ALL_GRAPHICS,
        VertexFragment = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        VertexCompute = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
        FragmentCompute = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
        VertexFragmentCompute = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
    };

    enum class DescriptorType
    {
        Sampler = VK_DESCRIPTOR_TYPE_SAMPLER,
        CombinedImageSampler = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        SampledImage = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        StorageImage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        //UniformTexelBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        //StorageTexelBuffer = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        UniformBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        StorageBuffer = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        //UniformBufferDynamic = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        //StorageBufferDynamic = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
        InputAttachment = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
    };

    enum class DescriptorFlags
    {
        Bindless = 1,
        InFlight = 2,
    };

    enum class Format
    {
        Undefined = VK_FORMAT_UNDEFINED,
        /* Color */
        R8_Unorm = VK_FORMAT_R8_UNORM,
        R16G16B16A16_Sfloat = VK_FORMAT_R16G16B16A16_SFLOAT,
        R32_Sfloat = VK_FORMAT_R32_SFLOAT,
        R32G32_Sfloat = VK_FORMAT_R32G32_SFLOAT,
        R32G32B32_Sfloat = VK_FORMAT_R32G32B32_SFLOAT,
        R32G32B32A32_Sfloat = VK_FORMAT_R32G32B32A32_SFLOAT,
        R8G8_Unorm = VK_FORMAT_R8G8_UNORM,
        R8G8B8A8_Srgb = VK_FORMAT_R8G8B8A8_SRGB,
        B8G8R8A8_Srgb = VK_FORMAT_B8G8R8A8_SRGB,
        R8G8B8A8_Unorm = VK_FORMAT_R8G8B8A8_UNORM,
        R32G32B32_Uint = VK_FORMAT_R32G32B32_UINT,
        R32G32B32A32_Uint = VK_FORMAT_R32G32B32A32_UINT,
        R32G32B32A32_Sint = VK_FORMAT_R32G32B32A32_SINT,
        /* Depth */
        D32_Sfloat = VK_FORMAT_D32_SFLOAT,
        D16_Unorm = VK_FORMAT_D16_UNORM,
    };

    enum class AttachmentLoadOp
    {
        Load = VK_ATTACHMENT_LOAD_OP_LOAD,
        Clear = VK_ATTACHMENT_LOAD_OP_CLEAR,
        DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    };

    enum class AttachmentStoreOp
    {
        Store = VK_ATTACHMENT_STORE_OP_STORE,
        DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    };

    enum class Filter
    {
        Nearest = VK_FILTER_NEAREST,
        Linear = VK_FILTER_LINEAR,
        Cubic = VK_FILTER_CUBIC_IMG,
    };

    enum class SamplerAddressMode
    {
        Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        MirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        ClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        ClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        MirrorClampToEdge = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    };

    enum struct PipelineBindPoint
    {
        Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
        Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
    };

    enum class PipelineStage
    {
        VertexShader = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        FragmentShader = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        ComputeShader = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };

    enum class IndexType
    {
        Uint16 = VK_INDEX_TYPE_UINT16,
        Uint32 = VK_INDEX_TYPE_UINT32,
    };

    enum class AccessFlags
    {
        ShaderRead = VK_ACCESS_SHADER_READ_BIT,
        ShaderWrite = VK_ACCESS_SHADER_WRITE_BIT,
        ColorAttachmentRead = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        ColorAttachmentWrite = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        DepthStencilAttachmentRead = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        DepthStencilAttachmentWrite = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        TransferRead = VK_ACCESS_TRANSFER_READ_BIT,
        TransferWrite = VK_ACCESS_TRANSFER_WRITE_BIT,
        HostRead = VK_ACCESS_HOST_READ_BIT,
        HostWrite = VK_ACCESS_HOST_WRITE_BIT,
        MemoryRead = VK_ACCESS_MEMORY_READ_BIT,
        MemoryWrite = VK_ACCESS_MEMORY_WRITE_BIT,
    };

    enum struct ImageLayout
    {
        Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
        General = VK_IMAGE_LAYOUT_GENERAL,
        ColorAttachmentOptimal = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        DepthStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        DepthStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        ShaderReadOnlyOptimal = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        TransferSrcOptimal = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TransferDstOptimal = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        Preinitialized = VK_IMAGE_LAYOUT_PREINITIALIZED,
        DepthReadOnlyStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        DepthAttachmentStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
        DepthAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        DepthReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
        StencilAttachmentOptimal = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
        StencilReadOnlyOptimal = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
        ReadOnlyOptimal = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        AttachmentOptimal = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    };

    enum struct ImageUsage
    {
        TransferSrc = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        TransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
        Storage = VK_IMAGE_USAGE_STORAGE_BIT,
        ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        DepthStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        TransientAttachment = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        InputAttachment = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
    };


    enum struct ImageAspect
    {
        Color = VK_IMAGE_ASPECT_COLOR_BIT,
        Depth = VK_IMAGE_ASPECT_DEPTH_BIT,
        Stencil = VK_IMAGE_ASPECT_STENCIL_BIT,
    };

    enum struct PrimitiveTopology
    {
        LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        TriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        LineListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        LineStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
        TriangleListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        TriangleStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
        PatchList = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
    };

    enum struct CullMode
    {
        None = VK_CULL_MODE_NONE,
        Front = VK_CULL_MODE_FRONT_BIT,
        Back = VK_CULL_MODE_BACK_BIT,
        FrontAndBack = VK_CULL_MODE_FRONT_AND_BACK,

    };

    enum struct CompareOperation
    {
        Less = VK_COMPARE_OP_LESS,
        Equal = VK_COMPARE_OP_EQUAL,
        LessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
        Greater = VK_COMPARE_OP_GREATER,
        NotEqual = VK_COMPARE_OP_NOT_EQUAL,
        GreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
    };

    enum struct BufferUsage
    {
        TransferSrc = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        TransferDst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        UniformTexelBuffer = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        StorageTextelBuffer = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        UniformBuffer = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        StorageBuffer = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        IndexBuffer = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VertexBuffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        IndirectBuffer = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    };

    enum struct MemoryProperty
    {
        DeviceLocal = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        HostVisible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        HostCoherent = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        HostCached = VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        LazilyAllocated = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
    };

}