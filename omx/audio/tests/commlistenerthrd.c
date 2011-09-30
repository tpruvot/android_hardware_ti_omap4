#include "tiomxplayer.h"

void* TIOMX_CommandListener(void* pThreadData){
    APP_DPRINT("Entering Command listener \n");
    char key;
    int error=0;
    fd_set rfds;
    appPrivateSt * appPrvt = (appPrivateSt *)pThreadData;
    struct timeval tv;
    int retval;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    while(!error && !appPrvt->comthrdstop){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        retval = select(1, &rfds, NULL, NULL, &tv);
        if(0 == retval)
            continue;
        else
            scanf(" %c",&key);

        switch (tolower(key)){
        case 'p':
            APP_DPRINT("Pausing \n");
            /* FIX ME */
            /*  We need to implement buffer control logic in order to get rid of this flag */
            appPrvt->stateToPause=1;
            error = SetOMXState(appPrvt, OMX_StatePause);
            break;
        case 'r':
            APP_DPRINT("Resuming \n");
            error = SetOMXState(appPrvt, OMX_StateExecuting);
            break;
        default:
            APP_DPRINT("Command not recognized \n");
            break;
            };
    }
    APP_DPRINT("Exiting Command Listener \n");

    return (void*)error;
}
