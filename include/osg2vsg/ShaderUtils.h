#pragma once

#include <osg2vsg/Export.h>

#include <vsg/all.h>

#include <osg/Array>
#include <osg/StateSet>


namespace osg2vsg
{
    enum ShaderModeMask : uint32_t
    {
        NONE = 0,
        LIGHTING = 1,
        MATERIAL = 2,
        BLEND = 4,
        BILLBOARD = 8,
        DIFFUSE_MAP = 16,
        OPACITY_MAP = 32,
        AMBIENT_MAP = 64,
        NORMAL_MAP = 128,
        SPECULAR_MAP = 256,
        SHADER_TRANSLATE = 512,
        ALL_SHADER_MODE_MASK = LIGHTING | MATERIAL | BLEND | BILLBOARD | DIFFUSE_MAP | OPACITY_MAP | AMBIENT_MAP | NORMAL_MAP | SPECULAR_MAP | SHADER_TRANSLATE
    };

    // taken from osg fbx plugin
    enum TextureUnit : uint32_t
    {
        DIFFUSE_TEXTURE_UNIT = 0,
        OPACITY_TEXTURE_UNIT,
        REFLECTION_TEXTURE_UNIT,
        EMISSIVE_TEXTURE_UNIT,
        AMBIENT_TEXTURE_UNIT,
        NORMAL_TEXTURE_UNIT,
        SPECULAR_TEXTURE_UNIT,
        SHININESS_TEXTURE_UNIT,
        MATERIAL_BINDING = 10 // same value as used in the shader
    };

    extern OSG2VSG_DECLSPEC uint32_t calculateShaderModeMask(const osg::StateSet* stateSet);

    // read a glsl file and inject defines based on shadermodemask and geometryatts
    extern OSG2VSG_DECLSPEC std::string readGLSLShader(const std::string& filename, const uint32_t& shaderModeMask, const uint32_t& geometryAttrbutes);

    // create shader source using statemask to determine type of shader to build and geometryattributes to determine attribute binding locations (specific to fbx files)
    extern OSG2VSG_DECLSPEC std::string createFbxVertexSource(const uint32_t& shaderModeMask, const uint32_t& geometryAttrbutes);
    extern OSG2VSG_DECLSPEC std::string createFbxFragmentSource(const uint32_t& shaderModeMask, const uint32_t& geometryAttrbutes);

    // create default shader source
    extern OSG2VSG_DECLSPEC std::string createDefaultVertexSource(const uint32_t& shaderModeMask, const uint32_t& geometryAttrbutes);
    extern OSG2VSG_DECLSPEC std::string createDefaultFragmentSource(const uint32_t& shaderModeMask, const uint32_t& geometryAttrbutes);


    class OSG2VSG_DECLSPEC ShaderCompiler : public vsg::Inherit<vsg::Object, ShaderCompiler>
    {
    public:
        ShaderCompiler(vsg::Allocator* allocator=nullptr);
        virtual ~ShaderCompiler();

        bool compile(vsg::ShaderStages& shaders);
    };
}
