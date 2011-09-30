/*
 *  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*============================================================================
 *  @file   TestApp.c
 *
 *  @brief  OS-specific sample application framework for RCM module
 *
 *  ============================================================================
 */


/* OS-specific headers */
#include <stdio.h>
#include <stdlib.h>

/* OSAL & Utils headers */
#include <OsalPrint.h>

/* RCM headers */
#include <RcmClient.h>
#include <RcmServer.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/* Sample test functions */
extern Int MpuRcmServerTest (Int testCase);

extern Int MpuRcmClientTest (Int testCase);


Void printUsage (Void)
{
    Osal_printf ("Usage: ./rcm_singletest.out <Test#> <Sub Test#>\n"
                    "Options:\n");
    Osal_printf ("\t./rcm_singletest.out 1 1 : "
                    "MPU RCM Client <--> SysM3 RCM Server\n");
    Osal_printf ("\t./rcm_singletest.out 1 2 : "
                    "MPU RCM Server <--> SysM3 RCM Client\n");
    Osal_printf ("\t./rcm_singletest.out 2 1 : "
                    "MPU RCM Client <--> AppM3 RCM Server\n");
    Osal_printf ("\t./rcm_singletest.out 2 2 : "
                    "MPU RCM Server <--> AppM3 RCM Client\n");
    Osal_printf ("\t./rcm_singletest.out 3 1 : "
                    "MPU RCM Client <--> Tesla RCM Server\n");
    Osal_printf ("\t./rcm_singletest.out 3 2 : "
                    "MPU RCM Server <--> Tesla RCM Client\n");

    return;
}

/*
 *  ======== main ========
 */
Int main (Int argc, Char * argv [])
{
    Int status      = 0;
    Int testNo;
    Int subTestNo;

    Osal_printf ("\n== RCM Client or Server Sample ==\n");

    if (argc < 3) {
        printUsage ();
        goto exit;
    }

    testNo = atoi (argv[1]);
    subTestNo = atoi (argv[2]);
    if ((testNo < 1 || testNo > 3) || (subTestNo != 1 && subTestNo != 2)) {
        printUsage ();
        goto exit;
    }

    /* Run RCM client test */
    if(subTestNo == 1) {
        Osal_printf ("RCM Client sample invoked\n");
        status = MpuRcmClientTest (testNo);
        if (status < 0) {
            Osal_printf ("Error in RCM Client sample\n");
        }
    }

    /* Run RCM server test */
    if(subTestNo == 2) {
        Osal_printf ("RCM Server sample invoked\n");
        status = MpuRcmServerTest (testNo);
        if (status < 0)
            Osal_printf ("Error in RCM Server sample\n");
    }

exit:
    Osal_printf ("\n== Sample End ==\n");

    /* Trace for TITAN support */
    if (status < 0)
        Osal_printf ("test_case_status=%d\n", status);
    else
        Osal_printf ("test_case_status=0\n");

    return status;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
