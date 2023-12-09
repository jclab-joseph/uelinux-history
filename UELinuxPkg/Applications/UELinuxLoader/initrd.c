#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadFile2.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/LinuxEfiInitrdMedia.h>

//
// Device path for the handle that incorporates our "EFI stub filesystem".
//
#pragma pack (1)
typedef struct {
    VENDOR_DEVICE_PATH          VenMediaNode;
    EFI_DEVICE_PATH_PROTOCOL    EndNode;
} SINGLE_VENMEDIA_NODE_DEVPATH;
#pragma pack ()

STATIC CONST SINGLE_VENMEDIA_NODE_DEVPATH mInitrdDevicePath = {
        {
                {
                        MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
                        { sizeof (VENDOR_DEVICE_PATH)       }
                },
                                      LINUX_EFI_INITRD_MEDIA_GUID
        },  {
                END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
                { sizeof (EFI_DEVICE_PATH_PROTOCOL) }
        }
};


/* extend LoadFileProtocol */
struct initrd_loader {
    EFI_LOAD_FILE2_PROTOCOL load_file;
    CONST VOID* address;
    UINTN length;
};

/*
 * static structure for LINUX_INITRD_MEDIA device path
 * see https://github.com/torvalds/linux/blob/v5.13/drivers/firmware/efi/libstub/efi-stub-helper.c
 */
typedef struct _MEDIA_INITRD_DEVICE_PATH {
    VENDOR_DEVICE_PATH vendor;
    EFI_DEVICE_PATH end;
} MEDIA_INITRD_DEVICE_PATH;

static EFIAPI EFI_STATUS initrd_load_file(
        IN EFI_LOAD_FILE2_PROTOCOL           *This,
        IN EFI_DEVICE_PATH_PROTOCOL         *FilePath,
        IN BOOLEAN                          BootPolicy,
        IN OUT UINTN                        *BufferSize,
        IN VOID                             *Buffer OPTIONAL
);

EFI_STATUS RegisterInitrd(
        VOID *initrd_buffer,
        UINTN initrd_size,
        EFI_HANDLE *phandle
) {
    EFI_STATUS status;
    EFI_DEVICE_PATH *dp = (EFI_DEVICE_PATH *) &mInitrdDevicePath;
    EFI_HANDLE handle;
    struct initrd_loader *loader;

    if (!initrd_buffer || initrd_size == 0)
        return EFI_SUCCESS;

    /*
     * check if a LINUX_INITRD_MEDIA_GUID DevicePath is already registered.
     * LocateDevicePath checks for the "closest DevicePath" and returns its handle,
     * where as InstallMultipleProtocolInterfaces only matches identical DevicePaths.
     */
    status = gBS->LocateDevicePath(&gEfiLoadFile2ProtocolGuid, &dp, &handle);
    if (status != EFI_NOT_FOUND) {
        // InitrdMedia is already registered
        return EFI_ALREADY_STARTED;
    }

    status = gBS->AllocatePool(EfiLoaderData, sizeof(struct initrd_loader), (VOID**)&loader);
    if (EFI_ERROR(status)) {
        return status;
    }

    loader->load_file.LoadFile = initrd_load_file;
    loader->address = initrd_buffer;
    loader->length = initrd_size;

    /* create a new handle and register the LoadFile2 protocol with the InitrdMediaPath on it */
    status = gBS->InstallMultipleProtocolInterfaces(
            phandle, &gEfiDevicePathProtocolGuid,
            &mInitrdDevicePath, &gEfiLoadFile2ProtocolGuid,
            loader,
            NULL);
    if (status != EFI_SUCCESS) {
        gBS->FreePool((VOID*)loader);
    }

    return status;
}

static EFIAPI EFI_STATUS initrd_load_file(
        IN EFI_LOAD_FILE2_PROTOCOL           *This,
        IN EFI_DEVICE_PATH_PROTOCOL         *FilePath,
        IN BOOLEAN                          BootPolicy,
        IN OUT UINTN                        *BufferSize,
        IN VOID                             *Buffer OPTIONAL
) {
    struct initrd_loader *loader;

    Print(L"initrd_load_file: %d\n", *BufferSize);

    if (!This || !BufferSize || !FilePath) {
        Print(L"initrd_load_file: EFI_INVALID_PARAMETER\n");
        return EFI_INVALID_PARAMETER;
    }
    if (BootPolicy) {
        Print(L"initrd_load_file: EFI_UNSUPPORTED\n");
        return EFI_UNSUPPORTED;
    }

    loader = (struct initrd_loader *) This;

    if (loader->length == 0 || !loader->address) {
        Print(L"initrd_load_file: EFI_NOT_FOUND\n");
        return EFI_NOT_FOUND;
    }

    if (!Buffer || *BufferSize < loader->length) {
        Print(L"initrd_load_file: EFI_BUFFER_TOO_SMALL\n");
        *BufferSize = loader->length;
        return EFI_BUFFER_TOO_SMALL;
    }

    CopyMem(Buffer, loader->address, loader->length);
    *BufferSize = loader->length;
    Print(L"initrd_load_file: SUCCESS: %d\n", *BufferSize);
    return EFI_SUCCESS;
}
