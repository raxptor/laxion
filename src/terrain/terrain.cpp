#include "terrain.h"

#include <kosmos/glwrap/gl.h>
#include <kosmos/core/mat4.h>
#include <kosmos/render/shader.h>
#include <kosmos/log/log.h>

#include <math.h>
#include <stdio.h>

namespace terrain
{
	// Tiles are just convenient places to start subdividing.
	//
	enum
	{
		TILE_SAMPLES    = 1024,
	};
	
	struct terrain_vtx_t
	{
		float x, y;
		float u, v;
	};
	
	int in_layer(int layer)
	{
		return 1 + 1 * layer;
	}
	
	void make_patch_vertices(terrain_vtx_t *out, int size)
	{
		//    0
		//   1 2
		//  3 4 5
		// 6 7 8 9
		float height = 1.0f / float(size-1);
		for (int i=0;i!=size;i++)
		{
			const float y = height * float(i);
			const float x0 = - height * float(i);
			int num = 1 + i;
			for (int j=0;j<num;j++)
			{
				out->x = x0 + float(2.0f * j)*height;
				out->y = y;
				out->u = 0;
				out->v = 0;
				out++;
			}
		}
	}
	
	// returns count of indices
	int make_indices(unsigned short *out, int size)
	{
		// 0
		// 1 2
		// 3 4 5
		// 6 7 8 9
	
		// k . . .
		// l . . . .
		// m . . . . .
		int k = 0;
		int l = 1;
		int m = 3;
		
		unsigned short *start = out;
		for (int i=0;i<(size-1);i++)
		{
			int K = k;
			int L = l;
			while (true)
			{
				if ((L + 1) < m)
				{
					/*
					*out++ = L;
					*out++ = K;
					*out++ = L+1;
					*/
					L++;
				}
				if ((K + 1) < l)
				{
					*out++ = L;
					*out++ = K;
					*out++ = K + 1;
					K++;
				}
				if (K == (l-1) && L == (m-1))
					break;
			}
			k = l;
			l = m;
			m = m + (i + 3);
		}
		return out - start;
	}

	float get_height(float x, float z)
	{
		return sinf(x*0.03f + z*0.03f) * 10.0;
	}
	
	terrain_vtx_t* make_patch(int size, terrain_vtx_t *out_array)
	{
		static terrain_vtx_t vtx[1024*1024];
		unsigned short idx[1024*1024];
		make_patch_vertices(vtx, size);
		int count = make_indices(idx, size);
		for (int i=0;i<count;i++)
		{
			int v = idx[i];
			out_array->x = vtx[v].x;
			out_array->y = vtx[v].y;
			out_array->u = vtx[v].x;
			out_array->v = vtx[v].y;
			out_array++;
		}
		return out_array;
	}

	enum {
		MAX_INSTANCES = 128
	};

	struct data
	{
		outki::TerrainConfig *config;
		kosmos::shader::program *sprog;
		GLuint vbo;
		int level0_begin, level0_end;
		int level1_begin, level1_end;

		float lvl0_mtx[MAX_INSTANCES*16];
		float lvl1_mtx[MAX_INSTANCES*16];
		int lvl0_count, lvl1_count;
	};

	static long long s_polys = 0, s_drawcalls = 0;

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
	
	int level(float s)
	{
		if (s < 0.5f)
			return 0;
		else
			return 1;
	}

	void flush(data* d, bool force)
	{
		int lim = force ? 0 : (MAX_INSTANCES - 1);
		if (d->lvl0_count > lim)
		{
			glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "tileworld"), d->lvl0_count, GL_FALSE, d->lvl0_mtx);
			//glDrawArraysInstanced(GL_TRIANGLES, d->level0_begin, d->level0_end - d->level0_begin, d->lvl0_count);
			d->lvl0_count = 0;

