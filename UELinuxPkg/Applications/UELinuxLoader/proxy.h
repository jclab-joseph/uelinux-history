#include <Uefi.h>
#include <Library/SynchronizationLib.h>

enum PROXY_TASK_STATUS {
    kProxyTaskWaiting,
    kProxyTaskProcessing,
    kProxyTaskFinished,
};

typedef struct _PROXY_TASK {
    VOID* fn;
    int   arg_n;
    VOID* args[10];
    VOID *ret;
    volatile UINT32 status;
} PROXY_TASK;

#define PROXY_TASK_QUEUE_SIZE 16

typedef struct _PROXY_SERVER {
    SPIN_LOCK task_lock;
    PROXY_TASK *task_queue[PROXY_TASK_QUEUE_SIZE];
    int task_front; // first data
    int task_rear; // can be insert
} PROXY_SERVER;

extern PROXY_SERVER m_proxy_server;

VOID ProxyServerInit(PROXY_SERVER *server);
BOOLEAN ProxyHandle();
EFI_STATUS ProxyCreateStub(EFI_SYSTEM_TABLE **out_system_table_stub);

VOID ProxyTaskTransfer(PROXY_TASK *task);
VOID* ProxyStubCall0(VOID* fn);
VOID* ProxyStubCall1(VOID* fn, VOID* a1);
VOID* ProxyStubCall2(VOID* fn, VOID* a1, VOID* a2);
VOID* ProxyStubCall3(VOID* fn, VOID* a1, VOID* a2, VOID* a3);
VOID* ProxyStubCall4(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4);
VOID* ProxyStubCall5(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5);
VOID* ProxyStubCall6(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6);
VOID* ProxyStubCall7(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7);
VOID* ProxyStubCall8(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7, VOID* a8);
VOID* ProxyStubCall9(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7, VOID* a8, VOID* a9);
VOID* ProxyStubCall10(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7, VOID* a8, VOID* a9, VOID* a10);