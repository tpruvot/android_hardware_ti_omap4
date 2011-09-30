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
/*!
 *  @file       ProcMgrApp.c
 *
 *  @brief      Sample application for ProcMgr module
 *
 *  ============================================================================
 */

/* Linux specific header files */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>


/* Standard headers */
#include <Std.h>

/* OSAL & Utils headers */
#include <Trace.h>
#include <OsalPrint.h>

/* Module level headers */
#include <_MultiProc.h>
#include <ti/ipc/MultiProc.h>
#include <ProcMgr.h>
#include <omap4430proc.h>
#include <ProcDefs.h>
#include <_ProcMgrDefs.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/** ============================================================================
 *  Macros and types
 *  ============================================================================
 */
#define NUM_MEM_ENTRIES 9

/*!
 *  @brief  Position of reset vector memory region in the memEntries array.
 */
#define RESET_VECTOR_ENTRY_ID 0

/** ============================================================================
 *  Globals
 *  ============================================================================
 */
/*!
 *  @brief  Array of memory entries for OMAP3530
 */
static OMAP4430PROC_MemEntry memEntries [NUM_MEM_ENTRIES] =
{
    {
        "RESETCTRL",   /* NAME           : Name of the memory region */
        0x87E00000u,   /* PHYSADDR       : Physical address */
        0x87E00000u,   /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,  /* MASTERVIRTADDR : Master virtual address (if known) */
        0x80u,         /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? */
    },
    {
        "DDR2",        /* NAME           : Name of the memory region */
        0x87E00080u,   /* PHYSADDR       : Physical address */
        0x87E00080u,   /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,  /* MASTERVIRTADDR : Master virtual address (if known) */
        0x000FFF80u,   /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? */
    },
    {
        "SYSLINKMEM",  /* NAME           : Name of the memory region */
        0x87F00000u,   /* PHYSADDR       : Physical address */
        0x87F00000u,   /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,  /* MASTERVIRTADDR : Master virtual address (if known) */
        0x00030000u,   /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? */
    },
    {
        "POOLMEM",     /* NAME           : Name of the memory region */
        0x87F30000,    /* PHYSADDR       : Physical address */
        0x87F30000,    /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1u,  /* MASTERVIRTADDR : Master virtual address (if known) */
        0x000D0000u,   /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? Logically */
    },
    {
        "DSPIRAM",     /* NAME           : Name of the memory region */
        0x5c7f8000,    /* PHYSADDR       : Physical address */
        0x107f8000,    /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1,   /* MASTERVIRTADDR : Master virtual address (if known) */
        0x00018000,    /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? */
    },
    {
        "DSPL1PRAM",   /* NAME           : Name of the memory region */
        0x5cE00000,    /* PHYSADDR       : Physical address */
        0x10E00000,    /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1,   /* MASTERVIRTADDR : Master virtual address (if known) */
        0x00008000,    /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? */
    },
    {
        "DSPL1DRAM",   /* NAME           : Name of the memory region */
        0x5cF04000,    /* PHYSADDR       : Physical address */
        0x10F04000,    /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1,   /* MASTERVIRTADDR : Master virtual address (if known) */
        0x00014000,    /* SIZE           : Size of the memory region */
        TRUE,          /* SHARED         : Shared access memory? */
    },
    {
        "L4_CORE",     /* NAME           : Name of the memory region */
        0x48000000,    /* PHYSADDR       : Physical address */
        0x48000000,    /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1,   /* MASTERVIRTADDR : Master virtual address (if known) */
        0x01000000,    /* SIZE           : Size of the memory region */
        FALSE,         /* SHARED         : Shared access memory? */
    },
    {
        "L4_PER",      /* NAME           : Name of the memory region */
        0x49000000,    /* PHYSADDR       : Physical address */
        0x49000000,    /* SLAVEVIRTADDR  : Slave virtual address */
        (UInt32) -1,   /* MASTERVIRTADDR : Master virtual address (if known) */
        0x00100000,    /* SIZE           : Size of the memory region */
        FALSE,         /* SHARED         : Shared access memory? */
    }
};

/*!
 *  @brief  OMAP4430PROC instance object.
 */
typedef struct OMAP4430PROC_Object_tag {
    ProcMgr_CommonObject commonObj;
    /*!< Common object required to be the first field in the instance object
         structure. This is used by ProcMgr to get the handle to the kernel
         object. */
    UInt32               openRefCount;
    /*!< Reference count for number of times open/close were called in this
         process. */
    Bool                 created;
    /*!< Indicates whether the object was created in this process. */
    UInt16               procId;
    /*!< Processor ID */
} OMAP4430PROC_Object;

