#pragma once

namespace laxion
{
	namespace appwindow
	{
		struct data;

		struct input_batch
		{
			// ccgui::mouse_input mouse;
		};

		typedef void (*updatefunc)(input_batch *input, float deltatime);

		data* create(const char *title, int width, int height, const char *iconfile);

		void destroy(data *);
		void set_title(data *, const char *title);
		void get_client_rect(data *d, int *x0, int *y0, int *x1, int *y1);
		void run_loop(data *d, updatefunc f);

#if defined(WIN32)
		void* hwnd(data *);
#endif
	}
}
