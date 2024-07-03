#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

// layout(set = 0, binding = 0) uniform matrix4 projection;
// layout(set = 0, binding = 1) uniform matrix4 view;
layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
    vec4 ambient_colour;
} global_ubo;

layout(push_constant) uniform push_constant {
    // only guaranteed a total of 128 bytes
    mat4 model; // 64 bytes
} u_push_constants;

layout(location = 0) out int out_mode;

// Data Transfer Object
layout(location = 1) out struct dto {
    vec4 ambient;
	vec2 tex_coord;
    vec3 normal;
} out_dto;

void main() {
	out_dto.tex_coord = in_texcoord;
    out_dto.normal = in_normal;
    out_dto.ambient = global_ubo.ambient_colour;
    // right to left
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
}