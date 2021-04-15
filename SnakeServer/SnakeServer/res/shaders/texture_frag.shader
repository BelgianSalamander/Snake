#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color;

void main() {
	float intensity = texture(u_Texture, v_TexCoord).x;
	vec4 texColor = vec4(intensity , intensity, intensity, intensity);
	color = texColor * u_Color;
}