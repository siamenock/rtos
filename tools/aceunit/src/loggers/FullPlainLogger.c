/* Copyright (c) 2007 - 2011, Christian Hujer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the AceUnit developers nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** Full Logger implementation that logs plain text messages.
 * The format of the messages is suitable for processing in popular text editors like EMacs or Vim.
 * It is the same as that of error messages yielded by most popular compilers like gcc or javac.
 * The format is:
 * <p><code><var>filename</var>:<var>linenumber</var>: error: in <var>symbolnname</var>: <var>message</var></p>
 * For example:
 * <p><samp>foo.c:30: error: Expected X to be 15 but was 13.</samp></p>
 * @author <a href="mailto:cher@riedquat.de">Christian Hujer</a>
 * @file FullPlainLogger.c
 */

#include <stdio.h>

#include "AceUnit.h"
#include "AceUnitLogging.h"

/* Current error case */
static TestCaseId_t errorCase;
/** @see TestLogger_t#testCaseFailed()} */
static void testCaseFailed(const AssertionError_t *recentError) {
#ifdef ACEUNIT_EMBEDDED
#error "FullPlainLogger does not support embedded AceUnit."
#else
	printf("[     FAIL ] %s:%d: %s\n", recentError->assertionId.filename, (int) recentError->assertionId.lineNumber, recentError->assertionId.message);

	errorCase = recentError->testId; 
#endif
}

/** @see TestLogger_t#testCaseEnded()} */
static void testCaseEnded(TestCaseId_t testCase) {
#ifdef ACEUNIT_EMBEDDED
#error "FullPlainLogger does not support embedded AceUnit."
#else
	if(testCase != errorCase)
		printf("[ PASS     ]\n");
#endif
	printf("[==========]\n");
}

/* Counter of test cases */
static int count;
/** @see TestLogger_t#testCaseStarted()} */
static void testCaseStarted(TestCaseId_t testCase) {
#ifdef ACEUNIT_EMBEDDED
#error "FullPlainLogger does not support embedded AceUnit."
#else
	const TestSuite_t *const *suites = suite1.suites;
	if(suites != NULL) {
		for(; NULL != *suites; suites++) {
			TestFixture_t* fixture = (TestFixture_t*)*suites;
			const testMethod_t* test = fixture->testCase;

			for(int i = 0; NULL != *test; i++, test++) {
				if(testCase == fixture->testIds[i]) 
					printf("[ Test %03d ] %s\n", ++count, fixture->testNames[i]);
			}
		}
	}
#endif
}

/** This Logger. */
AceUnitNewLogger(
	FullPlainLogger,
	NULL,
	NULL,
	NULL,
	testCaseStarted,
	testCaseFailed,
	testCaseEnded,
	NULL,
	NULL,
	NULL
	);

TestLogger_t *globalLogger = &FullPlainLogger; /* XXX Hack. Remove. */
