#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "proxy.h"

static EFI_SYSTEM_TABLE system_table_template = {
        {
                EFI_SYSTEM_TABLE_SIGNATURE,                                           // Signature
                EFI_SYSTEM_TABLE_REVISION,                                            // Revision
                sizeof(EFI_SYSTEM_TABLE),                                            // HeaderSize
                0,                                                                    // CRC32
                0                                                                     // Reserved
        },
        NULL,                                                                   // FirmwareVendor
        0,                                                                      // FirmwareRevision
        NULL,                                                                   // ConsoleInHandle
        NULL,                                                                   // ConIn
        NULL,                                                                   // ConsoleOutHandle
        NULL,                                                                   // ConOut
        NULL,                                                                   // StandardErrorHandle
        NULL,                                                                   // StdErr
        NULL,                                                                   // RuntimeServices
        NULL,                                                         // BootServices
        0,                                                                      // NumberOfConfigurationTableEntries
        NULL                                                                    // ConfigurationTable
};

extern EFI_BOOT_SERVICES m_boot_services_template;

EFI_STATUS ProxyCreateStub(EFI_SYSTEM_TABLE **out_system_table_stub) {
    EFI_STATUS status;
    EFI_SYSTEM_TABLE *system_table_stub = NULL;
    EFI_BOOT_SERVICES *boot_services_stub = NULL;

    status = gBS->AllocatePool(EfiLoaderData, sizeof(system_table_template), (VOID **) &system_table_stub);
    if (EFI_ERROR(status)) {
        goto done;
    }

    status = gBS->AllocatePool(EfiLoaderData, sizeof(m_boot_services_template), (VOID **) &boot_services_stub);
    if (EFI_ERROR(status)) {
        goto done;
    }

    CopyMem(system_table_stub, &system_table_template, sizeof(system_table_template));
    CopyMem(boot_services_stub, &m_boot_services_template, sizeof(m_boot_services_template));

    boot_services_stub->GetNextMonotonicCount = gBS->GetNextMonotonicCount;
    boot_services_stub->CalculateCrc32 = gBS->CalculateCrc32;
    boot_services_stub->InstallMultipleProtocolInterfaces = gBS->InstallMultipleProtocolInterfaces;
    boot_services_stub->UninstallMultipleProtocolInterfaces = gBS->UninstallMultipleProtocolInterfaces;

    boot_services_stub->Hdr.CRC32 = 0;
    gBS->CalculateCrc32(
            (UINT8 *) &boot_services_stub->Hdr,
            boot_services_stub->Hdr.HeaderSize,
            &boot_services_stub->Hdr.CRC32
    );

    system_table_stub->FirmwareVendor = gST->FirmwareVendor;
    system_table_stub->FirmwareRevision = gST->FirmwareRevision;
    system_table_stub->ConsoleInHandle = gST->ConsoleInHandle;
    system_table_stub->ConsoleOutHandle = gST->ConsoleOutHandle;
    system_table_stub->StandardErrorHandle = gST->StandardErrorHandle;
    system_table_stub->RuntimeServices = gST->RuntimeServices;
    system_table_stub->NumberOfTableEntries = gST->NumberOfTableEntries;
    system_table_stub->ConfigurationTable = gST->ConfigurationTable;
    system_table_stub->BootServices = boot_services_stub;

    system_table_stub->Hdr.CRC32 = 0;
    gBS->CalculateCrc32(
            (UINT8 *) &system_table_stub->Hdr,
            system_table_stub->Hdr.HeaderSize,
            &system_table_stub->Hdr.CRC32
    );

    *out_system_table_stub = system_table_stub;

    done:
    if (!EFI_ERROR(status)) {
        return status;
    }

    if (system_table_stub) {
        gBS->FreePool(system_table_stub);
    }
    if (boot_services_stub) {
        gBS->FreePool(boot_services_stub);
    }

    return status;
}

VOID ProxyTaskTransfer(PROXY_TASK *task) {
    task->status = kProxyTaskWaiting;

    while (1) {
        int next_rear;
        AcquireSpinLock(&m_proxy_server.task_lock);
        next_rear = (m_proxy_server.task_rear + 1) % PROXY_TASK_QUEUE_SIZE;
        if (next_rear == m_proxy_server.task_front) {
            // QUEUE IS FULL
            ReleaseSpinLock(&m_proxy_server.task_lock);
            for (int i=0; i<10; i++) {
                CpuPause();
            }
        } else {
            m_proxy_server.task_queue[m_proxy_server.task_rear] = 0;
            m_proxy_server.task_rear = next_rear;
            ReleaseSpinLock(&m_proxy_server.task_lock);
            break;
        }
    }

    while (task->status < kProxyTaskFinished) {
        CpuPause();
    }
}


VOID* ProxyStubCall0(VOID* fn) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 0;
    ProxyTaskTransfer(&task);
    return task.ret;
}

VOID* ProxyStubCall1(VOID* fn, VOID* a1) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 1;
    task.args[0] = a1;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall2
VOID* ProxyStubCall2(VOID* fn, VOID* a1, VOID* a2) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 2;
    task.args[0] = a1;
    task.args[1] = a2;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall3
VOID* ProxyStubCall3(VOID* fn, VOID* a1, VOID* a2, VOID* a3) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 3;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall4
VOID* ProxyStubCall4(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 4;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall5
VOID* ProxyStubCall5(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 5;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    task.args[4] = a5;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall6
VOID* ProxyStubCall6(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 6;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    task.args[4] = a5;
    task.args[5] = a6;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall7
VOID* ProxyStubCall7(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 7;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    task.args[4] = a5;
    task.args[5] = a6;
    task.args[6] = a7;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall8
VOID* ProxyStubCall8(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7, VOID* a8) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 8;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    task.args[4] = a5;
    task.args[5] = a6;
    task.args[6] = a7;
    task.args[7] = a8;
    ProxyTaskTransfer(&task);
    return task.ret;
}

// ProxyStubCall9
VOID* ProxyStubCall9(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7, VOID* a8, VOID* a9) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 9;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    task.args[4] = a5;
    task.args[5] = a6;
    task.args[6] = a7;
    task.args[7] = a8;
    task.args[8] = a9;
    ProxyTaskTransfer(&task);
    return task.ret;
}

VOID* ProxyStubCall10(VOID* fn, VOID* a1, VOID* a2, VOID* a3, VOID* a4, VOID* a5, VOID* a6, VOID* a7, VOID* a8, VOID* a9, VOID* a10) {
    PROXY_TASK task;
    task.fn = fn;
    task.arg_n = 10;
    task.args[0] = a1;
    task.args[1] = a2;
    task.args[2] = a3;
    task.args[3] = a4;
    task.args[4] = a5;
    task.args[5] = a6;
    task.args[6] = a7;
    task.args[7] = a8;
    task.args[8] = a9;
    task.args[9] = a10;
    ProxyTaskTransfer(&task);
    return task.ret;
}
