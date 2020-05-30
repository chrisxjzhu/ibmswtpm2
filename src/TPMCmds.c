/**
 *  Main Simulator Entry Point
 *  Written by Ken Goldman
 *  IBM Thomas J. Watson Research Center
 *  $Id: TPMCmds.c 1604 2020-04-03 18:50:29Z kgoldman $
 *  (c) Copyright IBM Corp. and others, 2016 - 2019
 */

/* D.5 TPMCmds.c */
/* D.5.1. Description */
/* This file contains the entry point for the simulator. */
/* D.5.2. Includes, Defines, Data Definitions, and Function Prototypes */
#include "TpmBuildSwitches.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#ifdef TPM_WINDOWS
#include <windows.h>
#include <winsock.h>
#endif
#include "TpmTcpProtocol.h"
#include "Manufacture_fp.h"
#include "Platform_fp.h"
#include "Simulator_fp.h"
#ifdef TPM_WINDOWS
#include "TcpServer_fp.h"
#endif
#ifdef TPM_POSIX
#include "TcpServerPosix_fp.h"
#endif
#include "TpmProfile.h" /* kgold */

#define PURPOSE "TPM Reference Simulator.\nCopyright Microsoft Corp.\n"
#define DEFAULT_TPM_PORT 2321

/* D.5.3. Functions */
/* D.5.3.1. Usage() */
/* This function prints the proper calling sequence for the simulator. */

static void Usage(const char *pszProgramName)
{
    fprintf(stderr, "%s", PURPOSE);
    fprintf(stderr, "Usage: %s [options..]\n", pszProgramName);
    fprintf(stderr, "  -port <port>\tStart the TPM server on port and port+1. The default ports are %d and %d.\n", DEFAULT_TPM_PORT, DEFAULT_TPM_PORT+1);
    fprintf(stderr, "  -rm\t\tRemanufacture the TPM before starting\n");
    fprintf(stderr, "  -h\t\tThis message\n");
    exit(1);
}

/* D.5.3.2. main() */
/* This is the main entry point for the simulator. */
/* It registers the interface and starts listening for clients */

int main(int argc, char *argv[])
{
    /* command line argument defaults */
    int manufacture = 0;
    int portNum = DEFAULT_TPM_PORT;
    int portNumPlat;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-rm") == 0) {
            manufacture = 1;
        } else if (strcmp(argv[i], "-port") == 0) {
            if (++i < argc) {
                portNum = atoi(argv[i]);
                if (portNum <= 0 || portNum > 65535) {
                    Usage(argv[0]);
                }
            } else {
                printf("Missing parameter for -port\n");
                Usage(argv[0]);
            }
        } else if (strcmp(argv[i], "-h") == 0) {
            Usage(argv[0]);
        } else {
            printf("'%s' is not a valid option\n", argv[i]);
            Usage(argv[0]);
        }
    }

    printf("LIBRARY_COMPATIBILITY_CHECK is %s\n",
        (LIBRARY_COMPATIBILITY_CHECK ? "ON" : "OFF"));

    // Enable NV memory
    _plat__NVEnable(NULL);
    
    if (manufacture || _plat__NVNeedsManufacture()) {
        printf("Manufacturing NV state...\n");

        if (TPM_Manufacture(1) != 0) {
            /* if the manufacture didn't work, then make sure that the NV
             * file doesn't survive. This prevents manufacturing failures
             * from being ignored the next time the code is run.
             */
            _plat__NVDisable(1);
            exit(1);
        }

        // Coverage test - repeated manufacturing attempt
        if (TPM_Manufacture(0) != 1) {
            exit(2);
        }

        // Coverage test - re-manufacturing
        TPM_TearDown();

        if (TPM_Manufacture(1) != 0) {
            exit(3);
        }
    }

    // Disable NV memory
    _plat__NVDisable(0);
    /* power on the TPM  - kgold MS simulator comes up powered off */
    _rpc__Signal_PowerOn(FALSE);
    _rpc__Signal_NvOn();

    portNumPlat = portNum + 1;
    StartTcpServer(&portNum, &portNumPlat);

    return EXIT_SUCCESS;
}
