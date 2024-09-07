#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 vert_color;

layout(push_constant) uniform Constants {
	mat4 transform;
} constants;

void main() {
	gl_Position = constants.transform * vec4(position, 1.0);
	vert_color = color;
}

