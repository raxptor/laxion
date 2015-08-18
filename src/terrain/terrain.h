#pragma once

#include <outki/types/laxion/Terrain.h>

namespace terrain
{
	struct view
	{
		float viewpoint[3];
		float view_mtx[16];
		float max_range_sqr;
		float r;
	};

	struct data;
    
	data* create(outki::TerrainConfig *config);
	void free(data*);
	float get_terrain_height(data *d, float x, float z);
	void draw_terrain(data *d, view* p);
}
