#include <vsg/vk/BufferData.h>
#include <vsg/vk/Descriptor.h>
#include <vsg/vk/DescriptorSet.h>
#include <vsg/vk/DescriptorPool.h>
#include <vsg/vk/BindVertexBuffers.h>
#include <vsg/vk/BindIndexBuffer.h>
#include <vsg/vk/Draw.h>

#include <vsg/io/ReaderWriter.h>

#include <vsg/traversals/DispatchTraversal.h>

#include <iostream>

#include <osg2vsg/StateAttributes.h>

using namespace vsg;

#if 0
#define DEBUG_OUTPUT std::cout
#else
#define DEBUG_OUTPUT if (false) std::cout
#endif


/////////////////////////////////////////////////////////////////////////////////////////
//
// GraphicsPipelineAttribute
//
GraphicsPipelineAttribute::GraphicsPipelineAttribute(Allocator* allocator) :
    Inherit(allocator)
{
}

void GraphicsPipelineAttribute::pushTo(State& state) const
{
    if (_bindPipeline) _bindPipeline->pushTo(state);
    if (_projPushConstant) _projPushConstant->pushTo(state);
    if (_viewPushConstant) _viewPushConstant->pushTo(state);
}

void GraphicsPipelineAttribute::popFrom(State& state) const
{
    if (_bindPipeline) _bindPipeline->popFrom(state);
    if (_projPushConstant) _projPushConstant->popFrom(state);
    if (_viewPushConstant) _viewPushConstant->popFrom(state);
}

void GraphicsPipelineAttribute::dispatch(CommandBuffer& commandBuffer) const
{
    if (_bindPipeline) _bindPipeline->dispatch(commandBuffer);
    if (_projPushConstant) _projPushConstant->dispatch(commandBuffer);
    if (_viewPushConstant) _viewPushConstant->dispatch(commandBuffer);
}

void GraphicsPipelineAttribute::read(Input& input)
{
    StateCommand::read(input);

    shaders.resize(input.readValue<uint32_t>("NumShader"));
    for (auto& shader : shaders)
    {
        shader = input.readObject<Shader>("Shader");
    }
}

void GraphicsPipelineAttribute::write(Output& output) const
{
    StateCommand::write(output);

    output.writeValue<uint32_t>("NumShader", shaders.size());
    for (auto& shader : shaders)
    {
        output.writeObject("Shader", shader.get());
    }
}

void GraphicsPipelineAttribute::compile(Context& context)
{
    DEBUG_OUTPUT<<"\nGraphicsPipelineAttribute::compile(Context& context) "<<this<<std::endl;

    //
    // set up descriptor layout and descriptor set and pipeline layout for uniforms
    //
    if (!descriptorPoolSizes.empty()) context.descriptorPool = DescriptorPool::create(context.device, maxSets, descriptorPoolSizes);
    DEBUG_OUTPUT<<"  context.descriptorPool = "<<context.descriptorPool.get()<<std::endl;

    // prevent previous GraphicsPipelineAttribute::compile(Context& context) calls during the current compile traversal accumulating into this GraphicsPipeline setup.
    context.descriptorSetLayouts.clear();

    for (unsigned int i = 0; i < descriptorSetLayoutBindings.size(); i++)
    {
#if 1
        auto descriptorSetLayout = DescriptorSetLayout::create(descriptorSetLayoutBindings[i]);
        descriptorSetLayout->compile(context);
        context.descriptorSetLayouts.emplace_back(descriptorSetLayout->implementation());
#else
        context.descriptorSetLayouts.push_back(DescriptorSetLayout::Implementation::create(context.device, descriptorSetLayoutBindings[i]));
#endif
        DEBUG_OUTPUT << "  context.descriptorSetLayout = " << context.descriptorSetLayouts[i].get() << std::endl;
    }

#if 1
    auto pipelineLayout = vsg::PipelineLayout::create(context.descriptorSetLayouts, pushConstantRanges);
    pipelineLayout->compile(context);
    context.pipelineLayout = pipelineLayout->implementation();
#else
    context.pipelineLayout = PipelineLayout::Implementation::create(context.device, context.descriptorSetLayouts, pushConstantRanges);
#endif
    DEBUG_OUTPUT<<"  context.pipelineLayout = "<<context.pipelineLayout.get()<<std::endl;


    ShaderModules shaderModules;
    shaderModules.reserve(shaders.size());
    for(auto& shader : shaders)
    {
        shaderModules.emplace_back(ShaderModule::create(context.device, shader));
    }

    auto shaderStages = ShaderStages::create(shaderModules);

    DEBUG_OUTPUT<<"  shaderStages = "<<shaderStages.get()<<std::endl;

    GraphicsPipelineStates full_pipelineStates = pipelineStates;
    full_pipelineStates.emplace_back(context.viewport);
    full_pipelineStates.emplace_back(shaderStages);
    full_pipelineStates.emplace_back(VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions));


    ref_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::create(context.device, context.renderPass, context.pipelineLayout, full_pipelineStates);

    DEBUG_OUTPUT<<"  pipeline = "<<pipeline.get()<<std::endl;

    _bindPipeline = BindPipeline::create(pipeline);

    DEBUG_OUTPUT<<"  _bindPipeline = "<<_bindPipeline.get()<<std::endl;

    if (context.projMatrix) _projPushConstant = PushConstants::create(VK_SHADER_STAGE_VERTEX_BIT, 0, context.projMatrix);
    if (context.viewMatrix) _viewPushConstant = PushConstants::create(VK_SHADER_STAGE_VERTEX_BIT, 64, context.viewMatrix);
    DEBUG_OUTPUT<<"GraphicsPipelineAttribute::compile(Context& context) finished "<<this<<"\n"<<std::endl;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Texture
//
Texture::Texture(Allocator* allocator) :
    Inherit(allocator)
{
    // set default sampler info
    _samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    _samplerInfo.minFilter = VK_FILTER_LINEAR;
    _samplerInfo.magFilter = VK_FILTER_LINEAR;
    _samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    _samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    _samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
#if 1
    // requres Logical device to have deviceFeatures.samplerAnisotropy = VK_TRUE; set when creating the vsg::Device
    _samplerInfo.anisotropyEnable = VK_TRUE;
    _samplerInfo.maxAnisotropy = 16;
#else
    _samplerInfo.anisotropyEnable = VK_FALSE;
    _samplerInfo.maxAnisotropy = 1;
#endif
    _samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    _samplerInfo.unnormalizedCoordinates = VK_FALSE;
    _samplerInfo.compareEnable = VK_FALSE;
    _samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

ref_ptr<vsg::Descriptor> Texture::compile(Context& context)
{
    ref_ptr<Sampler> sampler = Sampler::create(context.device, _samplerInfo, nullptr);
    vsg::ImageData imageData = vsg::transferImageData(context.device, context.commandPool, context.graphicsQueue, _textureData, sampler);
    if (!imageData.valid())
    {
        DEBUG_OUTPUT<<"Texture not created"<<std::endl;
        return ref_ptr<vsg::Descriptor>();
    }

    return vsg::DescriptorImage::create(_bindingIndex, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vsg::ImageDataList{imageData});
}
