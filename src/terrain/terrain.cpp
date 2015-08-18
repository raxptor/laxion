#include "terrain.h"

#include <kosmos/glwrap/gl.h>
#include <kosmos/core/mat4.h>
#include <kosmos/core/line.h>
#include <kosmos/render/shader.h>
#include <kosmos/log/log.h>

#include <math.h>
#include <stdio.h>

#include <cassert>

namespace terrain
{
	enum
	{
		LEVEL_0_SAMPLES = 16,
		LEVEL_1_SAMPLES = LEVEL_0_SAMPLES * 2
	};

	struct terrain_vtx_t
	{
		float x, y;
		float u, v;
	};

	struct patch
	{
		int u0, v0, u1, v1;
	};

	struct corner
	{
		int u, v;
	};

	struct draw_buffer
	{
		enum {
			LEVELS = 2,
			MAX_PATCHES = 1024,
			MAX_TILES = 4 * MAX_PATCHES,
		};

		int patch_count;
		patch patches[MAX_PATCHES];
		corner corners[4*MAX_PATCHES];

		kosmos::mat4f tile2world[LEVELS][MAX_TILES];
		kosmos::mat4f tile2heightmap[LEVELS][MAX_TILES];
		int tile_count[LEVELS];
	};

	struct data
	{
		outki::TerrainConfig *config;
		kosmos::shader::program *sprog;

		GLuint vbo, vao;
		int verts_begin[2];
		int verts_count[2];

		float *height_data;
		uint32_t heightmap_size;
		GLuint heightmap;

		draw_buffer buffer;

	};

	struct draw_info
	{
		data *d;
		view v;
		float mps; // meters per sample
		float comp_r;
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
					*out++ = K;
					*out++ = L;
					*out++ = L+1;
					L++;

				}
				if ((K + 1) < l)
				{
					*out++ = L;
					*out++ = K + 1;
					*out++ = K;
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
		return sinf(x*0.1f + z*0.24f + sin(x*0.4)*3.0f) * (0.30f + 100.0f * sinf(x*0.0001f + z*0.00015));
	}

	float get_terrain_height(data *d, float x, float z)
	{
		return get_height(x, z);
	}

