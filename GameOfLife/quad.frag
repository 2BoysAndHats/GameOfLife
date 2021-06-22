#version 430
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D displayTexture;
uniform float scale;
uniform vec2 offset;

void main()
{
	FragColor = texture(displayTexture, (TexCoord / scale) + offset);
}