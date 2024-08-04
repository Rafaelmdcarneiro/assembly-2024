//#g_shader_fragment_forward
#version 450

layout(location=0) out vec4 outColor;

in PerVertexData
{
    vec4 color;
} vertexData;

void main()
{
    outColor = vertexData.color;
}