	terrain_vtx_t* make_patch(int size, terrain_vtx_t *out_array)
	{
		static terrain_vtx_t vtx[1024*1024];
		static unsigned short idx[1024*1024];
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

	int level(float s)
	{
		if (s < 0.5f)
			return 0;
		else
			return 1;
	}

	void draw_buffer_clear(draw_buffer *buf)
	{
		buf->patch_count = 0;
		buf->tile_count[0] = 0;
		buf->tile_count[1] = 0;
	}

	void add_patch(data *d, int index, kosmos::mat4f mtx, float *mtx_uv)
	{
		if (index == 2)
			return;

		draw_buffer* db = &d->buffer;
		assert(index < draw_buffer::LEVELS);

		int count = db->tile_count[index];
		if (count == draw_buffer::MAX_TILES)
		{
			KOSMOS_WARNING("Reached max number of tiles allowed, throwing away");
			return;
		}

		memcpy(db->tile2world[index][count], mtx, sizeof(kosmos::mat4f));
		memcpy(db->tile2heightmap[index][count], mtx_uv, sizeof(kosmos::mat4f));
		db->tile_count[index]++;
	}

	bool project(float *out, float *mtx, float *in)
	{
		kosmos::mul_mat4_vec4(out, mtx, in);

		if (out[3] < 0.0001f)
		{
			out[0] = 0;
			out[1] = 0;
			out[2] = 0;
			out[3] = 0;
			return false;
		}

		float k = 1.0f / out[3];
		out[0] = k * out[0];
		out[1] = k * out[1];
		out[2] = k * out[2];
		out[3] = 1.0f;
		return true;
	}


	float cp_distance_axis(float p, float x0, float x1)
	{
		if (p >= x0 && p < x1)
			return 0;
		else if (p < x0)
			return x0 - p;
		else
			return p - x1;
	}

	float cp_distance(float x, float y, float x0, float y0, float x1, float y1)
	{
		return cp_distance_axis(x, x0, x1) + cp_distance_axis(y, y0, y1);
	}

	float edge_scaling(draw_info *di, int u0, int v0, int u1, int v1)
	{
		float x0 = di->mps * u0;
		float x1 = di->mps * u1;
		float z0 = di->mps * v0;
		float z1 = di->mps * v1;

		float d = kosmos::line_point_distance(di->v.viewpoint[0], di->v.viewpoint[2], x0, z0, x1, z1);
//		float d = cp_distance(di->v.viewpoint[0], di->v.viewpoint[2], x0, z0, x1, z1);

//		float dx = (x0+x1)/2.0f - di->v.viewpoint[0];
//		float dz = (z0+z1)/2.0f - di->v.viewpoint[2];
//		d = sqrtf(dx*dx + dz*dz);

		if (d < 0)
		{
			return 0;
		}
		else
		{
			float k = sqrtf((x1-x0)*(x1-x0) + (z1-z0)*(z1-z0));
			return k / d;
		}
	}

	void do_tile(draw_info *di, int u0, int v0, int u1, int v1);

	void do_tile_subdivide(draw_info *di, int u0, int v0, int u1, int v1)
	{
		int cu = (u0 + u1) / 2;
		int cv = (v0 + v1) / 2;
		do_tile(di, u0, v0, cu, cv); // tl
		do_tile(di, cu, v0, u1, cv); // tr
		do_tile(di, u0, cv, cu, v1); // bl
		do_tile(di, cu, cv, u1, v1); // br
	}

	inline bool allow_subdivide(draw_info *di, int u0, int v0, int u1, int v1)
	{
		assert(u1 > u0);
		return (u1 - u0) > LEVEL_1_SAMPLES;
	}

	inline bool within_range(draw_info *di, int u, int v)
	{
		const float dx = (di->mps * u - di->v.viewpoint[0]);
		const float dy = (di->mps * v - di->v.viewpoint[2]);
		return dx*dx + dy*dy < di->v.max_range_sqr;
	}

	static int c_do_tiles = 0;

	void do_tile(draw_info *di, int u0, int v0, int u1, int v1)
	{
		assert(u1 > u0);
		assert(v1 > v0);
		assert((u1-u0) >= LEVEL_0_SAMPLES);
		assert((v1-v0) >= LEVEL_0_SAMPLES);
		c_do_tiles++;

		const bool can_subdivide = allow_subdivide(di, u0, v0, u1, v1);

		const float s0 = di->comp_r * edge_scaling(di, u0, v0, u1, v0); // top
		const float s1 = di->comp_r * edge_scaling(di, u0, v0, u0, v1); // left
		const float s2 = di->comp_r * edge_scaling(di, u1, v0, u1, v1); // right
		const float s3 = di->comp_r * edge_scaling(di, u0, v1, u1, v1); // bottom

		if (!s0 && !s1 && !s2 && !s3)
			return;

		if (can_subdivide && (s0 > 1 || s1 > 1 || s2 > 1 || s3 > 1))
		{
			do_tile_subdivide(di, u0, v0, u1, v1);
			return;
		}

		if (can_subdivide)
		{
			assert(s0 < 1);
			assert(s1 < 1);
			assert(s2 < 1);
			assert(s3 < 1);
		}

		draw_buffer *db = &di->d->buffer;
		if (db->patch_count == draw_buffer::MAX_PATCHES)
		{
			KOSMOS_WARNING("Too many patches");
			return;
		}

		patch *p = &db->patches[db->patch_count];
		p->u0 = u0;
		p->v0 = v0;
		p->u1 = u1;
		p->v1 = v1;

		corner *c = &db->corners[4 * db->patch_count];
		c[0].u = u0;
		c[0].v = v0;

		c[1].u = u1;
		c[1].v = v0;

		c[2].u = u0;
		c[2].v = v1;

		c[3].u = u1;
		c[3].v = v1;

		db->patch_count++;
	}

	bool is_split(draw_buffer *buf, int u0, int v0, int u1, int v1)
	{
		int mu = (u0+u1) / 2;
		int mv = (v0+v1) / 2;
		for (int i=0;i<4*buf->patch_count;i++)
		{
			if (buf->corners[i].u == mu && buf->corners[i].v == mv)
				return true;
		}
		return false;
	}

	int tile_level(draw_buffer *buf, int u0, int v0, int u1, int v1)
	{
		bool k = is_split(buf, u0, v0, u1, v1);
		if (k)
		{
			if (is_split(buf, u0, v0, (u0+u1)/2, (v0+v1)/2) || is_split(buf, u1, v1, (u0+u1)/2, (v0+v1)/2))
				return 2;

			return 1;
		}
		else
		{
			return 0;
		}
	}

	void render_patches(draw_info *di)
	{
		draw_buffer *db = &di->d->buffer;

		for (int i=0;i<db->patch_count;i++)
		{
			float mtx[16], uvmtx[16];
			kosmos::mat4_zero(mtx);
			kosmos::mat4_zero(uvmtx);

			int u0 = db->patches[i].u0;
			int v0 = db->patches[i].v0;
			int u1 = db->patches[i].u1;
			int v1 = db->patches[i].v1;

			const float cx = di->mps * ((u0+u1) / 2);
			const float cz = di->mps * ((v0+v1) / 2);
			const float sx = di->mps * ((u1 - u0) / 2);
			const float sz = di->mps * ((v1 - v0) / 2);

			// down
			mtx[0] = sx;
			mtx[5] = 1.0f;
			mtx[10] = sz;
			mtx[12] = cx;
			mtx[14] = cz;
			mtx[15] = 1;

			uvmtx[0] = (u1-u0) / 2;
			uvmtx[5] = (u1-u0) / 2;
			uvmtx[12] = (u0 + u1) / 2;
			uvmtx[13] = (v0 + v1) / 2;
			add_patch(di->d, tile_level(db, u0, v1, u1, v1), mtx, uvmtx);

			// up
			mtx[0] = -sx;
			mtx[10] = -sz;
			mtx[12] = cx;
			mtx[14] = cz;

			uvmtx[0] = -(u1-u0) / 2;
			uvmtx[5] = -(u1-u0) / 2;
			add_patch(di->d, tile_level(db, u0, v0, u1, v0), mtx, uvmtx);

			// left
			mtx[0] = 0;
			mtx[10] = 0;
			mtx[2] = sx;
			mtx[8] = -sz;

			uvmtx[0] = 0;
			uvmtx[1] = (u1-u0) / 2;
			uvmtx[4] = -(u1-u0) / 2;
			uvmtx[5] = 0;
			add_patch(di->d, tile_level(db, u0, v0, u0, v1), mtx, uvmtx);

			// right
			mtx[0] = 0;
			mtx[10] = 0;
			mtx[2] = -sx;
			mtx[8] = sz;

			uvmtx[0] = 0;
			uvmtx[1] = -(u1-u0) / 2;
			uvmtx[4] = (u1-u0) / 2;
			uvmtx[5] = 0;
			add_patch(di->d, tile_level(db, u1, v0, u1, v1), mtx, uvmtx);
		}
	}

	void cerr()
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
			KOSMOS_ERROR("gl error " << err);
	}

