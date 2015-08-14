
attribute vec2 vpos;
attribute vec2 tpos;

uniform mat4 viewproj;
uniform mat4 tileworld[16];

void main(void)
{
	int k = gl_InstanceIDARB;
	vec4 world = tileworld[k] * vec4(vpos.x, 0.0, vpos.y, 1.0);
	world.y = sin(world.x*0.01 + world.z*0.03) * 10.0;
	gl_Position = viewproj * world;
}
