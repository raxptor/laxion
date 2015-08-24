#version 150

in vec2 vpos;
in vec2 tpos;

uniform mat4 viewproj;
uniform mat4 tileworld[128];
uniform sampler2D heightmap;

out vec2 col;

void main(void)
{
	int k = gl_InstanceID;
	vec4 world = tileworld[2*k] * vec4(vpos.x, 0.0, vpos.y, 1.0);
	vec2 texel = (tileworld[2*k+1] * vec4(vpos.x, vpos.y, 0.0, 1.0)).xy;

	ivec2 uv = ivec2(texel.x, texel.y);
	uv.x = uv.x & 4095;
	uv.y = uv.y & 4095;
	world.y = texelFetch(heightmap, uv, 0).x;
	col = vec2(texel.x, texel.y);
	gl_Position = viewproj * world;
}