/** ============================================================================
 *   Dummy Function Definations used for the sample
 *  ============================================================================
 */

Int
PROC_attach_dummy (Handle handle, Processor_AttachParams * params)
{
    return 1;
}
Int
PROC_detach_dummy (Handle handle)
{
    return 1;
}

Int
PROC_start_dummy (Handle                  handle,
                    UInt32                  entryPt,
                    Processor_StartParams * params)
{
    return 1;
}

Int
PROC_stop_dummy (Handle handle)
{
    return 1;
}
Int
PROC_read_dummy (Handle   handle,
                   UInt32   procAddr,
                   UInt32 * numBytes,
                   Ptr      buffer)
{
    return 1;
}

Int
PROC_write_dummy (Handle   handle,
                    UInt32   procAddr,
                    UInt32 * numBytes,
                    Ptr      buffer)
{
    return 1;
}

Int
PROC_control_dummy (Handle handle, Int32 cmd, Ptr arg)
{
    return 1;
}

Int
PROC_translateAddr_dummy (Handle           handle,
                            Ptr *            dstAddr,
                            ProcMgr_AddrType dstAddrType,
                            Ptr              srcAddr,
                            ProcMgr_AddrType srcAddrType)
{
    return 1;
}

Int
PROC_map_dummy (Handle   handle,
                  UInt32   procAddr,
                  UInt32   size,
                  UInt32 * mappedAddr,
                  UInt32 * mappedSize,
                  UInt32   map_attribs)
{
    return 1;
}

Int
PROC_unmap_dummy (Handle  handle, UInt32 mapped_addr)
{
    return 1;
}


Int CallbackFxn_ProcMgr (UInt16        procId,
                         Handle        handle,
                         ProcMgr_State fromState,
                         ProcMgr_State toState)
{
    return 1;
}


/*!
 *  @brief  Handle to the ProcMgr instance used.
 */
ProcMgr_Handle ProcMgrApp_handle       = NULL;

Handle ProcMgrApp_handle_open  = NULL;

/*!
 *  @brief  Handle to the Processor instance used.
 */
Handle ProcMgrApp_procHandle   = NULL;


/** ============================================================================
 *  Functions
 *  ============================================================================
 */
/*!
 *  @brief  Function to execute the startup for ProcMgrApp sample application
 */
