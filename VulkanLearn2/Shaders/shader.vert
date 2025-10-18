#version 450

// Tọa độ 3 đỉnh của tam giác
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

// Màu sắc 3 đỉnh
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0), // Đỏ
    vec3(0.0, 1.0, 0.0), // Xanh lá
    vec3(0.0, 0.0, 1.0)  // Xanh dương
);

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}