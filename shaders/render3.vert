#version 330

// Attribute変数
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

// Varying変数
out vec3 f_fragColor;

// Uniform変数
uniform mat4 u_mvpMat;

void main() {

    gl_Position = u_mvpMat * vec4(in_position, 1.0);

	  f_fragColor = in_color;
}
