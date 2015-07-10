#pragma once

namespace terrain
{
	struct params
	{
		float viewpoint[3];
		float r;
	};

	struct mapping
	{
		float samples_per_meter;
	};

	void compute_tiles(mapping *m, float *camera_pos, float range, int* x0, int* z0, int *x1, int* z1);
	void draw_terrain_tiles(mapping *m, params* p, int x0, int z0, int x1, int z1);
}