Int
ProcMgrApp_startup ()
{
    Int                   status = 0;
    ProcMgr_Config        config;
    OMAP4430PROC_Config   procConfig;
    OMAP4430PROC_Params   procParams;
    Handle                procHandle;
    ProcMgr_Params        params;
    ProcMgr_StartParams   startParams;
    ProcMgr_State         state;
    UInt16                procId;
    UInt32                mappedAddr;
    UInt32                mappedSize;
#if 0
    Ptr                   buffer;
    UInt32                numBytes;
    ProcMgr_ProcInfo      procInfo;
    Ptr                   dstAddr;
#endif
    UInt32 *              aBufferSend;
    UInt32                ulSendBufferSize = 0x2000;
    UInt32                i;


    Osal_printf ("Entered ProcMgrApp_startup\n");
    /* Setup ProcMgr and its subsidiary modules */
    ProcMgr_getConfig (&config);
    status = ProcMgr_setup (&config);
    if (status < 0) {
        Osal_printf ("Error in ProcMgr_setup [0x%x]\n", status);
    }
    else {
        Osal_printf ("ProcMgr_setup status: [0x%x]\n", status);
        OMAP4430PROC_getConfig (&procConfig);
        status = OMAP4430PROC_setup (&procConfig);
        if (status < 0) {
            Osal_printf ("Error in OMAP4430PROC_setup [0x%x]\n", status);
        }
        /* Get MultiProc ID by name. */
        procId = MultiProc_getId ("SysM3");
        /* Create an instance of the Processor object for OMAP3530 */
        OMAP4430PROC_Params_init (NULL, &procParams);
        procParams.numMemEntries       = NUM_MEM_ENTRIES;
        procParams.memEntries          = (OMAP4430PROC_MemEntry *) &memEntries;
        procParams.resetVectorMemEntry = RESET_VECTOR_ENTRY_ID;

        procHandle = OMAP4430PROC_create (procId, &procParams);
    }

    if (status >= 0) {
        Osal_printf ("MultiProc_getId procId: [0x%x]\n", procId);

        /* Initialize parameters */
        ProcMgr_Params_init (NULL, &params);

        params.procHandle = procHandle;
        ProcMgrApp_handle = ProcMgr_create (procId, &params);
        if (ProcMgrApp_handle == NULL) {
            Osal_printf ("Error in ProcMgr_create \n");
            status = PROCMGR_E_FAIL;
        }

//new for testing DMMs
        Osal_printf ("calling Malloc the buffer \n");

        aBufferSend = (UInt32 *) malloc (sizeof (UInt32) * ulSendBufferSize);
        if (aBufferSend == NULL) {
            Osal_printf ("Memory allocation failed.\n");
            status = -1;
        }
        else {
            Osal_printf ("Populating the buffer \n");
            /* Initialize buffer */
            for (i = 0;i < ulSendBufferSize;i++) {
                //populating the buffer with
                aBufferSend[i] = 0xfafa;
            }

            Osal_printf ("\n APPLICATION CALLING: ProcMgr_map \n");

            status = ProcMgr_map(ProcMgrApp_handle,
                                (UInt32)aBufferSend,
                                ulSendBufferSize,
                                &mappedAddr,
                                &mappedSize,
                                ProcMgr_MapType_Virt, procId);

            if (status == 0) {
                Osal_printf ("Error in ProcMgr_map [0x%x]\n", status);
            }
            else {
               Osal_printf ("ProcMgr_map  done[0x%x]\n", status);
            }
            Osal_printf ("Mapped address [0x%x]\n", mappedAddr);
            for (i = 0;i < ulSendBufferSize;i++) {
                aBufferSend[i] = 0xF0F0F0F0;
            }
            Osal_printf ("\n\n MAP DONE Check on the Ducati CCS memory window "
                        "for values written to memory address [0x%x] ...\n",
                        mappedAddr);
            Osal_printf ("Waiting ... Press Any key to continue with Unmap \n");
            getchar();

            Osal_printf ("\n\n APPLICATION CALLING: ProcMgr_unmap \n");
            status = ProcMgr_unmap(ProcMgrApp_handle,mappedAddr, procId);
            if (status == 0) {
                Osal_printf ("Error in ProcMgr_unmap [0x%x]\n", status );
            }
            else {
               Osal_printf ("ProcMgr_unmap  done[0x%x]\n", status);
            }
            Osal_printf ("UnMapped address [0x%x]\n", mappedAddr);
            Osal_printf ("\n\n UNMAP DONE Check on the Ducati CCS memory window "
                            "memory address [0x%x] ...\n", mappedAddr);
        }

        // delay after unmap so that we can connect the CCS
        Osal_printf ("\n\n Exiting ...\n");
        getchar();
        getchar();
        Osal_printf ("\n\n Exiting ...\n");
        getchar();

#if 0     //ProcMgrApp_handle_open = ProcMgr_create (2, &params);
         status = ProcMgr_open(&ProcMgrApp_handle_open, 2);
         if (status < 0) {
             Osal_printf ("Error in ProcMgr_open [0x%x]\n", status);
          }
         else
             Osal_printf ("ProcMgr_open done[0x%x]\n", status);

        Osal_printf ("\n\n****\n\n");
          numBytes=10;
        buffer = (Ptr *) Memory_calloc (NULL,(10* sizeof (int)), 0);
        status = ProcMgr_read (ProcMgrApp_handle,
                              0x5000,
                              &numBytes,
                              buffer);
        if (status == 0) {
           Osal_printf ("Error in ProcMgr_read [0x%x]\n", status);
        }
        else
           Osal_printf ("ProcMgr_read done[0x%x]\n", status);
        Osal_printf ("\n\n****\n\n");
            status = ProcMgr_write (ProcMgrApp_handle,
                                   0x5000,
                                    &numBytes,
                                   buffer);
        if (status == 0) {
           Osal_printf ("Error in ProcMgr_write [0x%x]\n", status);
        }
        else
           Osal_printf ("ProcMgr_write done[0x%x]\n", status);
        Osal_printf ("\n\n****\n\n");
           status = ProcMgr_translateAddr (ProcMgrApp_handle,
                                        &dstAddr,
                                        ProcMgr_AddrType_MasterKnlVirt,
                                        (Ptr )0x6000,
                                        ProcMgr_AddrType_SlaveVirt);
        if (status == 0) {
            Osal_printf ("Error in ProcMgr_translateAddr [0x%x]\n", status);
        }
        else
            Osal_printf ("ProcMgr_translateAddr  done[0x%x]\n", status);
        Osal_printf ("\n\n****\n\n");
        status = ProcMgr_map (ProcMgrApp_handle,
                              0x500,
                              10,
                              &mappedAddr,
                              &mappedSize,
                              ProcMgr_MapType_Virt);
        if (status == 0) {
            Osal_printf ("Error in ProcMgr_map [0x%x]\n", status);
        }
        else
           Osal_printf ("ProcMgr_map  done[0x%x]\n", status);
        Osal_printf ("\n\n****\n\n");
        status = ProcMgr_registerNotify (ProcMgrApp_handle,
                                        CallbackFxn_ProcMgr,
                                        NULL,
                                        NULL);
        if (status == 0) {
              Osal_printf ("Error in ProcMgr_registerNotify [0x%x]\n", status);
        }
        else
            Osal_printf ("ProcMgr_registerNotify  done[0x%x]\n", status);
        Osal_printf ("\n\n****\n\n");
        status = ProcMgr_getProcInfo (ProcMgrApp_handle,
                                  &procInfo);
        if (status == 0) {
               Osal_printf ("Error in ProcMgr_getProcInfo [0x%x]\n", status);
        }
        else
           Osal_printf ("ProcMgr_getProcInfo  done[0x%x]\n", status);


#endif


            if (status >= 0) {

                if (status < 0) {
                    Osal_printf ("Error in ProcMgr_load [0x%x]\n", status);
                }
                else {
                    Osal_printf ("ProcMgr_load status: [0x%x]\n", status);
                    state = ProcMgr_getState (ProcMgrApp_handle);
                    Osal_printf ("After load: ProcMgr_getState\n"
                                 "    state [0x%x]\n",
                                 state);

                    ProcMgr_getStartParams (ProcMgrApp_handle, &startParams);
                    status = ProcMgr_start (ProcMgrApp_handle, 0, &startParams);
                    if (status < 0) {
                        Osal_printf ("ProcMgr_start failed [0x%x]\n", status);
                    }
                    else {
                        Osal_printf ("ProcMgr_start passed [0x%x]\n", status);
                        state = ProcMgr_getState (ProcMgrApp_handle);
                        Osal_printf ("After start: ProcMgr_getState\n"
                                     "    state [0x%x]\n",
                                     state);
                    }
                }
            }
    }
    Osal_printf ("Leaving ProcMgrApp_startup\n");
    Osal_printf ("*********************************************************\n");

    return 0;
}


