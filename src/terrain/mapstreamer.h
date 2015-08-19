#ifndef LAXION_TERRAIN_LOD_H
#define LAXION_TERRAIN_LOD_H

namespace mapstreamer
{
	struct data;

	data* create(int dimensions);
	void free(data *d);

	int tex_handle(data *d);

	// request area
	void request(int x0, int y0, int x1, int y1);
	// if streamed in
	bool query(int x0, int y0, int x1, int y1);
}

#endif