	data* create(outki::TerrainConfig *config)
	{
		data *d = new data();
		glGenBuffers(1, &d->vbo);

		d->sprog = kosmos::shader::program_get(config->Shader);
		d->config = config;

		if (!d->sprog)
		{
			KOSMOS_ERROR("No terrain program")
			return 0;
		}

		static terrain_vtx_t buf[1024*1024];
		terrain_vtx_t *end0 = make_patch(LEVEL_0_SAMPLES + 1, &buf[0]);
		terrain_vtx_t *end1 = make_patch(LEVEL_0_SAMPLES * 2 + 1, end0);

		d->verts_begin[0] = 0;
		d->verts_count[0] = end0 - buf;
		d->verts_begin[1] = end0 - buf;
		d->verts_count[1] = end1 - end0;

		glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(terrain_vtx_t) * (end1 - buf), buf, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &d->vao);
		glBindVertexArray(d->vao);

		d->heightmap_size = d->config->WindowSize;
		d->height_data = new float[d->heightmap_size * d->heightmap_size];
		for (uint32_t u=0;u!=d->heightmap_size;u++)
		{
			for (uint32_t v=0;v!=d->heightmap_size;v++)
			{
				d->height_data[v * d->heightmap_size + u] = get_height(u/d->config->SamplesPerMeter, v/d->config->SamplesPerMeter);
			}
		}

