#ifndef GREENEV_APP_H_
#define GREENEV_APP_H_

typedef struct
{
	int exit_status;

	ev_signal _SIGINT;
	ev_signal _SIGTERM;
} application;

// Singleton guard and global application instance pointer
extern application * app;

void application_ctor(application *);
void application_dtor(application *);

int application_run(application *);

#endif //GREENEV_APP_H_
