#include "unit_tests.h"

///

int unittest_cmdq_01_result;
void * unittest_cmdq_01_res_a;
void * unittest_cmdq_01_res_cmd_arg;

void unittest_cmdq_01_cb(void * a, struct cmd cmd)
{
	unittest_cmdq_01_result += cmd.id;
	unittest_cmdq_01_res_cmd_arg = cmd.arg;
	unittest_cmdq_01_res_a = a;
}

void unittest_cmdq_01(int _i CK_ATTRIBUTE_UNUSED)
/*
Simple test of command queue object.
Just construct that, insert two commands and destroy
*/
{
	struct watcher_cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	unittest_cmdq_01_result = 0;
	unittest_cmdq_01_res_a = NULL;

	// Construct command queue
	cmd_q = watcher_cmd_q_new(loop, 256, unittest_cmdq_01_cb, (void *)0xABCD12EF);
	ck_assert_ptr_ne(cmd_q, NULL);

	watcher_cmd_q_start(cmd_q);

	// Insert one command, iterate event loop once and check result
	watcher_cmd_q_put(cmd_q, 1, (void *)0x1357AFDE);
	ev_run(loop, EVRUN_NOWAIT);
	ck_assert_int_eq(unittest_cmdq_01_result, 1);
	ck_assert_ptr_eq(unittest_cmdq_01_res_a, (void *)0xABCD12EF);
	ck_assert_ptr_eq(unittest_cmdq_01_res_cmd_arg, (void *)0x1357AFDE);

	// Insert second command, iterate event loop once and check result
	watcher_cmd_q_put(cmd_q, 2, NULL);
	ev_run(loop, EVRUN_NOWAIT);
	ck_assert_int_eq(unittest_cmdq_01_result, 3);
	ck_assert_ptr_eq(unittest_cmdq_01_res_cmd_arg, NULL);

	// Finalize
	watcher_cmd_q_stop(cmd_q);
	watcher_cmd_q_delete(cmd_q);
}

///

void unittest_cmdq_02(int _i CK_ATTRIBUTE_UNUSED)
{
	bool ret;
	unsigned int q_depth = 256;
	struct watcher_cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	// Construct command queue
	cmd_q = watcher_cmd_q_new(loop, q_depth, unittest_cmdq_01_cb, NULL);
	ck_assert_ptr_ne(cmd_q, NULL);
	watcher_cmd_q_start(cmd_q);

	// Insert maximum number of commands
	int expected_result = 0;
	unittest_cmdq_01_result = 0;
	for (unsigned int i=0; i<255; i++)
	{
		ret = watcher_cmd_q_put(cmd_q, i, NULL);
		ck_assert_int_eq(ret, true);
		expected_result += i;
	}

	// Insert one more - this should fail ...
	ret = watcher_cmd_q_put(cmd_q, -1, NULL);
	ck_assert_int_eq(ret, false);

	// Process all events in the queue
	ev_run(loop, EVRUN_NOWAIT);

	// Check result
	ck_assert_int_eq(unittest_cmdq_01_result, expected_result);

	// Finalize
	watcher_cmd_q_stop(cmd_q);
	watcher_cmd_q_delete(cmd_q);
}

///

int unittest_cmdq_03_result[3];
int unittest_cmdq_03_rescnt;

void unittest_cmdq_03_cb(void * a, struct cmd cmd)
{
	unittest_cmdq_03_result[unittest_cmdq_03_rescnt++] = cmd.id;
}

void unittest_cmdq_03(int _i CK_ATTRIBUTE_UNUSED)
/*
Check that command queue is keeping order of commands intact
*/
{
	struct watcher_cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	unittest_cmdq_03_rescnt = 0;
	unittest_cmdq_03_result[0] = -1;
	unittest_cmdq_03_result[1] = -1;
	unittest_cmdq_03_result[2] = -1;

	// Construct command queue
	cmd_q = watcher_cmd_q_new(loop, 256, unittest_cmdq_03_cb, NULL);
	watcher_cmd_q_start(cmd_q);

	// Insert one command, iterate event loop once and check result
	watcher_cmd_q_put(cmd_q, 1, NULL);
	watcher_cmd_q_put(cmd_q, 2, NULL);

	// Process all events in the queue
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_int_eq(unittest_cmdq_03_result[0], 1);
	ck_assert_int_eq(unittest_cmdq_03_result[1], 2);
	ck_assert_int_eq(unittest_cmdq_03_result[2], -1);

	// Finalize
	watcher_cmd_q_stop(cmd_q);
	watcher_cmd_q_delete(cmd_q);
}
