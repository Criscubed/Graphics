// Fragment shader
#version 150

out vec4 frag_color;

uniform float u_time;

void main() {
	float s = abs(sin(u_time));
    frag_color = vec4(s, s, s, 1.0);
}
