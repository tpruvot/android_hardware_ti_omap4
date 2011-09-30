/*
 *  testlib.h
 *
 *  Unit test interface API.
 *
 *  Copyright (C) 2009-2011 Texas Instruments, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TESTLIB_H_
#define _TESTLIB_H_

/* error type definitions */
#define TESTLIB_UNAVAILABLE -65378
#define TESTLIB_INVALID        -1

#define T(test) ++i; \
    if (!id || i == id) printf("TEST #% 3d - %s\n", i, #test); \
    if (i == id) { \
        printf("TEST_DESC - "); \
        fflush(stdout); \
        return __internal__TestLib_Report(test); \
    }

/* test run function that must be defined from the test app */

/**
 * Runs a specified test by id, or lists all test cases.  This
 * function uses the TESTS macros, and defines each T(test) to
 * run a test starting from id == 1, and then return the result.
 *
 * @author a0194118 (9/7/2009)
 *
 * @param id   Test case id, or 0 if only listing test cases
 *
 * @return Summary result: TEST_RESULT_OK, FAIL, INVALID or
 *         UNAVAILABLE.
 */
#define TESTS_ \
    int __internal__TestLib_DoList(int id) { int i = 0;

#define _TESTS \
    return TESTLIB_INVALID; }

#define DEFINE_TESTS(TESTS) TESTS_ TESTS _TESTS

/* internal function prototypes and defines */
extern int __internal__TestLib_Report(int res);
extern void __internal__TestLib_NullFn(void *ptr);

#define nullfn __internal__TestLib_NullFn

/**
 * Parses argument list, prints usage on error, lists test
 * cases, runs tests and reports results.
 *
 * @author a0194118 (9/12/2009)
 *
 * @param argc      Number of test arguments
 * @param argv      Test argument array
 * @param init_fn   Initialization function of void fn(void *).
 *                  This is called before the testing.
 * @param exit_fn   Deinit function of void fn(void *).  This is
 *                  done after the testing concludes.
 * @param ptr       Custom pointer that is passed into the
 *                  initialization functions.
 *
 * @return # of test cases failed, 0 on success, -1 if no tests
 *         were run because of an error or a list request.
 */
int TestLib_Run(int argc, char **argv, void(*init_fn)(void *),
                void(*exit_fn)(void *), void *ptr);

#endif
