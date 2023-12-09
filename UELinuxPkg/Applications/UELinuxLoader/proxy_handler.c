#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/BaseMemoryLib.h>

#include "proxy.h"

PROXY_SERVER m_proxy_server;

typedef VOID* (EFIAPI* fnTemplateArg0)();
typedef VOID* (EFIAPI* fnTemplateArg1)(VOID*);
typedef VOID* (EFIAPI* fnTemplateArg2)(VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg3)(VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg4)(VOID*, VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg5)(VOID*, VOID*, VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg6)(VOID*, VOID*, VOID*, VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg7)(VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg8)(VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg9)(VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*);
typedef VOID* (EFIAPI* fnTemplateArg10)(VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*, VOID*);

static VOID *ProxyTaskExecute(PROXY_TASK* task) {
    ASSERT(task->fn != NULL);
    switch (task->arg_n) {
        case 0:
            return ((fnTemplateArg0) task->fn)();
        case 1:
            return ((fnTemplateArg1) task->fn)(task->args[0]);
        case 2:
            return ((fnTemplateArg2) task->fn)(task->args[0], task->args[1]);
        case 3:
            return ((fnTemplateArg3) task->fn)(task->args[0], task->args[1], task->args[2]);
        case 4:
            return ((fnTemplateArg4) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3]);
        case 5:
            return ((fnTemplateArg5) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3], task->args[4]);
        case 6:
            return ((fnTemplateArg6) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3], task->args[4], task->args[5]);
        case 7:
            return ((fnTemplateArg7) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3], task->args[4], task->args[5], task->args[6]);
        case 8:
            return ((fnTemplateArg8) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3], task->args[4], task->args[5], task->args[6], task->args[7]);
        case 9:
            return ((fnTemplateArg9) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3], task->args[4], task->args[5], task->args[6], task->args[7], task->args[8]);
        case 10:
            return ((fnTemplateArg10) task->fn)(task->args[0], task->args[1], task->args[2], task->args[3], task->args[4], task->args[5], task->args[6], task->args[7], task->args[8], task->args[9]);
    }
    return 0;
}

VOID ProxyServerInit(PROXY_SERVER *server) {
    ZeroMem(server, sizeof(*server));
    InitializeSpinLock(&server->task_lock);
}


BOOLEAN ProxyHandle() {
    PROXY_TASK* task = NULL;

    AcquireSpinLock(&m_proxy_server.task_lock);
    if (m_proxy_server.task_front != m_proxy_server.task_rear) {
        // has task
        task = m_proxy_server.task_queue[m_proxy_server.task_front];
        m_proxy_server.task_front = (m_proxy_server.task_front + 1) % PROXY_TASK_QUEUE_SIZE;
    }
    ReleaseSpinLock(&m_proxy_server.task_lock);

    if (task) {
        if (task->status != kProxyTaskWaiting) {
            DEBUG((DEBUG_INFO, "status = %d\n", task->status));
            ASSERT(task->status == kProxyTaskWaiting);
        }
        task->status = kProxyTaskProcessing;
        task->ret = ProxyTaskExecute(task);
        task->status = kProxyTaskFinished;
    }

    // AcquireSpinLock(&m_proxy_server.task_lock);
    // m_proxy_server.task_front = (m_proxy_server.task_front + 1) % PROXY_TASK_QUEUE_SIZE;
    // ReleaseSpinLock(&m_proxy_server.task_lock);

    return task != NULL;
}
