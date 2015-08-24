#pragma once

namespace mapstreamer
{
	struct data;

	data* create(int dimensions);
	void free(data *d);

	int tex_handle(data *d);

	float sample(data *d, int u, int v);

	// request area
	void request(data *d, int x0, int y0, int x1, int y1);
	// if streamed in
	bool query(data *d, int x0, int y0, int x1, int y1);
}

