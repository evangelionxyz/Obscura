#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform sampler2D u_Textures[];

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint fragTextureSlot;
layout(location = 3) in flat uint fragUseTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 texColor = vec4(1.0);
    if (fragUseTexture != 0u)
    {
        texColor = texture(u_Textures[fragTextureSlot], fragTexCoord);
    }
    outColor = fragColor * texColor;
}
