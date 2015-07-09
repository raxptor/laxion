#pragma once

namespace terrain
{
	struct params
	{
		float viewpoint[3];
		float r;
	};

	void draw_terrain(float x0, float z0, float x1, float z1, params* p);
}