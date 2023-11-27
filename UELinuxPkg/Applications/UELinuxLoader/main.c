#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

//VOID EFIAPI BlockIo2ReadNotifyFunc (
//        IN  EFI_EVENT                Event,
//        IN  VOID                     *Context
//)
//{
//    Print(L"EVENTTTTTTTTTTTTTTTT\n");
//}

EFI_LOADED_IMAGE_PROTOCOL* loader_li;
EFI_MP_SERVICES_PROTOCOL*  mp_services;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *current_fs;

UINTN num_processors;
UINTN num_enabled_processors;

static VOID DumpCpus();
EFI_STATUS StartImage();

VOID EFIAPI LinuxCpuProcedure (VOID* Parameter) {
    EFI_MP_SERVICES_PROTOCOL *mp_services = (EFI_MP_SERVICES_PROTOCOL*) Parameter;
    UINTN ProcessorNumber;
    EFI_STATUS status = mp_services->WhoAmI(mp_services, &ProcessorNumber);
    (void) status;
    // Print(L"HELLO I AM TWO CPU: %d/%d\n", status, ProcessorNumber);
}


EFI_STATUS EFIAPI LoaderMain(
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
) {
    EFI_STATUS  status;

    status = gBS->HandleProtocol(
            ImageHandle,
            &gEfiLoadedImageProtocolGuid,
            (VOID**)&loader_li
            );
    if (EFI_ERROR(status)) {
        Print(L"HandleProtocol(gEfiLoadedImageProtocolGuid) failed: 0x%x\n", status);
        return 1;
    }

    status = gBS->HandleProtocol(
            loader_li->DeviceHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&current_fs
            );
    if (EFI_ERROR(status)) {
        Print(L"HandleProtocol(gEfiSimpleFileSystemProtocolGuid) failed: 0x%x\n", status);
        return 1;
    }

    status = gBS->LocateProtocol (
            &gEfiMpServiceProtocolGuid,
            NULL,
            (VOID**)&mp_services
    );
    if (EFI_ERROR(status)) {
        Print(L"LocateProtocol(gEfiMpServiceProtocolGuid) failed: 0x%p\n", status);
        return 1;
    }

    // Determine number of processors
    status = mp_services->GetNumberOfProcessors(mp_services, &num_processors, &num_enabled_processors);
    if (EFI_ERROR (status))
    {
        Print( L"mp_services->GetNumEnabledProcessors:Unable to determine number of processors\n") ;
        return EFI_UNSUPPORTED;
    }

    if (num_enabled_processors < 2) {
        Print( L"ASSERT(NumberOfEnabledProcessors >= 2): NumberOfEnabledProcessors=%u\n", num_enabled_processors) ;
        return 1;
    }

    DumpCpus();
    status = StartImage();
    if (EFI_ERROR(status)) {
        return status;
    }

//    EFI_EVENT Event;
//
//    // Create an Event, required to call StartupThisAP in non-blocking mode
//    status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, &Event);
//    if(status == EFI_SUCCESS)
//    {
//        Print(L"Successful Event creation.\n");
//        status = mp_services->StartupThisAP(
//                mp_services,
//                LinuxCpuProcedure,
//                1,
//                Event,
//                1000,
//                mp_services,
//                NULL);
//        if (status == EFI_SUCCESS) {
//            Print(L"Task successfully started.\n");
//        } else {
//            Print(L"Failed to start Task on CPU %d: %r\n", 1, status);
//        }
//    } else {
//        Print(L"Event creation failed: %r\n", status);
//    }
//
//    UINTN EventIndex;
//    gBS->WaitForEvent(1, &Event, &EventIndex);
//    Print(L"EVENT: %d\n", EventIndex);

    while(1) {}

    return 0;
}

VOID DumpCpus() {
    Print(L"Number of processors: %d\n", num_processors);
    Print(L"Number of enabled processors: %d\n", num_enabled_processors);

    for (UINTN i=0; i<num_processors; i++)
    {
        EFI_PROCESSOR_INFORMATION ProcessorInfo;
        EFI_STATUS status = mp_services->GetProcessorInfo(mp_services, i, &ProcessorInfo);
        if (EFI_ERROR(status)) {
            Print(L"Processor #%d GetProcessorInfo Failed: 0x%x\n", status);
            continue;
        }

        Print(L"Processor #%d ACPI Processor ID = %lX, Flags = %x, Package = %x, Core = %x, Thread = %x \n",
               i,
               ProcessorInfo.ProcessorId,
               ProcessorInfo.StatusFlag,
               ProcessorInfo.Location.Package,
               ProcessorInfo.Location.Core,
               ProcessorInfo.Location.Thread);
    }
}
