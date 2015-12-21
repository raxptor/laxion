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

	// Generate per
	float noise(uint32_t x, uint32_t y)
	{
		uint32_t n = x + y * 57;
		n = (n << 13) ^ n;
		return ( 1.0 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
	}

	float perlin(int octaves, float base, float exp, float s, float t)
	{
		float f = 1.0f / base;
		float out = 0;
		float a = 1.0f;
		for (int i=0;i<octaves;i++)
		{
			float ls = f * s;
			float lt = f * t;
			int is = (int)ls;
			int it = (int)lt;
			float fs = ls - is;
			float ft = lt - it;

			float s00 = noise(is, it);
			float s01 = noise(is, it + 1);
			float s10 = noise(is + 1, it);
			float s11 = noise(is + 1, it + 1);

			float u = s00 + fs * (s10-s00);
			float v = s01 + fs * (s11-s01);
			float p = u + ft * (v - u);

			out += a * p;
			a = exp * a;
			f = 2.0f * f;
		}
		return out;
	}

	float generate(float s, float t)
	{
		return 10.0f * perlin(2, 256, 0.80f, s, t);
	}

	data* create(int dimensions)
	{
		data *d = new data();

		d->heightmap_size = dimensions;
		d->height_data = new float[d->heightmap_size * d->heightmap_size];
		for (uint32_t u=0;u!=d->heightmap_size;u++)
		{
			for (uint32_t v=0;v!=d->heightmap_size;v++)
			{
				d->height_data[v * d->heightmap_size + u] = sample(d, u, v);
			}
		}

		glGenTextures(1, &d->heightmap);
		glBindTexture(GL_TEXTURE_2D, d->heightmap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, d->heightmap_size, d->heightmap_size, 0, GL_RED, GL_FLOAT, d->height_data);

		return d;
	}

	float sample(data *d, int u, int v)
	{
		return generate(u, v);
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