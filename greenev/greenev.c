#include "greenev.h"

int main(int argc, char **argv)
{
	int ret = 99;
	struct application app;

	assert(ev_version_major() == EV_VERSION_MAJOR);
	assert(ev_version_minor() >= EV_VERSION_MINOR);

	logging_init();
	application_ctor(&app, argc, argv);

	ret = application_run(&app);
	LOG_INFO("Exiting (exit code: %d)", ret);
	
	application_dtor(&app);
	logging_finish();

	return ret;
}
