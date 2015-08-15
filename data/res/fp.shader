#version 150

in vec2 col;
out vec4 outColor;

void main(void)
{
	outColor = vec4(sin(0.03 * col.x)*0.5+0.5,sin(0.03 * col.y) * 0.5+0.5,1,1);
}
