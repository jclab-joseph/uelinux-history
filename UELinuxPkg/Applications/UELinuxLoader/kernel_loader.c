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
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>

#define LOADED_KERNEL_CONTEXT_SIGNATURE SIGNATURE_32('u','e','l','k')

typedef struct {
    UINTN Signature;
    VOID *Source;
    UINTN SourceSize;

    EFI_HANDLE loaded_image_handle;
    EFI_IMAGE_ENTRY_POINT entrypoint;
} LOADED_KERNEL_CONTEXT;

extern EFI_LOADED_IMAGE_PROTOCOL *loader_li;
extern EFI_MP_SERVICES_PROTOCOL *mp_services;
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *current_fs;

static
EFI_STATUS
EFIAPI
HookedExitBootServices(
        IN  EFI_HANDLE ImageHandle,
        IN  UINTN MapKey
);

VOID EFIAPI LinuxProcessorEntry(VOID *parameter) {
    LOADED_KERNEL_CONTEXT *ctx = (LOADED_KERNEL_CONTEXT *) parameter;
    EFI_STATUS status;
    EFI_EXIT_BOOT_SERVICES orig_exit_boot_services;

    DEBUG((DEBUG_INFO, "***** LinuxProcessorEntry *****\n"));

    orig_exit_boot_services = gBS->ExitBootServices;
    gBS->ExitBootServices = HookedExitBootServices;
    status = ctx->entrypoint(ctx->loaded_image_handle, gST);
    gBS->ExitBootServices = orig_exit_boot_services;
    if (EFI_ERROR(status)) {
        DEBUG((DEBUG_INFO, "***** Entry Point Failed ***** 0x%p\n", status));
        Print(L"LoadImage failed: 0x%p\n", status);
        return;
    }
}

