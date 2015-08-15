#version 150

in vec2 vpos;
in vec2 tpos;

uniform mat4 viewproj;
uniform mat4 tileworld[128];

void main(void)
{
	int k = gl_InstanceID;
	vec4 world = tileworld[k] * vec4(vpos.x, 0.0, vpos.y, 1.0);
	world.y = sin(world.x*0.01 + world.z*0.03) * 10.0;
	gl_Position = viewproj * world;
//	gl_Position = vec4(world.x * 0.001, world.z * 0.001, 0.5, 1);
}
