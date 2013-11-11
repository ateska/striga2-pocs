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
{
	struct cmd_q cmd_q;
	struct ev_loop * loop = ev_default_loop(0);

	unittest_cmdq_01_result = 0;
	unittest_cmdq_01_res_a = NULL;

	// Construct command queue
	cmd_q_ctor(&cmd_q, loop, unittest_cmdq_01_cb, (void *)0xABCD12EF);
	cmd_q_start(&cmd_q);

	// Insert one command, iterate event loop once and check result
	cmd_q_insert(&cmd_q, 1, (void *)0x1357AFDE);
	ev_run(loop, EVRUN_NOWAIT);
	ck_assert_int_eq(unittest_cmdq_01_result, 1);
	ck_assert_ptr_eq(unittest_cmdq_01_res_a, (void *)0xABCD12EF);
	ck_assert_ptr_eq(unittest_cmdq_01_res_cmd_arg, (void *)0x1357AFDE);

	// Insert second command, iterate event loop once and check result
	cmd_q_insert(&cmd_q, 2, NULL);
	ev_run(loop, EVRUN_NOWAIT);
	ck_assert_int_eq(unittest_cmdq_01_result, 3);
	ck_assert_ptr_eq(unittest_cmdq_01_res_cmd_arg, NULL);

	// Finalize
	cmd_q_stop(&cmd_q);
	cmd_q_dtor(&cmd_q);
}
