/** AceUnit test header file for package foo.
 *
 * @warning This is a generated file. Do not edit. Your changes will be lost.
 * @file foo.h
 */

#include "AceUnit.h"

#ifdef ACEUNIT_SUITES

extern TestSuite_t fileFixture;
extern TestSuite_t gmallocFixture;

const TestSuite_t *suitesOf1[] = {
    &fileFixture,
    &gmallocFixture,
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
