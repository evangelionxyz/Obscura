#version 450

layout(push_constant) uniform SpritePushConstants
{
    mat4 model;
    vec4 color;
    vec2 uvOffset;
    vec2 uvScale;
    uint textureSlot;
    uint useTexture;
    float padding[6];
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out flat uint fragTextureSlot;
layout(location = 3) out flat uint fragUseTexture;

void main()
{
    gl_Position     = pc.model * vec4(inPosition, 1.0);
    fragColor       = inColor * pc.color;
    fragTexCoord    = inTexCoord * pc.uvScale + pc.uvOffset;
    fragTextureSlot = pc.textureSlot;
    fragUseTexture  = pc.useTexture;
}
