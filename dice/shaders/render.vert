#version 330

// Attribute変数
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in float in_faceID;


// Varying変数
out vec2 texcoord;
out float f_faceID;

// Uniform変数
uniform mat4 u_mvpMat;

void main() {
    
    gl_Position = u_mvpMat * vec4(in_position, 1.0);

	texcoord = in_texcoord;
	f_faceID= in_faceID;
}
