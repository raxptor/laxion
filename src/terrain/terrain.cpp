#include "terrain.h"

#include <kosmos/glwrap/gl.h>
#include <kosmos/core/mat4.h>
#include <kosmos/render/shader.h>
#include <kosmos/log/log.h>

#include <math.h>
#include <stdio.h>

namespace terrain
{
	struct terrain_vtx_t
	{
		float x, y;
		float u, v;
	};
	
	int in_layer(int layer)
	{
		return 1 + 1 * layer;
	}
    
    enum
    {
        LEVEL_0_SAMPLES = 32,
        LEVEL_1_SAMPLES = LEVEL_0_SAMPLES * 2
    };
    
	
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
				{/*
					*out++ = K;
					*out++ = L;
					*out++ = L+1;*/
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
		return sinf(x*0.01f + z*0.03f) * 10.0;
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

	enum {
		MAX_INSTANCES = 64
	};

	struct data
	{
		outki::TerrainConfig *config;
		kosmos::shader::program *sprog;
		GLuint vbo, vao;
		int level0_begin, level0_end;
		int level1_begin, level1_end;

		float lvl0_mtx[MAX_INSTANCES*2*16];
		float lvl1_mtx[MAX_INSTANCES*2*16];
		int lvl0_count, lvl1_count;
    
        float *height_data;
        uint32_t heightmap_size;
        GLuint heightmap;
	};

	static long long s_polys = 0, s_drawcalls = 0;

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
			glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "tileworld"), 2 * d->lvl0_count, GL_FALSE, d->lvl0_mtx);
			glDrawArraysInstanced(GL_TRIANGLES, d->level0_begin, d->level0_end - d->level0_begin, d->lvl0_count);

			s_polys += d->lvl0_count * (d->level0_end-d->level0_begin) / 3;
			s_drawcalls++;
            
            d->lvl0_count = 0;
		}
		if (d->lvl1_count > lim)
		{
			glUniformMatrix4fv(kosmos::shader::find_uniform(d->sprog, "tileworld"), 2 * d->lvl1_count, GL_FALSE, d->lvl1_mtx);
			glDrawArraysInstanced(GL_TRIANGLES, d->level1_begin, d->level1_end - d->level1_begin, d->lvl1_count);
            
			s_polys += d->lvl1_count * (d->level1_end-d->level1_begin) / 3;
			s_drawcalls++;
            
            d->lvl1_count = 0;
		}
	}

	void draw_patch(data *d, int index, float *mtx_world, float *mtx_uv)
	{
		flush(d, false);

		if (index == 0)
		{
			memcpy(&d->lvl0_mtx[d->lvl0_count * 32], mtx_world, 16*sizeof(float));
            memcpy(&d->lvl0_mtx[d->lvl0_count * 32 + 16], mtx_uv, 16*sizeof(float));
			d->lvl0_count++;
		}
		else
		{
			memcpy(&d->lvl1_mtx[d->lvl1_count * 32], mtx_world, 16*sizeof(float));
            memcpy(&d->lvl1_mtx[d->lvl1_count * 32 + 16], mtx_uv, 16*sizeof(float));
			d->lvl1_count++;
		}
	}
    
    struct draw_info
    {
        data *d;
        view v;
        float mps; // meters per sample
        float comp_r;
    };
    
    float edge_scaling(draw_info *di, int u0, int v0, int u1, int v1)
    {
        const float x0 = di->mps * u0;
        const float z0 = di->mps * v0;
        const float x1 = di->mps * u1;
        const float z1 = di->mps * v1;
        const float y0 = get_height(x0, z0);
        const float y1 = get_height(x1, z1);
        float dx0 = x0 - di->v.viewpoint[0];
        float dy0 = y0 - di->v.viewpoint[1];
        float dz0 = z0 - di->v.viewpoint[2];
        float dx1 = x1 - di->v.viewpoint[0];
        float dy1 = y1 - di->v.viewpoint[1];
        float dz1 = z1 - di->v.viewpoint[2];
        float d0 = dx0*dx0 + dy0*dy0 + dz0*dz0;
        float d1 = dx1*dx1 + dy1*dy1 + dz1*dz1;
        float d = d0 < d1 ? d0 : d1;
        return sqrtf((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0)+(z1-z0)*(z1-z0)) / sqrtf(d);
    }
    
    void do_tile(draw_info *di, int u0, int v0, int u1, int v1);

    void do_tile_subdivide(draw_info *di, int u0, int v0, int u1, int v1)
    {
        int cu = (u0 + u1) / 2;
        int cv = (v0 + v1) / 2;
        do_tile(di, u0, v0, cu, cv);
        do_tile(di, cu, v0, u1, cv);
        do_tile(di, u0, cv, cu, v1);
        do_tile(di, cu, cv, u1, v1);
    }
    
    inline bool allow_subdivide(draw_info *di, int u0, int v0, int u1, int v1)
    {
        return (u1 - u0) > LEVEL_1_SAMPLES;
    }
    
    inline bool within_range(draw_info *di, int u, int v)
    {
        const float dx = (di->mps * u - di->v.viewpoint[0]);
        const float dy = (di->mps * v - di->v.viewpoint[2]);
        return dx*dx + dy*dy < di->v.max_range_sqr;
    }
    
    void do_tile(draw_info *di, int u0, int v0, int u1, int v1)
	{
        const bool r0 = within_range(di, u0, v0);
        const bool r1 = within_range(di, u1, v0);
        const bool r2 = within_range(di, u0, v1);
        const bool r3 = within_range(di, u1, v1);
        const bool can_subdivide = allow_subdivide(di, u0, v0, u1, v1);

        if (!r0 && !r1 && !r2 && !r3)
        {
            int tx = di->mps * (u1 - u0);
            if (tx*tx < 0.0001f * di->v.max_range_sqr)
                return;
            
            if (can_subdivide)
            {
                do_tile_subdivide(di, u0, v0, u1, v1);
            }
            return;
        }
        
		const float s0 = di->comp_r * edge_scaling(di, u0, v0, u1, v0);
		const float s1 = di->comp_r * edge_scaling(di, u0, v0, u0, v1);
		const float s2 = di->comp_r * edge_scaling(di, u1, v1, u1, v0);
		const float s3 = di->comp_r * edge_scaling(di, u1, v1, u0, v1);

        if (can_subdivide && (s0 > 1 || s1 > 1 || s2 > 1 || s3 > 1))
		{
            do_tile_subdivide(di, u0, v0, u1, v1);
			return;
		}

        const float cx = di->mps * ((u0+u1) / 2);
		const float cz = di->mps * ((v0+v1) / 2);
		const float sx = di->mps * ((u1 - u0) / 2);
		const float sz = di->mps * ((v1 - v0) / 2);
		float mtx[16], uvmtx[16];
		kosmos::mat4_zero(mtx);
        kosmos::mat4_zero(uvmtx);
        
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
        
		draw_patch(di->d, level(s3), mtx, uvmtx);
        
		// up
		mtx[0] = -sx;
		mtx[10] = -sz;
		mtx[12] = cx;
		mtx[14] = cz;
        
        uvmtx[0] = -(u1-u0) / 2;
        uvmtx[5] = -(u1-u0) / 2;
		draw_patch(di->d, level(s0), mtx, uvmtx);

        // left
		mtx[0] = 0;
		mtx[10] = 0;
		mtx[2] = -sx;
		mtx[8] = sz;


        uvmtx[0] = 0;
        uvmtx[1] = -(u1-u0) / 2;
        uvmtx[4] = (u1-u0) / 2;
        uvmtx[5] = 0;
        
		draw_patch(di->d, level(s1), mtx, uvmtx);
		// right
		mtx[0] = 0;
		mtx[10] = 0;
		mtx[2] = sx;
		mtx[8] = -sz;
        
        uvmtx[0] = 0;
        uvmtx[1] = (u1-u0) / 2;
        uvmtx[4] = -(u1-u0) / 2;
        uvmtx[5] = 0;
    
		draw_patch(di->d, level(s2), mtx, uvmtx);
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

		d->level0_begin = 0;
		d->level0_end = end0 - buf;
		d->level1_begin = d->level0_end;
		d->level1_end = end1 - buf;

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
               uint32_t x = (u&1)^(v&1);
                d->height_data[v * d->heightmap_size + u] = x ? 0.1f : 0.0f;//get_height(u/d->config->SamplesPerMeter, v/d->config->SamplesPerMeter);
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
        
        s_drawcalls = 0;
        s_polys = 0;
        
        for (int u=-1;u!=2;u++)
        {
            for (int v=-1;v!=2;v++)
            {
                int tile_u = cam_u + u;
                int tile_v = cam_v + v;
                
                do_tile(&di, ws * tile_u, ws * tile_v, ws * tile_u + ws, ws * tile_v + ws);            }
        }
        
        flush(d, true);
        printf("draw calls: %lld polys: %lld\n", s_drawcalls, s_polys);
    }
}