			s_polys += (d->level0_end-d->level0_begin) / 3;
			s_drawcalls++;
		}
		if (d->lvl1_count > lim)
		{
			glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "tileworld"), d->lvl0_count, GL_FALSE, d->lvl1_mtx);
			//glDrawArraysInstanced(GL_TRIANGLES, d->level0_begin, d->level1_end - d->level1_begin, d->lvl1_count);
			d->lvl1_count = 0;
			s_polys += (d->level1_end-d->level1_begin) / 3;
			s_drawcalls++;
		}
	}

	void draw_patch(data *d, int index, float *mtx)
	{
		flush(d, false);

		if (index == 0)
		{
			memcpy(&d->lvl0_mtx[d->lvl0_count * 16], mtx, 16*sizeof(float));
			d->lvl0_count++;
		}
		else
		{
			memcpy(&d->lvl1_mtx[d->lvl1_count * 16], mtx, 16*sizeof(float));
			d->lvl1_count++;
		}
	}

	void do_tile(data *d, mapping *m, params *p, float x0, float z0, float x1, float z1)
	{
		const float s0 = p->r * edge_scaling(p, x0, z0, x1, z0);
		const float s1 = p->r * edge_scaling(p, x0, z0, x0, z1);
		const float s2 = p->r * edge_scaling(p, x1, z1, x1, z0);
		const float s3 = p->r * edge_scaling(p, x1, z1, x0, z1);

		if (s0 > 1 || s1 > 1 || s2 > 1 || s3 > 1)
		{
			float cx = .5f * (x0 + x1);
			float cz = .5f * (z0 + z1);
			do_tile(d, m, p, x0, z0, cx, cz);
			do_tile(d, m, p, cx, z0, x1, cz);
			do_tile(d, m, p, x0, cz, cx, z1);
			do_tile(d, m, p, cx, cz, x1, z1);
			return;
		}

		const float cx = (x0+x1) / 2.0f;
		const float cz = (z0+z1) / 2.0f;

		const float sx = 0.5f * (x1 - x0);
		const float sz = 0.5f * (z1 - z0);
		float mtx[16];
		kosmos::mat4_zero(mtx);
		// down
		mtx[0] = sx;
		mtx[5] = 1.0f;
		mtx[10] = sz;
		mtx[12] = cx;
		mtx[14] = cz;
		mtx[15] = 1;
		draw_patch(d, level(s3), mtx);
		// up
		mtx[0] = -sx;
		mtx[10] = -sz;
		mtx[12] = cx;
		mtx[14] = cz;
		draw_patch(d, level(s0), mtx);
		// left
		mtx[0] = 0;
		mtx[10] = 0;
		mtx[2] = -sx;
		mtx[8] = -sz;
		draw_patch(d, level(s1), mtx);
		// right
		mtx[0] = 0;
		mtx[10] = 0;
		mtx[2] = sx;
		mtx[8] = sz;
		draw_patch(d, level(s2), mtx);
	}

	data* create(outki::TerrainConfig *config)
	{
		data *d = new data();
		glGenBuffers(1, &d->vbo);

		d->sprog = kosmos::shader::program_get(config->Shader);
		if (!d->sprog)
		{
			KOSMOS_ERROR("No terrain program")
			return 0;
		}

		static terrain_vtx_t buf[1024*1024];
		terrain_vtx_t *end0 = make_patch(33, &buf[0]);
		terrain_vtx_t *end1 = make_patch(65, end0);

		d->level0_begin = 0;
		d->level0_end = end0 - buf;
		d->level1_begin = d->level0_end;
		d->level1_end = end1 - buf;

		glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(terrain_vtx_t) * (end1 - buf), buf, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return d;
	}

	void free(data *d)
	{
		kosmos::shader::program_free(d->sprog);
		glDeleteBuffers(1, &d->vbo);
		delete d;
	}


	void draw_terrain_tiles(data *d, mapping *m, params* p, int x0, int z0, int x1, int z1)
	{
		const float tile_size = (float)TILE_SAMPLES / m->samples_per_meter;

		kosmos::shader::program_use(d->sprog);

		// binds to active buffer
		glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
		glVertexAttribPointer(kosmos::shader::find_attribute(d->sprog, "vpos"), 2, GL_FLOAT, GL_FALSE, sizeof(terrain_vtx_t), 0);
		glVertexAttribPointer(kosmos::shader::find_attribute(d->sprog, "tpos"), 2, GL_FLOAT, GL_FALSE, sizeof(terrain_vtx_t), (void*)8);
		glEnableVertexAttribArray(kosmos::shader::find_attribute(d->sprog, "vpos"));
		glEnableVertexAttribArray(kosmos::shader::find_attribute(d->sprog, "tpos"));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "viewproj"), 1, GL_FALSE, p->view_mtx);

		s_polys = 0;
		s_drawcalls = 0;

		d->lvl0_count = 0;
		d->lvl1_count = 0;

		for (int z=z1-1;z>=z0;z--)
		{
			for (int x=x0;x<x1;x++)
			{
				do_tile(d, m, p, tile_size * x, tile_size * z, tile_size * (x + 1),  tile_size * (z + 1));
			}
		}

		flush(d, true);

		glUseProgram(0);
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

		printf("%d %d %d %d\n", *x0, *z0, *x1, *z1);
	}
}
