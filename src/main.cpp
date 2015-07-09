#include <iostream>

#include <putki/pkgloader.h>
#include <putki/pkgmgr.h>
#include <putki/config.h>
#include <putki/liveupdate/liveupdate.h>

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

void camera_matrix(float *out)
{
	float persp[16], rot[16], trans[16];

	kosmos::mat4_rot_x(rot, 0.30f);
	kosmos::mat4_trans(trans, -camera_pos[0], -camera_pos[1], -camera_pos[2]);
	kosmos::mat4_persp(persp, 1.20f, 0.90f, 1.0f, 10.0f);
	kosmos::mul_mat4(out, persp, rot, trans);
}

void frame(laxion::appwindow::input_batch *input, float deltatime)
{
	int x0, y0, x1, y1;
	laxion::appwindow::get_client_rect(window, &x0, &y0, &x1, &y1);
	
	kosmos::render::begin(x1-x0, y1-x0, true, true, 0xff00ff);

	glMatrixMode(GL_PROJECTION);

	float out[16];

	gtime += 10*deltatime;
	camera_pos[0] = sinf(0.03f * gtime) * 100;
	camera_pos[1] = sinf(0.35f * gtime) * 10;
	camera_pos[2] = cosf(0.05f * gtime) * 100;

	camera_matrix(out);
	glLoadMatrixf(out);

	terrain::params p;
	memcpy(p.viewpoint, camera_pos, 3*sizeof(float));
	p.r = 5.0f;
	terrain::draw_terrain(-1000, -1000, 1000, 1000, &p);

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
	
	window = laxion::appwindow::create("Laxion", 800, 600, 0);

	liveupdate = putki::liveupdate::connect();
	
	laxion::appwindow::run_loop(window, &frame);
	
	return 0;
}
