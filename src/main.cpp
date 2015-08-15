#include <iostream>

#include <putki/pkgloader.h>
#include <putki/pkgmgr.h>
#include <putki/config.h>
#include <putki/liveupdate/liveupdate.h>
#include <putki/types.h>

#include <environment/appwindow.h>
#include <kosmos/render/render.h>
#include <kosmos/render/render2d.h>
#include <kosmos/render/shader.h>
#include <kosmos/core/mat4.h>
#include <kosmos/log/log.h>

#include <cassert>
#include <stdio.h>
#include <math.h>

#include <kosmos/glwrap/gl.h>
#include <outki/types/laxion/terrain.h>

#include "terrain/terrain.h"

// binding up the blob loads.
namespace outki
{
	void bind_kosmos_loaders();
	void bind_laxion_loaders();
}

namespace
{
	laxion::appwindow::data *window;
	putki::liveupdate::data *liveupdate;
}

kosmos::shader::program *prog = 0;
kosmos::render2d::stream *stream = 0;

float camera_pos[3] = {3, 0, 0};
float gtime = 0;

putki::pkgmgr::loaded_package *s_pkg = 0;
outki::TerrainConfig *s_terrain_config = 0;
terrain::data *s_terrain;


void load()
{
	s_pkg = putki::pkgloader::from_file("terrain.pkg");
	if (!s_pkg)
	{
		KOSMOS_ERROR("Failed to load package");
	}
	s_terrain_config = (outki::TerrainConfig*) putki::pkgmgr::resolve(s_pkg, "main-terrain");
	if (!s_terrain_config)
	{
		KOSMOS_ERROR("failed to resolve terrain");
	}
}

void camera_matrix(float *out)
{
	float persp[16], rot[16], trans[16];
	kosmos::mat4_rot_x(rot, 3.1415f * 0.2f);
	kosmos::mat4_trans(trans, -camera_pos[0], -camera_pos[1], -camera_pos[2]);
	kosmos::mat4_persp(persp, 1.20f, 0.90f, 1.0f, 1000.0f);
	kosmos::mul_mat4(out, persp, rot, trans);
}

// from camera viewpoint
void draw_world(float *viewproj)
{
	LIVE_UPDATE(&s_terrain_config);
	
	terrain::mapping m;
	m.samples_per_meter = 10.0f;

	int x0, y0, x1, y1;
	terrain::compute_tiles(&m, camera_pos, 900.0f, &x0, &y0, &x1, &y1);

	terrain::params p;
	memcpy(p.viewpoint, camera_pos, 3*sizeof(float));
	memcpy(&p.view_mtx[0], viewproj, 4*4*sizeof(float));

	p.r = 1.4f;
	p.terrain = s_terrain_config;
	terrain::draw_terrain_tiles(s_terrain, &m, &p, x0, y0, x1, y1);
}

void frame(laxion::appwindow::input_batch *input, float deltatime)
{
	if (!s_terrain)
	{
		s_terrain = terrain::create(s_terrain_config);
	}

	int x0, y0, x1, y1;
	laxion::appwindow::get_client_rect(window, &x0, &y0, &x1, &y1);
	
	kosmos::render::begin(x1-x0, y1-x0, true, true, 0xff00ff);

	float out[16];
	gtime += 3.3f * deltatime;
	camera_pos[0] = sinf(0.3f * gtime) * 100;
	camera_pos[1] = 10.0f + sinf(0.35f * gtime) * 4.0f;
	camera_pos[2] = -20.0f * gtime + cosf(0.05f * gtime) * 100;

	camera_matrix(out);

	draw_world(out);

	kosmos::render::end();

	if (liveupdate)
	{
		if (!putki::liveupdate::connected(liveupdate))
		{
			putki::liveupdate::disconnect(liveupdate);
			liveupdate = 0;
		}
		else
		{
			putki::liveupdate::update(liveupdate);
		}
	}
}

int main(int argc, char *argv[])
{
	KOSMOS_INFO("Laxion v0.1");
	
	outki::bind_kosmos_loaders();
	outki::bind_laxion_loaders();
	
	putki::liveupdate::init();

	load();

	window = laxion::appwindow::create("Laxion", 800, 600, 0);

	liveupdate = putki::liveupdate::connect();
	
	laxion::appwindow::run_loop(window, &frame);


	return 0;
}
