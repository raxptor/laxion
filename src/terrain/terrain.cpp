#include "terrain.h"

#include <kosmos/glwrap/gl.h>

#include <math.h>
#include <stdio.h>

namespace terrain
{
	enum
	{
		TILE_SAMPLES    = 4096
	};

	float get_height(float x, float z)
	{
		return -3.f * sinf(x*0.02f) + 4.0f * cosf(z*0.09f);// + 5.0f * sinf(-x*0.3f + z*0.5f);
	}

	float edge_scaling(params* p, float x0, float z0, float x1, float z1)
	{
		const float y0 = get_height(x0, z0);
		const float y1 = get_height(x1, z1);
		float dx0 = x0 - p->viewpoint[0];
		float dy0 = y0 - p->viewpoint[1];
		float dz0 = z0 - p->viewpoint[2];
		float dx1 = x1 - p->viewpoint[0];
		float dy1 = y1 - p->viewpoint[1];
		float dz1 = z1 - p->viewpoint[2];
		float d0 = dx0*dx0 + dy0*dy0 + dz0*dz0;
		float d1 = dx1*dx1 + dy1*dy1 + dz1*dz1;
		float d = d0 < d1 ? d0 : d1;
		return sqrtf((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0)+(z1-z0)*(z1-z0)) / sqrtf(d);
	}

	inline void vtx(float x, float z)
	{
		glVertex3f(x, get_height(x, z), z);
	}

	void color(float s)
	{
		int level = 0;

		if (s < 0.5f)
			level = 0;
		else
			level = 1;

		switch (level)
		{
			case 0:
				glColor3f(0.50f, 0.00f, 0.00f);
				break;
			case 1:
				glColor3f(0.00f, 0.60f, 0.00f);
				break;
			case 2:
				glColor3f(0.00f, 0.00f, 0.60f);
				break;
			default:
				glColor3f(0.60f, 0.60f, 0.60f);
				break;
		}
	}

	void do_tile(mapping *m, params *p, float x0, float z0, float x1, float z1, bool line)
	{
		const float s0 = p->r * edge_scaling(p, x0, z0, x1, z0);
		const float s1 = p->r * edge_scaling(p, x0, z0, x0, z1);
		const float s2 = p->r * edge_scaling(p, x1, z1, x1, z0);
		const float s3 = p->r * edge_scaling(p, x1, z1, x0, z1);

		if (s0 > 1 || s1 > 1 || s2 > 1 || s3 > 1)
		{
			float cx = .5f * (x0 + x1);
			float cz = .5f * (z0 + z1);
			do_tile(m, p, x0, z0, cx, cz, line);
			do_tile(m, p, cx, z0, x1, cz, line);
			do_tile(m, p, x0, cz, cx, z1, line);
			do_tile(m, p, cx, cz, x1, z1, line);
			return;
		}

		const float cx = (x0+x1) / 2.0f;
		const float cz = (z0+z1) / 2.0f;

		if (!line)
		{
		//color(s0);
			glColor3f(0.5*sin(x0*0.2)+0.5f,0.5f+cos(z0*0.1),0.5);
		vtx(cx, cz);
		vtx(x0, z0);
		vtx(x1, z0);

		//color(s3);
		vtx(cx, cz);
		vtx(x0, z1);
		vtx(x1, z1);

//		color(s2);
		vtx(cx, cz);
		vtx(x1, z0);
		vtx(x1, z1);

//		color(s1);
		vtx(cx, cz);
		vtx(x0, z0);
		vtx(x0, z1);
		}
		else
		{
		glColor3f(1, 1, 1);
		glVertex3f(x0, get_height(x0, z0), z0);
		glVertex3f(x1, get_height(x1, z0), z0);
		glVertex3f(x0, get_height(x0, z0), z0);
		glVertex3f(x0, get_height(x0, z1), z1);

		glVertex3f(x1, get_height(x1, z0), z0);
		glVertex3f(x1, get_height(x1, z1), z1);
		glVertex3f(x0, get_height(x0, z1), z1);
		glVertex3f(x1, get_height(x1, z1), z1);
		}
	}

	void draw_terrain_tiles(mapping *m, params* p, int x0, int z0, int x1, int z1)
	{

		const float tile_size = (float)TILE_SAMPLES / m->samples_per_meter;

		glLineWidth(1.0f);
		glBegin(GL_TRIANGLES);
		for (int z=z1-1;z>=z0;z--)
		{
			for (int x=x0;x<x1;x++)
			{
				do_tile(m, p, tile_size * x, tile_size * z, tile_size * (x + 1),  tile_size * (z + 1), false);
			}
		}

		glEnd();

		glLineWidth(1.0f);
		glBegin(GL_LINES);
		for (int z=z0;z<z1;z++)
		{
			for (int x=x0;x<x1;x++)
			{
				//do_tile(m, p, tile_size * x, tile_size * z, tile_size * (x + 1),  tile_size * (z + 1), true);
			}
		}

		glEnd();
	}

	void compute_tiles(mapping *m, float *camera_pos, float range, int* x0, int* z0, int *x1, int* z1)
	{
		const float tile_size = (float)TILE_SAMPLES / m->samples_per_meter;
		int tx = (int)(camera_pos[0] / tile_size);
		int tz = (int)(camera_pos[2] / tile_size);

		int range_tiles = range / tile_size;

		// compute the location of the centre tile

		*x0 = tx - range_tiles;
		*z0 = tz - range_tiles;
		*x1 = tx + range_tiles;
		*z1 = tz + range_tiles;
	}
}