static EFI_STATUS
EFIAPI
LoadedImageContextReadImageFile(
        IN     VOID *UserHandle,
        IN     UINTN Offset,
        IN OUT UINTN *ReadSize,
        OUT    VOID *Buffer
) {
    UINTN EndPosition;
    LOADED_KERNEL_CONTEXT *ctx;

    if ((UserHandle == NULL) || (ReadSize == NULL) || (Buffer == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if (MAX_ADDRESS - Offset < *ReadSize) {
        return EFI_INVALID_PARAMETER;
    }

    ctx = (LOADED_KERNEL_CONTEXT *) UserHandle;
    ASSERT (ctx->Signature == LOADED_KERNEL_CONTEXT_SIGNATURE);

    //
    // Move data from our local copy of the file
    //
    EndPosition = Offset + *ReadSize;
    if (EndPosition > ctx->SourceSize) {
        *ReadSize = (UINT32) (ctx->SourceSize - Offset);
    }

    if (Offset >= ctx->SourceSize) {
        *ReadSize = 0;
    }

    CopyMem(Buffer, (CHAR8 *) ctx->Source + Offset, *ReadSize);
    return EFI_SUCCESS;
}

static EFI_STATUS
EFIAPI
HookedExitBootServices(
        IN  EFI_HANDLE ImageHandle,
        IN  UINTN MapKey
) {
    DEBUG((DEBUG_INFO, "***** HookedExitBootServices CALLED MapKey=%d", MapKey));
    Print(L"***** HookedExitBootServices CALLED MapKey=%d", MapKey);
    return EFI_SUCCESS;
}

EFI_STATUS StartImage() {
    EFI_STATUS status;
    const CHAR16 *kernel_file = L"KERNEL.EFI";
    CHAR16 *kernel_path;
    EFI_FILE_PROTOCOL *root;
    EFI_FILE_PROTOCOL *file;
    EFI_FILE_INFO *file_info = NULL;
    UINTN buffer_size;
    EFI_DEVICE_PATH *file_device_path;
    unsigned char *file_buffer;
    UINTN file_size;
    EFI_HANDLE image_handle;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_EVENT processor_event;

    CHAR16 *self_path = ConvertDevicePathToText(loader_li->FilePath, FALSE, FALSE);
    UINTN path_len = StrLen(self_path);

    while (path_len > 0 && self_path[path_len - 1] != L'\\') {
        path_len--;
        self_path[path_len] = 0;
    }

    status = gBS->AllocatePool(EfiBootServicesData, (path_len + StrLen(kernel_file) + 1) * sizeof(CHAR16),
                               (VOID **) &kernel_path);
    if (EFI_ERROR(status)) {
        Print(L"AllocatePool failed: 0x%x\n", status);
        return status;
    }

    StrCpyS(kernel_path, path_len + 1, self_path);
    StrCpyS(kernel_path + path_len, StrLen(kernel_file) + 1, kernel_file);

    Print(L"kernel_path : %s\n", kernel_path);

    file_device_path = FileDevicePath(loader_li->DeviceHandle, kernel_path);
    if (!file_device_path) {
        Print(L"FileDevicePath failed: 0x%x\n", status);
        return 1;
    }

    status = current_fs->OpenVolume(current_fs, &root);
    if (EFI_ERROR(status)) {
        Print(L"OpenVolume failed: 0x%x\n", status);
        return status;
    }

    status = root->Open(root, &file, kernel_path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"OpenFile failed: 0x%x\n", status);
        return status;
    }

    buffer_size = sizeof(file_info);
    status = gBS->AllocatePool(EfiBootServicesCode, buffer_size, (VOID **) &file_info);
    if (EFI_ERROR(status)) {
        Print(L"AllocatePool failed: 0x%x\n", status);
        return status;
    }

    status = file->GetInfo(file, &gEfiFileInfoGuid, &buffer_size, file_info);
    if (status == EFI_BUFFER_TOO_SMALL) {
        gBS->FreePool(file_info);
        status = gBS->AllocatePool(EfiBootServicesCode, buffer_size, (VOID **) &file_info);
        if (EFI_ERROR(status)) {
            Print(L"AllocatePool failed: 0x%x\n", status);
            return status;
        }
        status = file->GetInfo(file, &gEfiFileInfoGuid, &buffer_size, file_info);
    }
    if (EFI_ERROR(status)) {
        Print(L"GetInfo failed: 0x%x\n", status);
        return status;
    }

    file_size = file_info->FileSize;
    status = gBS->AllocatePool(EfiBootServicesCode, file_size, (VOID **) &file_buffer);
    if (EFI_ERROR(status)) {
        Print(L"AllocatePool failed: 0x%x\n", status);
        return status;
    }

    buffer_size = file_size;
    status = file->Read(file, &buffer_size, (VOID *) file_buffer);
    if (EFI_ERROR(status)) {
        Print(L"FileRead failed: 0x%x\n", status);
        return status;
    }
    Print(L"KERNEL READ : %u bytes\n", buffer_size);

    file->Close(file);

    CHAR16 *temp = ConvertDevicePathToText(file_device_path, TRUE, FALSE);
    Print(L"IMAGE PATH: %s\n", temp);

    status = gBS->LoadImage(FALSE, gImageHandle, file_device_path, file_buffer, file_size, &image_handle);
    if (EFI_ERROR(status)) {
        Print(L"LoadImage failed: 0x%x\n", status);
        return status;
    }

    status = gBS->HandleProtocol(image_handle, &gEfiLoadedImageProtocolGuid, (VOID **) &loaded_image);
    if (EFI_ERROR(status)) {
        Print(L"HandleProtocol(gEfiLoadedImageProtocolGuid) failed: 0x%x\n", status);
        return status;
    }

    Print(L"loaded_image->DeviceHandle: %p\n", loaded_image->DeviceHandle);
    Print(L"loaded_image->FilePath: %p\n", loaded_image->FilePath);


    LOADED_KERNEL_CONTEXT ctx;
    PE_COFF_LOADER_IMAGE_CONTEXT pe_loader_context;

    ZeroMem(&ctx, sizeof(ctx));
    ctx.Signature = LOADED_KERNEL_CONTEXT_SIGNATURE;
    ctx.Source = (VOID *) file_buffer;
    ctx.SourceSize = file_size;

    ZeroMem(&pe_loader_context, sizeof(pe_loader_context));
    pe_loader_context.Handle = (VOID *) &ctx;
    pe_loader_context.ImageRead = (PE_COFF_LOADER_READ_FILE) LoadedImageContextReadImageFile;
    status = PeCoffLoaderGetImageInfo(&pe_loader_context);
    Print(L"PeCoffLoaderGetImageInfod: 0x%x\n", status);
    Print(L"pe_loader_context.ImageType: %d\n", pe_loader_context.ImageType);

    switch (pe_loader_context.ImageType) {
        case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
            pe_loader_context.ImageCodeMemoryType = EfiLoaderCode;
            pe_loader_context.ImageDataMemoryType = EfiLoaderData;
            break;
        case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
            pe_loader_context.ImageCodeMemoryType = EfiBootServicesCode;
            pe_loader_context.ImageDataMemoryType = EfiBootServicesData;
            break;
        case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
        case EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER:
            pe_loader_context.ImageCodeMemoryType = EfiRuntimeServicesCode;
            pe_loader_context.ImageDataMemoryType = EfiRuntimeServicesData;
            break;
        default:
            pe_loader_context.ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
            return EFI_UNSUPPORTED;
    }

    UINTN pe_image_size;
    if (pe_loader_context.SectionAlignment > EFI_PAGE_SIZE) {
        pe_image_size = (UINTN) pe_loader_context.ImageSize + pe_loader_context.SectionAlignment;
    } else {
        pe_image_size = (UINTN) pe_loader_context.ImageSize;
    }
    UINTN pe_image_pages = EFI_SIZE_TO_PAGES (pe_image_size);

    Print(L"pe_loader_context.ImageAddress: 0x%p\n", pe_loader_context.ImageAddress);

    status = EFI_OUT_OF_RESOURCES;
    if ((pe_loader_context.ImageAddress >= 0x100000) || pe_loader_context.RelocationsStripped) {
        status = gBS->AllocatePages(
                AllocateAddress,
                (EFI_MEMORY_TYPE) (pe_loader_context.ImageCodeMemoryType),
                pe_image_pages,
                &pe_loader_context.ImageAddress
        );
    }
    if (EFI_ERROR (status) && !pe_loader_context.RelocationsStripped) {
        status = gBS->AllocatePages(
                AllocateAnyPages,
                (EFI_MEMORY_TYPE) (pe_loader_context.ImageCodeMemoryType),
                pe_image_pages,
                &pe_loader_context.ImageAddress
        );
    }
    if (EFI_ERROR(status)) {
        Print(L"Kernel AllocatePages failed: 0x%x\n", status);
        return status;
    }

    Print(L"pe_loader_context.ImageAddress: 0x%p\n", pe_loader_context.ImageAddress);

    // UINTN ImageBasePage = pe_loader_context.ImageAddress;
    if (!pe_loader_context.IsTeImage) {
        pe_loader_context.ImageAddress = (pe_loader_context.ImageAddress + pe_loader_context.SectionAlignment - 1) &
                                         ~((UINTN) pe_loader_context.SectionAlignment - 1);
    }

    Print(L"pe_loader_context.ImageAddress: 0x%p\n", pe_loader_context.ImageAddress);

    status = PeCoffLoaderLoadImage(&pe_loader_context);
    if (EFI_ERROR(status)) {
        Print(L"PeCoffLoaderLoadImage failed: 0x%x\n", status);
        return status;
    }

    status = PeCoffLoaderRelocateImage(&pe_loader_context);
    if (EFI_ERROR(status)) {
        Print(L"PeCoffLoaderRelocateImage failed: 0x%x\n", status);
        return status;
    }

    //
    // Flush the Instruction Cache
    //
    InvalidateInstructionCacheRange((VOID *) (UINTN) pe_loader_context.ImageAddress,
                                    (UINTN) pe_loader_context.ImageSize);

    DEBUG ((DEBUG_INFO | DEBUG_LOAD,
            "Loading driver at 0x%11p EntryPoint=0x%11p ",
            (VOID * )(UINTN)pe_loader_context.ImageAddress,
            FUNCTION_ENTRY_POINT(pe_loader_context.EntryPoint)
           ));

    EFI_PHYSICAL_ADDRESS linux_addr = 256 << 20; // 256MB
    const UINTN linux_size = 128 << 20; // 128MB

    for (int i = 0; i < 64; i++) {
        EFI_PHYSICAL_ADDRESS addr = linux_addr;
        status = gBS->AllocatePages(
                AllocateAddress,
                EfiLoaderData,
                EFI_SIZE_TO_PAGES(linux_size),
                &addr
        );
        if (EFI_ERROR(status)) {
            Print(L"TryAllocate(%d) Failed: 0x%p\n", linux_size, status);
        } else {
            Print(L"TryAllocate(%d) Success: addr=0x%p\n", linux_size, addr);
            linux_addr = addr;
            break;
        }
        linux_addr += 256 << 20; // 256MB
    }

    CHAR16 *cmdline = CatSPrint(
            NULL,
            L"nosmp earlyprintk=serial,ttyS1,115200 console=tty0 console=ttyS1,115200 acpi=off memmap=exactmap,128K@0M,%luM@%luM",
            linux_size >> 20,
            linux_addr >> 20
    );

    loaded_image->LoadOptions = cmdline;
    loaded_image->LoadOptionsSize = StrLen(loaded_image->LoadOptions) * sizeof(CHAR16);

    Print(L"pe_loader_context.EntryPoint : 0x%p\n", pe_loader_context.EntryPoint);

    ctx.entrypoint = (EFI_IMAGE_ENTRY_POINT) (UINTN) pe_loader_context.EntryPoint;
    ctx.loaded_image_handle = image_handle;

    status = gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &processor_event);
    if (EFI_ERROR(status)) {
        Print(L"CreateEvent failed: 0x%x\n", status);
        return status;
    }

    DEBUG((DEBUG_INFO, "StartupThisAP before\n"));
    status = mp_services->StartupThisAP(
            mp_services,
            LinuxProcessorEntry,
            1,
            processor_event,
            0,
            &ctx,
            NULL
    );
    if (EFI_ERROR(status)) {
        Print(L"StartupThisAP failed: 0x%x\n", status);
        return status;
    }
    while (1) {
        CpuPause();
    }

    UINTN index;
    status = gBS->WaitForEvent(1, &processor_event, &index);
    Print(L"WaitForEvent: 0x%x, index=%d\n", status, index);

    return 0;
}
