#version 430

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(push_constant) uniform PC {
    uint64_t bufferAddress;
} pc;

struct Vertex {
    vec2 position;
    vec4 color;
};

layout(buffer_reference, std430, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(location = 0) out vec4 vertexColor;

void main() {
    VertexBuffer buf = VertexBuffer(pc.bufferAddress);
    Vertex v = buf.vertices[gl_VertexIndex];
    gl_Position = vec4(v.position, 0.0, 1.0);
    vertexColor = v.color;
}