		glGenTextures(1, &d->heightmap);
		glBindTexture(GL_TEXTURE_2D, d->heightmap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, d->heightmap_size, d->heightmap_size, 0, GL_RED, GL_FLOAT, d->height_data);
		return d;
	}

	void free(data *d)
	{
		kosmos::shader::program_free(d->sprog);
		glDeleteBuffers(1, &d->vbo);
		delete d;
	}

	void draw_terrain(data *d, view* view)
	{
		c_do_tiles = 0;

		const int ws = d->config->WindowSize;
		const int spm = d->config->SamplesPerMeter;

		const int bias = 0x40000000;
		const int bias_tiles = bias / ws;
		const int bias_samples = bias_tiles * ws;

		int cam_u = int(view->viewpoint[0] * spm) + bias_samples;
		int cam_v = int(view->viewpoint[2] * spm) + bias_samples;

		// tile start
		cam_u = (cam_u / ws) - bias_tiles;
		cam_v = (cam_v / ws) - bias_tiles;

		draw_info di;
		di.d = d;
		di.v = *view;
		di.mps = 1.0f / spm;
		di.comp_r = view->r;

		draw_buffer_clear(&d->buffer);

		for (int u=-1;u!=2;u++)
		{
			for (int v=-1;v!=2;v++)
			{
				int tile_u = cam_u + u;
				int tile_v = cam_v + v;
				do_tile(&di, ws * tile_u, ws * tile_v, ws * tile_u + ws, ws * tile_v + ws);
			}
		}

		render_patches(&di);

		// -- actual drawing goes here.

		kosmos::shader::program_use(d->sprog);

		// binds to active buffer
		glBindVertexArray(d->vao);

		kosmos::shader::program_use(d->sprog);

		glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
		glVertexAttribPointer(kosmos::shader::find_attribute(d->sprog, "vpos"), 2, GL_FLOAT, GL_FALSE, sizeof(terrain_vtx_t), 0);
		glVertexAttribPointer(kosmos::shader::find_attribute(d->sprog, "tpos"), 2, GL_FLOAT, GL_FALSE, sizeof(terrain_vtx_t), (void*)8);
		glEnableVertexAttribArray(kosmos::shader::find_attribute(d->sprog, "vpos"));
		glEnableVertexAttribArray(kosmos::shader::find_attribute(d->sprog, "tpos"));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, d->heightmap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glUniform1i(kosmos::shader::find_uniform(d->sprog, "heightmap"), 0);
		glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "viewproj"), 1, GL_FALSE, view->view_mtx);

		int polys = 0;
		int drawcalls = 0;


		for (int i=0;i<draw_buffer::LEVELS;i++)
		{
			const int max_batch = 64;

			// uniform buffer
			kosmos::mat4f tileworld[2*max_batch];

			int left = d->buffer.tile_count[i];
			int pos = 0;
			while (left > 0)
			{
				int next = (left > max_batch) ? max_batch : left;
				for (int j=0;j<next;j++)
				{
					memcpy(tileworld[2*j+0], d->buffer.tile2world[i][pos], sizeof(kosmos::mat4f));
					memcpy(tileworld[2*j+1], d->buffer.tile2heightmap[i][pos], sizeof(kosmos::mat4f));
					pos++;
				}

				left -= next;

				glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "tileworld"), 2 * next, GL_FALSE, (float*)tileworld);

				glDrawArraysInstanced(GL_TRIANGLES, d->verts_begin[i], d->verts_count[i], next);
				polys += next * d->verts_count[i];
				drawcalls++;
			}
		}

		printf("draw calls %d polys %d tiles=%d\n", drawcalls, polys, c_do_tiles);
	}
	
}
