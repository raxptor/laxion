#include "mapstreamer.h"

#include <kosmos/glwrap/gl.h>
#include <kosmos/core/mat4.h>
#include <kosmos/core/line.h>
#include <kosmos/render/shader.h>
#include <kosmos/log/log.h>

namespace mapstreamer
{
	struct data
	{
		float *height_data;
		uint32_t heightmap_size;
		GLuint heightmap;
	};

	data* create(int dimensions)
	{
		data *d = new data();

		d->heightmap_size = dimensions;
		d->height_data = new float[d->heightmap_size * d->heightmap_size];
		for (uint32_t u=0;u!=d->heightmap_size;u++)
		{
			for (uint32_t v=0;v!=d->heightmap_size;v++)
			{
				d->height_data[v * d->heightmap_size + u] = (rand() % 100) * 0.01f; //sample(d, u, v);
			}
		}

		glGenTextures(1, &d->heightmap);
		glBindTexture(GL_TEXTURE_2D, d->heightmap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, d->heightmap_size, d->heightmap_size, 0, GL_RED, GL_FLOAT, d->height_data);

		return d;
	}

	float sample(data *d, int u, int v)
	{
		return 10.0f * sin(u * 0.03f) + 10.0f * cos(v * 0.05);
	}

	void free(data *d)
	{
		delete d;
	}

	int tex_handle(data *d)
	{
		return d->heightmap;
	}
}