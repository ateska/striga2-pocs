#include "unit_tests.h"

///

Suite * check_cmdq_suite (void)
{
	Suite *s = suite_create("greenev");

	/* Core test case */
	TCase *tc_core = tcase_create("cmdq");
	tcase_add_test(tc_core, unittest_cmdq_01);
	tcase_add_test(tc_core, unittest_cmdq_02);
	suite_add_tcase(s, tc_core);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s = check_cmdq_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
