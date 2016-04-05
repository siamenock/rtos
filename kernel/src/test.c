#include <stdio.h>
#include "AceUnitData.h"

/* 
 * The number of test cases contained in this suite.
 * This is used to verify that the number of executed test cases is correct.
 */
#define TEST_CASES_FOR_VERIFICATION 6

int run_tests(void) {
	int ret = 0;
	runSuite(&suite1);

	if(runnerData->testCaseFailureCount != 0) {
		printf("Test Cases: %d  Errors: %d\n", (int) runnerData->testCaseCount, (int) runnerData->testCaseFailureCount);
		ret = -1;
	}
	if(runnerData->testCaseCount != TEST_CASES_FOR_VERIFICATION) {
		printf("Test Cases: %d but expected %d\n", (int) runnerData->testCaseCount, TEST_CASES_FOR_VERIFICATION);
		ret = -1;
	}

	printf("Test Cases: %d  Success: %d\n", (int) runnerData->testCaseCount,
		       	(int) runnerData->testCaseCount - runnerData->testCaseFailureCount);
	return ret;
}
