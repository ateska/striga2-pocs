#include "greenev.h"

int main(int argc, char const *argv[])
{
	assert(ev_version_major() == EV_VERSION_MAJOR);
	assert(ev_version_minor() >= EV_VERSION_MINOR);

	logging_init();
	application app;
	application_ctor(&app);

	int ret = application_run(&app);
	LOG_INFO("Exiting (exit code: %d)", ret);
	
	application_dtor(&app);
	logging_finish();

	return ret;
}
