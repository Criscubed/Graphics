// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
uniform float u_time;

void main() {
	float s = sin(u_time);
    gl_Position = vec4(a_position.x + s, a_position.y + s, a_position.z * s, 1.0);
} 
