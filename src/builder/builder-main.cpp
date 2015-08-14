#include <putki/builder/build.h>
#include <putki/builder/builder.h>
#include <putki/builder/package.h>


// generated.
namespace inki
{
	void bind_kosmos();
//	void bind_ccg_ui();
	void bind_laxion();
}

//void ccg_ui_register_handlers(putki::builder::data *builder);
void laxion_register_handlers(putki::builder::data *builder);
void kosmos_register_handlers(putki::builder::data *builder);

void app_register_handlers(putki::builder::data *builder)
{
	kosmos_register_handlers(builder);
//	ccg_ui_register_handlers(builder);
	laxion_register_handlers(builder);
}

void app_build_packages(putki::db::data *out, putki::build::packaging_config *pconf)
{
	{
		putki::package::data *pkg = putki::package::create(out);
		putki::package::add(pkg, "main-terrain", true);
		putki::build::commit_package(pkg, pconf, "terrain.pkg");
	}
}

int run_putki_builder(int argc, char **argv);

int main(int argc, char **argv)
{
	inki::bind_laxion();
//	inki::bind_ccg_ui();
	inki::bind_kosmos();

	putki::builder::set_builder_configurator(&app_register_handlers);
	putki::builder::set_packager(&app_build_packages);

	return run_putki_builder(argc, argv);
}
