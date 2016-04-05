/** AceUnit test header file for package foo.
 *
 * @warning This is a generated file. Do not edit. Your changes will be lost.
 * @file foo.h
 */

#include "AceUnit.h"

#ifdef ACEUNIT_SUITES

extern TestSuite_t gmalloc_testFixture;
extern TestSuite_t file_testFixture;
extern TestSuite_t lock_testFixture;

const TestSuite_t *suitesOf1[] = {
    &gmalloc_testFixture,
    &file_testFixture,
    &lock_testFixture,
    NULL
};

#if defined __cplusplus
extern
#endif
const TestSuite_t suite1 = {
    1,
#ifndef ACEUNIT_EMBEDDED
    "foo",
#endif
    suitesOf1
};

#endif
