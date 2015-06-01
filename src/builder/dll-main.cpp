#include <putki/data-dll/dllinterface.h>
#include <putki/builder/builder.h>

namespace inki
{
	void bind_kosmos();
	void bind_kosmos_editor();
//	void bind_ccg_ui();
//	void bind_ccg_ui_editor();
	void bind_laxion();
	void bind_laxion_editor();
}

//void ccg_ui_register_handlers(putki::builder::data *builder);
void laxion_register_handlers(putki::builder::data *builder);

void setup_builder(putki::builder::data *builder)
{
//	ccg_ui_register_handlers(builder);
	laxion_register_handlers(builder);
}

extern "C"
{
	#if defined(_MSC_VER)
	__declspec(dllexport) putki::data_dll_i* __cdecl load_data_dll(const char *data_path)
	#else
	putki::data_dll_i* load_data_dll(const char *data_path)
	#endif
	{
		inki::bind_kosmos();
		inki::bind_kosmos_editor();
//		inki::bind_ccg_ui();
//		inki::bind_ccg_ui_editor();
		inki::bind_laxion();
		inki::bind_laxion_editor();

		putki::builder::set_builder_configurator(&setup_builder);

		// bind at startup.
		return putki::create_dll_interface(data_path);
	}

}