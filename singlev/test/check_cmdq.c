#include "unit_tests.h"

///

int unittest_cmdq_01_result_cnt;
int unittest_cmdq_01_result_cmdid;
void * unittest_cmdq_01_result_data;

void unittest_cmdq_01_cb(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd_q * cmd_q = w->data;
	struct cmd cmd;

	while (true)
	{
		int qsize = cmd_q_get(cmd_q, &cmd);
		if (qsize < 0) break;

		unittest_cmdq_01_result_cnt++;
		unittest_cmdq_01_result_cmdid += cmd.id;
		unittest_cmdq_01_result_data = cmd.data;

		if (qsize == 0) break;
	}
}

void unittest_cmdq_01(int _i CK_ATTRIBUTE_UNUSED)
/*
Simple test of command queue object.
Just construct that, insert two commands and destroy
*/
{
	struct cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	unittest_cmdq_01_result_cnt = 0;
	unittest_cmdq_01_result_cmdid = 0;
	unittest_cmdq_01_result_data = NULL;

	// Construct command queue
	cmd_q = cmd_q_new(5, unittest_cmdq_01_cb);
	ck_assert_ptr_ne(cmd_q, NULL);

	cmd_q_start(cmd_q, loop);

	// Insert one command, iterate event loop once and check result
	cmd_q_put(cmd_q, loop, 1, (void *)0x1357AFDE);
	ev_run(loop, EVRUN_NOWAIT);
	ck_assert_int_eq(unittest_cmdq_01_result_cnt, 1);
	ck_assert_int_eq(unittest_cmdq_01_result_cmdid, 1);
	ck_assert_ptr_eq(unittest_cmdq_01_result_data, (void *)0x1357AFDE);

	// Insert second command, iterate event loop once and check result
	unittest_cmdq_01_result_cmdid = 0;
	cmd_q_put(cmd_q, loop, 2, NULL);
	ev_run(loop, EVRUN_NOWAIT);
	ck_assert_int_eq(unittest_cmdq_01_result_cnt, 2);
	ck_assert_int_eq(unittest_cmdq_01_result_cmdid, 2);
	ck_assert_ptr_eq(unittest_cmdq_01_result_data, NULL);

	// Finalize
	cmd_q_stop(cmd_q, loop);
	cmd_q_delete(cmd_q);
}

///

void unittest_cmdq_02(int _i CK_ATTRIBUTE_UNUSED)
/*
Check reaction to 'queue full' situation
*/
{
	bool ret;
	unsigned int q_depth = 256;
	struct cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	// Construct command queue
	cmd_q = cmd_q_new(q_depth, unittest_cmdq_01_cb);
	ck_assert_ptr_ne(cmd_q, NULL);
	cmd_q_start(cmd_q, loop);

	// Insert maximum number of commands
	int expected_result = 0;
	unittest_cmdq_01_result_cnt = 0;
	unittest_cmdq_01_result_cmdid = 0;
	for (unsigned int i=0; i<(q_depth-1); i++)
	{
		ret = cmd_q_put(cmd_q, loop, i, NULL);
		ck_assert_int_eq(ret, true);
		expected_result += i;
	}

	// Insert one more - this should fail ...
	ret = cmd_q_put(cmd_q, loop, -1, NULL);
	ck_assert_int_eq(ret, false);

	// Process all events in the queue
	ev_run(loop, EVRUN_NOWAIT);

	// Check result
	ck_assert_int_eq(unittest_cmdq_01_result_cnt, q_depth-1);
	ck_assert_int_eq(unittest_cmdq_01_result_cmdid, expected_result);

	// Finalize
	cmd_q_stop(cmd_q, loop);
	cmd_q_delete(cmd_q);
}

///

int unittest_cmdq_03_result[3];
int unittest_cmdq_03_rescnt;

void unittest_cmdq_03_cb(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd_q * cmd_q = w->data;
	struct cmd cmd;

	while (true)
	{
		int qsize = cmd_q_get(cmd_q, &cmd);
		if (qsize < 0) break;

		unittest_cmdq_03_result[unittest_cmdq_03_rescnt++] = cmd.id;

		if (qsize == 0) break;
	}
}


void unittest_cmdq_03(int _i CK_ATTRIBUTE_UNUSED)
/*
Check that command queue is keeping order of commands intact
*/
{
	struct cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	unittest_cmdq_03_rescnt = 0;
	unittest_cmdq_03_result[0] = -1;
	unittest_cmdq_03_result[1] = -1;
	unittest_cmdq_03_result[2] = -1;

	// Construct command queue
	cmd_q = cmd_q_new(256, unittest_cmdq_03_cb);
	cmd_q_start(cmd_q, loop);

	// Insert one command, iterate event loop once and check result
	cmd_q_put(cmd_q, loop, 1, NULL);
	cmd_q_put(cmd_q, loop, 2, NULL);

	// Process all events in the queue
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_int_eq(unittest_cmdq_03_result[0], 1);
	ck_assert_int_eq(unittest_cmdq_03_result[1], 2);
	ck_assert_int_eq(unittest_cmdq_03_result[2], -1);

	// Finalize
	cmd_q_stop(cmd_q, loop);
	cmd_q_delete(cmd_q);
}

///

int unittest_cmdq_04_result_cnt;

void unittest_cmdq_04_cb(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd_q * cmd_q = w->data;
	struct cmd cmd;
	int ref_val = 7;

	while (true)
	{
		int qsize = cmd_q_get(cmd_q, &cmd);
		
		ck_assert_int_eq(qsize, ref_val);
		ref_val--;

		unittest_cmdq_04_result_cnt++;

		if (qsize < 0) break;
		if (qsize == 0) break;
	}
}


void unittest_cmdq_04(int _i CK_ATTRIBUTE_UNUSED)
/*
Check that remaining queue size is reported well in callback
*/
{
	bool ret;
	unsigned int q_depth = 12;
	struct cmd_q * cmd_q;
	struct ev_loop * loop = ev_default_loop(EVFLAG_NOENV | EVFLAG_NOSIGMASK);

	unittest_cmdq_04_result_cnt = 0;

	// Construct command queue
	cmd_q = cmd_q_new(q_depth, unittest_cmdq_04_cb);
	ck_assert_ptr_ne(cmd_q, NULL);
	cmd_q_start(cmd_q, loop);

	// Insert first batch of commands
	for (unsigned int i=0; i<8; i++)
	{
		ret = cmd_q_put(cmd_q, loop, i, NULL);
		ck_assert_int_eq(ret, true);
	}

	// Process all events in the queue
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_int_eq(unittest_cmdq_04_result_cnt, 8);

	// Insert second batch of commands (now overflow should happen)
	for (unsigned int i=0; i<8; i++)
	{
		ret = cmd_q_put(cmd_q, loop, i, NULL);
		ck_assert_int_eq(ret, true);
	}

	// Process all events in the queue
	ev_run(loop, EVRUN_NOWAIT);

	ck_assert_int_eq(unittest_cmdq_04_result_cnt, 16);

	// Finalize
	cmd_q_stop(cmd_q, loop);
	cmd_q_delete(cmd_q);
}
