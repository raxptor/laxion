#include "terrain.h"

#include <kosmos/glwrap/gl.h>

#include <math.h>
#include <stdio.h>

namespace terrain
{
	float get_height(float x, float z)
	{
		return -3.f * sinf(x*0.03f) + 4.0f * cosf(z*0.05f);
	}

	float edge_scaling(params* p, float x0, float z0, float x1, float z1)
	{
		const float y0 = get_height(x0, z0);
		const float y1 = get_height(x1, z1);

		const float cx = 0.5f*(x0+x1);
		const float cy = 0.5f*(y0+y1);
		const float cz = 0.5f*(z0+z1);
		
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

	void do_tile(params* p, float x0, float z0, float x1, float z1)
	{
		const float s0 = p->r * edge_scaling(p, x0, z0, x1, z0);
		const float s1 = p->r * edge_scaling(p, x0, z0, x0, z1);
		const float s2 = p->r * edge_scaling(p, x1, z1, x1, z0);
		const float s3 = p->r * edge_scaling(p, x1, z1, x0, z1);

		if (s0 > 1 || s1 > 1 || s2 > 1 || s3 > 1)
		{
			float cx = .5f * (x0 + x1);
			float cz = .5f * (z0 + z1);
			do_tile(p, x0, z0, cx, cz);
			do_tile(p, cx, z0, x1, cz);
			do_tile(p, x0, cz, cx, z1);
			do_tile(p, cx, cz, x1, z1);
			return;
		}

		float x = (x0 - p->viewpoint[0]);
		float y = get_height(x0, z0) - p->viewpoint[1];
		float z = (z0 - p->viewpoint[2]);
		float s = 4 * sqrtf(x*x + y*y + z*z);

		glColor3f(s, s, s);
		glVertex3f(x0, get_height(x0, z0), z0);
		glVertex3f(x1, get_height(x1, z0), z0);
		glVertex3f(x0, get_height(x0, z0), z0);
		glVertex3f(x0, get_height(x0, z1), z1);

		glVertex3f(x1, get_height(x1, z0), z0);
		glVertex3f(x1, get_height(x1, z1), z1);
		glVertex3f(x0, get_height(x0, z1), z1);
		glVertex3f(x1, get_height(x1, z1), z1);
	}


	void draw_terrain(float x0, float z0, float x1, float z1, params* p)
	{
		glLineWidth(1.0f);
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);

		do_tile(p, x0, z0, x1, z1);

		glEnd();
	}
}
