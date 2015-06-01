#include <iostream>

#include <putki/pkgloader.h>
#include <putki/pkgmgr.h>
#include <putki/config.h>
#include <putki/liveupdate/liveupdate.h>

#include <environment/appwindow.h>
#include <kosmos/render/render.h>
#include <kosmos/render/render2d.h>
#include <kosmos/render/shader.h>
#include <kosmos/log/log.h>

#include <cassert>
#include <stdio.h>

#include <kosmos/glwrap/gl.h>


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

void frame(laxion::appwindow::input_batch *input, float deltatime)
{
	int x0, y0, x1, y1;
	laxion::appwindow::get_client_rect(window, &x0, &y0, &x1, &y1);
	
	kosmos::render::begin(x1-x0, y1-x0, true, true, 0xff00ff);
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
