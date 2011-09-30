#include <Std.h>
#include <OsalPrint.h>

#include <UsrUtilsDrv.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */



Void MemoryTest(Void);
Void GateTest(Void);
Void ListTest(Void);

Int main (Int argc, Char * argv [])
{
    UsrUtilsDrv_setup();

    MemoryTest();
    GateTest();
    ListTest();

    UsrUtilsDrv_destroy();

    return 0;
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