/*!
 *  @brief  Function to execute the shutdown for ProcMgrApp sample application
 */
Int
ProcMgrApp_shutdown (Void)
{
    Int                 status = 0;
    ProcMgr_State       state;
    ProcMgr_StopParams  stop_params;

    Osal_printf ("Entered ProcMgrApp_shutdown\n");

    stop_params.proc_id = PROC_SYSM3;
    if (ProcMgrApp_handle != NULL) {
        status = ProcMgr_stop (ProcMgrApp_handle, &stop_params);
        Osal_printf ("ProcMgr_stop status: [0x%x]\n", status);
        state = ProcMgr_getState (ProcMgrApp_handle);
        Osal_printf ("After stop: ProcMgr_getState\n"
                     "    state [0x%x]\n",
                     state);

        //status = ProcMgr_unload (ProcMgrApp_handle, ProcMgrApp_fileId) ;
        //Osal_printf ("ProcMgr_unload status: [0x%x]\n", status);

        state = ProcMgr_getState (ProcMgrApp_handle);
        Osal_printf ("After unload: ProcMgr_getState\n"
                     "    state [0x%x]\n",
                     state);

        status = ProcMgr_detach (ProcMgrApp_handle);
        Osal_printf ("ProcMgr_detach status: [0x%x]\n", status);

        //state = ProcMgr_getState (ProcMgrApp_handle);
        //Osal_printf ("After detach: ProcMgr_getState\n"
        //            "    state [0x%x]\n",
        //            state);
    }


    if (ProcMgrApp_handle != NULL) {
        Osal_printf (" calling ProcMgr_delete  [0x%x]\n");
        status = ProcMgr_delete (&ProcMgrApp_handle);
        Osal_printf ("ProcMgr_delete completed [0x%x]\n", status);
    }

    ProcMgr_destroy ();
    Osal_printf ("ProcMgr_destroy completed\n");

    Osal_printf ("Leaving ProcMgrApp_shutdown\n");
    Osal_printf ("*********************************************************\n");

    return 0;
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
