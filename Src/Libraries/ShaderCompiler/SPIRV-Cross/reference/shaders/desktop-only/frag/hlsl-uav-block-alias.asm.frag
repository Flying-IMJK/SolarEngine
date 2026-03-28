#version 450

layout(binding = 0, std430) buffer Foobar
{
    vec4 m_Data[];
} Foobar_1;

layout(binding = 1, std430) buffer Foobaz
{
    vec4 m_Data[];
} Foobaz_1;

layout(location = 0) out vec4 _entryPointOutput;

vec4 _main()
{
    return Foobar_1.m_Data[0] + Foobaz_1.m_Data[0];
}

void main()
{
    _entryPointOutput = _main();
}

