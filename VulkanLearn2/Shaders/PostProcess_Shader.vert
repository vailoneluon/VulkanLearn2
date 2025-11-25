#version 450

layout(location = 0) out vec2 outUV;

const vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0), // 0. Tam giác 1
    vec2( 1.0, -1.0), // 1.
    vec2(-1.0,  1.0), // 2.
    vec2(-1.0,  1.0), // 3. Tam giác 2
    vec2( 1.0, -1.0), // 4.
    vec2( 1.0,  1.0)  // 5.
);

const vec2 uvs[6] = vec2[](
    vec2(0.0, 0.0), // 0.
    vec2(1.0, 0.0), // 1.
    vec2(0.0, 1.0), // 2.
    vec2(0.0, 1.0), // 3.
    vec2(1.0, 0.0), // 4.
    vec2(1.0, 1.0)  // 5.
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outUV = uvs[gl_VertexIndex];
}