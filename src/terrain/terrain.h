#pragma once

#include <outki/types/laxion/Terrain.h>

namespace terrain
{
	struct params
	{
		float view_mtx[16];
		outki::TerrainConfig *terrain;
		float viewpoint[3];
		float r;
	};

	struct mapping
	{
		float samples_per_meter;
	};

	struct data;

	data* create(outki::TerrainConfig *config);
	void free(data*);

	void compute_tiles(mapping *m, float *camera_pos, float range, int* x0, int* z0, int *x1, int* z1);
	void draw_terrain_tiles(data *d, mapping *m, params* p, int x0, int z0, int x1, int z1);
}
