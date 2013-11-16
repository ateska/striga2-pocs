#include "singlev.h"

int main(int argc, char **argv)
{
	int ret = 99;

	assert(ev_version_major() == EV_VERSION_MAJOR);
	assert(ev_version_minor() >= EV_VERSION_MINOR);

	logging_init();

	LOG_INFO("Exiting (exit code: %d)", ret);
	
	logging_finish();

	return ret;
}
