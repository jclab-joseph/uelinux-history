#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "proxy.h"

/**
  Raise the task priority level to the new level.
  High level is implemented by disabling processor interrupts.

  @param  NewTpl  New task priority level

  @return The previous task priority level

**/
STATIC
EFI_TPL
EFIAPI
ProxyRaiseTplStub(
        IN EFI_TPL NewTpl
) {
    return (EFI_TPL) ProxyStubCall1((VOID*)gBS->RaiseTPL, (VOID*)NewTpl);
}

/**
  Lowers the task priority to the previous value.   If the new
  priority unmasks events at a higher priority, they are dispatched.

  @param  NewTpl  New, lower, task priority

**/
STATIC
VOID
EFIAPI
ProxyRestoreTplStub(
        IN EFI_TPL NewTpl
) {
    ProxyStubCall1((VOID*)gBS->RestoreTPL, (VOID*)NewTpl);
}

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address

  @return Status. On success, Memory is filled in with the base address allocated
  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in
                                 spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyAllocatePagesStub(
        IN EFI_ALLOCATE_TYPE Type,
        IN EFI_MEMORY_TYPE MemoryType,
        IN UINTN NumberOfPages,
        IN OUT EFI_PHYSICAL_ADDRESS *Memory
) {
    return (EFI_STATUS) ProxyStubCall4((VOID*)gBS->AllocatePages, (VOID*)Type, (VOID*) MemoryType, (VOID*) NumberOfPages, (VOID*) Memory);
}

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed
  @param  NumberOfPages          The number of pages to free

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range
  @retval EFI_INVALID_PARAMETER  Address not aligned
  @return EFI_SUCCESS         -Pages successfully freed.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyFreePagesStub(
        IN EFI_PHYSICAL_ADDRESS Memory,
        IN UINTN NumberOfPages
) {
    return (EFI_STATUS)ProxyStubCall2((VOID*)gBS->FreePages, (VOID*)Memory, (VOID*)NumberOfPages);
}

/**
  This function returns a copy of the current memory map. The map is an array of
  memory descriptors, each of which describes a contiguous block of memory.

  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of
                                 the buffer allocated by the caller.  On output,
                                 it is the size of the buffer returned by the
                                 firmware  if the buffer was large enough, or the
                                 size of the buffer needed  to contain the map if
                                 the buffer was too small.
  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MapKey                 A pointer to the location in which firmware
                                 returns the key for the current memory map.
  @param  DescriptorSize         A pointer to the location in which firmware
                                 returns the size, in bytes, of an individual
                                 EFI_MEMORY_DESCRIPTOR.
  @param  DescriptorVersion      A pointer to the location in which firmware
                                 returns the version number associated with the
                                 EFI_MEMORY_DESCRIPTOR.

  @retval EFI_SUCCESS            The memory map was returned in the MemoryMap
                                 buffer.
  @retval EFI_BUFFER_TOO_SMALL   The MemoryMap buffer was too small. The current
                                 buffer size needed to hold the memory map is
                                 returned in MemoryMapSize.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyGetMemoryMapStub(
        IN OUT UINTN *MemoryMapSize,
        IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
        OUT UINTN *MapKey,
        OUT UINTN *DescriptorSize,
        OUT UINT32 *DescriptorVersion
) {
    return (EFI_STATUS)ProxyStubCall5(
            (VOID*)gBS->GetMemoryMap,
            (VOID*)MemoryMapSize,
            (VOID*)MemoryMap,
            (VOID*)MapKey,
            (VOID*)DescriptorSize,
            (VOID*)DescriptorVersion
    );
}

/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool

  @retval EFI_INVALID_PARAMETER  PoolType not valid or Buffer is NULL
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyAllocatePoolStub(
        IN EFI_MEMORY_TYPE PoolType,
        IN UINTN Size,
        OUT VOID **Buffer
) {
    return (EFI_STATUS)ProxyStubCall3((VOID*)gBS->AllocatePool, (VOID*)PoolType, (VOID*)Size, (VOID*)Buffer);
}

/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyFreePoolStub(
        IN VOID *Buffer
) {
    return (EFI_STATUS)ProxyStubCall1((VOID*)gBS->FreePool, (VOID*)Buffer);
}

/**
  Creates an event.

  @param  Type                   The type of event to create and its mode and
                                 attributes
  @param  NotifyTpl              The task priority level of event notifications
  @param  NotifyFunction         Pointer to the events notification function
  @param  NotifyContext          Pointer to the notification functions context;
                                 corresponds to parameter "Context" in the
                                 notification function
  @param  Event                  Pointer to the newly created event if the call
                                 succeeds; undefined otherwise

  @retval EFI_SUCCESS            The event structure was created
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  @retval EFI_OUT_OF_RESOURCES   The event could not be allocated

**/
STATIC
EFI_STATUS
EFIAPI
ProxyCreateEventStub(
        IN UINT32 Type,
        IN EFI_TPL NotifyTpl,
        IN EFI_EVENT_NOTIFY NotifyFunction, OPTIONAL
        IN VOID *NotifyContext, OPTIONAL
        OUT EFI_EVENT *Event
) {
    return (EFI_STATUS)ProxyStubCall5(
            (VOID*)gBS->CreateEvent,
            (VOID*)Type,
            (VOID*)NotifyTpl,
            (VOID*)NotifyFunction,
            (VOID*)NotifyContext,
            (VOID*)Event
    );
}

/**
  Creates an event in a group.

  @param  Type                   The type of event to create and its mode and
                                 attributes
  @param  NotifyTpl              The task priority level of event notifications
  @param  NotifyFunction         Pointer to the events notification function
  @param  NotifyContext          Pointer to the notification functions context;
                                 corresponds to parameter "Context" in the
                                 notification function
  @param  EventGroup             GUID for EventGroup if NULL act the same as
                                 gBS->CreateEvent().
  @param  Event                  Pointer to the newly created event if the call
                                 succeeds; undefined otherwise

  @retval EFI_SUCCESS            The event structure was created
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  @retval EFI_OUT_OF_RESOURCES   The event could not be allocated

**/
STATIC
EFI_STATUS
EFIAPI
ProxyCreateEventExStub (
        IN UINT32            Type,
        IN EFI_TPL           NotifyTpl,
        IN EFI_EVENT_NOTIFY  NotifyFunction  OPTIONAL,
        IN CONST VOID        *NotifyContext  OPTIONAL,
        IN CONST EFI_GUID    *EventGroup     OPTIONAL,
        OUT EFI_EVENT        *Event
) {
    return (EFI_STATUS)ProxyStubCall6(
            (VOID*)gBS->CreateEventEx,
            (VOID*)Type,
            (VOID*)NotifyTpl,
            (VOID*)NotifyFunction,
            (VOID*)NotifyContext,
            (VOID*)EventGroup,
            (VOID*)Event
    );
}

/**
  Sets the type of timer and the trigger time for a timer event.

  @param  UserEvent              The timer event that is to be signaled at the
                                 specified time
  @param  Type                   The type of time that is specified in
                                 TriggerTime
  @param  TriggerTime            The number of 100ns units until the timer
                                 expires

  @retval EFI_SUCCESS            The event has been set to be signaled at the
                                 requested time
  @retval EFI_INVALID_PARAMETER  Event or Type is not valid

**/
STATIC
EFI_STATUS
EFIAPI
ProxySetTimerStub(
        IN EFI_EVENT UserEvent,
        IN EFI_TIMER_DELAY Type,
        IN UINT64 TriggerTime
) {
    return (EFI_STATUS)ProxyStubCall3((VOID*)gBS->SetTimer, (VOID*)UserEvent, (VOID*)Type, (VOID*)TriggerTime);
}

/**
  Stops execution until an event is signaled.

  @param  NumberOfEvents         The number of events in the UserEvents array
  @param  UserEvents             An array of EFI_EVENT
  @param  UserIndex              Pointer to the index of the event which
                                 satisfied the wait condition

  @retval EFI_SUCCESS            The event indicated by Index was signaled.
  @retval EFI_INVALID_PARAMETER  The event indicated by Index has a notification
                                 function or Event was not a valid type
  @retval EFI_UNSUPPORTED        The current TPL is not TPL_APPLICATION

**/
STATIC
EFI_STATUS
EFIAPI
ProxyWaitForEventStub(
        IN UINTN NumberOfEvents,
        IN EFI_EVENT *UserEvents,
        OUT UINTN *UserIndex
) {
    return (EFI_STATUS)ProxyStubCall3((VOID*)gBS->WaitForEvent, (VOID*)NumberOfEvents, (VOID*)UserEvents, (VOID*)UserIndex);
}

/**
  Signals the event.  Queues the event to be notified if needed.

  @param  UserEvent              The event to signal .

  @retval EFI_INVALID_PARAMETER  Parameters are not valid.
  @retval EFI_SUCCESS            The event was signaled.

**/
STATIC
EFI_STATUS
EFIAPI
ProxySignalEventStub(
        IN EFI_EVENT UserEvent
) {
    return (EFI_STATUS)ProxyStubCall1((VOID*)gBS->SignalEvent, (VOID*)UserEvent);
}

/**
  Closes an event and frees the event structure.

  @param  UserEvent              Event to close

  @retval EFI_INVALID_PARAMETER  Parameters are not valid.
  @retval EFI_SUCCESS            The event has been closed

**/
STATIC
EFI_STATUS
EFIAPI
ProxyCloseEventStub(
        IN EFI_EVENT UserEvent
) {
    return (EFI_STATUS)ProxyStubCall1((VOID*)gBS->CloseEvent, (VOID*)UserEvent);
}

/**
  Check the status of an event.

  @param  UserEvent              The event to check

  @retval EFI_SUCCESS            The event is in the signaled state
  @retval EFI_NOT_READY          The event is not in the signaled state
  @retval EFI_INVALID_PARAMETER  Event is of type EVT_NOTIFY_SIGNAL

**/
STATIC
EFI_STATUS
EFIAPI
ProxyCheckEventStub(
        IN EFI_EVENT UserEvent
) {
    return (EFI_STATUS)ProxyStubCall1((VOID*)gBS->CheckEvent, (VOID*)UserEvent);
}

/**
  Wrapper function to ProxyInstallProtocolInterfaceNotify.  This is the public API which
  Calls the private one which contains a BOOLEAN parameter for notifications

  @param  UserHandle             The handle to install the protocol handler on,
                                 or NULL if a new handle is to be allocated
  @param  Protocol               The protocol to add to the handle
  @param  InterfaceType          Indicates whether Interface is supplied in
                                 native form.
  @param  Interface              The interface for the protocol being added

  @return Status code

**/
STATIC
EFI_STATUS
EFIAPI
ProxyInstallProtocolInterfaceStub(
        IN OUT EFI_HANDLE *UserHandle,
        IN EFI_GUID *Protocol,
        IN EFI_INTERFACE_TYPE InterfaceType,
        IN VOID *Interface
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->InstallProtocolInterface,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)InterfaceType,
            (VOID*)Interface
    );
}

/**
  Reinstall a protocol interface on a device handle.  The OldInterface for Protocol is replaced by the NewInterface.

  @param  UserHandle             Handle on which the interface is to be
                                 reinstalled
  @param  Protocol               The numeric ID of the interface
  @param  OldInterface           A pointer to the old interface
  @param  NewInterface           A pointer to the new interface

  @retval EFI_SUCCESS            The protocol interface was installed
  @retval EFI_NOT_FOUND          The OldInterface on the handle was not found
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value

**/
STATIC
EFI_STATUS
EFIAPI
ProxyReinstallProtocolInterfaceStub(
        IN EFI_HANDLE UserHandle,
        IN EFI_GUID *Protocol,
        IN VOID *OldInterface,
        IN VOID *NewInterface
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->ReinstallProtocolInterface,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)OldInterface,
            (VOID*)NewInterface
    );
}

/**
  Uninstalls all instances of a protocol:interfacer from a handle.
  If the last protocol interface is remove from the handle, the
  handle is freed.

  @param  UserHandle             The handle to remove the protocol handler from
  @param  Protocol               The protocol, of protocol:interface, to remove
  @param  Interface              The interface, of protocol:interface, to remove

  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_SUCCESS            Protocol interface successfully uninstalled.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyUninstallProtocolInterfaceStub(
        IN EFI_HANDLE UserHandle,
        IN EFI_GUID *Protocol,
        IN VOID *Interface
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->UninstallProtocolInterface,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)Interface
    );
}

/**
  Queries a handle to determine if it supports a specified protocol.

  @param  UserHandle             The handle being queried.
  @param  Protocol               The published unique identifier of the protocol.
  @param  Interface              Supplies the address where a pointer to the
                                 corresponding Protocol Interface is returned.

  @return The requested protocol interface for the handle

**/
STATIC
EFI_STATUS
EFIAPI
ProxyHandleProtocolStub(
        IN EFI_HANDLE UserHandle,
        IN EFI_GUID *Protocol,
        OUT VOID **Interface
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->HandleProtocol,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)Interface
    );
}

/**
  Add a new protocol notification record for the request protocol.

  @param  Protocol               The requested protocol to add the notify
                                 registration
  @param  Event                  The event to signal
  @param  Registration           Returns the registration record

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully returned the registration record
                                 that has been added

**/
STATIC
EFI_STATUS
EFIAPI
ProxyRegisterProtocolNotifyStub(
        IN EFI_GUID *Protocol,
        IN EFI_EVENT Event,
        OUT  VOID **Registration
){
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->RegisterProtocolNotify,
            (VOID*)Protocol,
            (VOID*)Event,
            (VOID*)Registration
    );
}

/**
  Locates the requested handle(s) and returns them in Buffer.

  @param  SearchType             The type of search to perform to locate the
                                 handles
  @param  Protocol               The protocol to search for
  @param  SearchKey              Dependant on SearchType
  @param  BufferSize             On input the size of Buffer.  On output the
                                 size of data returned.
  @param  Buffer                 The buffer to return the results in

  @retval EFI_BUFFER_TOO_SMALL   Buffer too small, required buffer size is
                                 returned in BufferSize.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully found the requested handle(s) and
                                 returns them in Buffer.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyLocateHandleStub(
        IN EFI_LOCATE_SEARCH_TYPE SearchType,
        IN EFI_GUID *Protocol   OPTIONAL,
        IN VOID *SearchKey  OPTIONAL,
        IN OUT UINTN *BufferSize,
        OUT EFI_HANDLE *Buffer
) {
    return (EFI_STATUS)ProxyStubCall5(
            (VOID*)gBS->LocateHandle,
            (VOID*)SearchType,
            (VOID*)Protocol,
            (VOID*)SearchKey,
            (VOID*)BufferSize,
            (VOID*)Buffer
    );
}

/**
  Locates the handle to a device on the device path that best matches the specified protocol.

  @param  Protocol               The protocol to search for.
  @param  DevicePath             On input, a pointer to a pointer to the device
                                 path. On output, the device path pointer is
                                 modified to point to the remaining part of the
                                 devicepath.
  @param  Device                 A pointer to the returned device handle.

  @retval EFI_SUCCESS            The resulting handle was returned.
  @retval EFI_NOT_FOUND          No handles matched the search.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyLocateDevicePathStub(
        IN EFI_GUID *Protocol,
        IN OUT EFI_DEVICE_PATH_PROTOCOL **DevicePath,
        OUT EFI_HANDLE *Device
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->LocateDevicePath,
            (VOID*)Protocol,
            (VOID*)DevicePath,
            (VOID*)Device
    );
}

/**
  Boot Service called to add, modify, or remove a system configuration table from
  the EFI System Table.

  @param  Guid           Pointer to the GUID for the entry to add, update, or
                         remove
  @param  Table          Pointer to the configuration table for the entry to add,
                         update, or remove, may be NULL.

  @return EFI_SUCCESS               Guid, Table pair added, updated, or removed.
  @return EFI_INVALID_PARAMETER     Input GUID not valid.
  @return EFI_NOT_FOUND             Attempted to delete non-existant entry
  @return EFI_OUT_OF_RESOURCES      Not enough memory available

**/
STATIC
EFI_STATUS
EFIAPI
ProxyInstallConfigurationTableStub(
        IN EFI_GUID *Guid,
        IN VOID *Table
) {
    return (EFI_STATUS)ProxyStubCall2(
            (VOID*)gBS->InstallConfigurationTable,
            (VOID*)Guid,
            (VOID*)Table
    );
}

/**
  Loads an EFI image into memory and returns a handle to the image.

  @param  BootPolicy              If TRUE, indicates that the request originates
                                  from the boot manager, and that the boot
                                  manager is attempting to load FilePath as a
                                  boot selection.
  @param  ParentImageHandle       The caller's image handle.
  @param  FilePath                The specific file path from which the image is
                                  loaded.
  @param  SourceBuffer            If not NULL, a pointer to the memory location
                                  containing a copy of the image to be loaded.
  @param  SourceSize              The size in bytes of SourceBuffer.
  @param  ImageHandle             Pointer to the returned image handle that is
                                  created when the image is successfully loaded.

  @retval EFI_SUCCESS             The image was loaded into memory.
  @retval EFI_NOT_FOUND           The FilePath was not found.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED         The image type is not supported, or the device
                                  path cannot be parsed to locate the proper
                                  protocol for loading the file.
  @retval EFI_OUT_OF_RESOURCES    Image was not loaded due to insufficient
                                  resources.
  @retval EFI_LOAD_ERROR          Image was not loaded because the image format was corrupt or not
                                  understood.
  @retval EFI_DEVICE_ERROR        Image was not loaded because the device returned a read error.
  @retval EFI_ACCESS_DENIED       Image was not loaded because the platform policy prohibits the
                                  image from being loaded. NULL is returned in *ImageHandle.
  @retval EFI_SECURITY_VIOLATION  Image was loaded and an ImageHandle was created with a
                                  valid EFI_LOADED_IMAGE_PROTOCOL. However, the current
                                  platform policy specifies that the image should not be started.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyLoadImageStub(
        IN BOOLEAN BootPolicy,
        IN EFI_HANDLE ParentImageHandle,
        IN EFI_DEVICE_PATH_PROTOCOL *FilePath,
        IN VOID *SourceBuffer   OPTIONAL,
        IN UINTN SourceSize,
        OUT EFI_HANDLE *ImageHandle
) {
    return (EFI_STATUS)ProxyStubCall6(
            (VOID*)gBS->LoadImage,
            (VOID*)BootPolicy,
            (VOID*)ParentImageHandle,
            (VOID*)FilePath,
            (VOID*)SourceBuffer,
            (VOID*)SourceSize,
            (VOID*)ImageHandle
    );
}

/**
  Transfer control to a loaded image's entry point.

  @param  ImageHandle             Handle of image to be started.
  @param  ExitDataSize            Pointer of the size to ExitData
  @param  ExitData                Pointer to a pointer to a data buffer that
                                  includes a Null-terminated string,
                                  optionally followed by additional binary data.
                                  The string is a description that the caller may
                                  use to further indicate the reason for the
                                  image's exit.

  @retval EFI_INVALID_PARAMETER   Invalid parameter
  @retval EFI_OUT_OF_RESOURCES    No enough buffer to allocate
  @retval EFI_SECURITY_VIOLATION  The current platform policy specifies that the image should not be started.
  @retval EFI_SUCCESS             Successfully transfer control to the image's
                                  entry point.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyStartImageStub(
        IN EFI_HANDLE ImageHandle,
        OUT UINTN *ExitDataSize,
        OUT CHAR16 **ExitData  OPTIONAL
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->StartImage,
            (VOID*)ImageHandle,
            (VOID*)ExitDataSize,
            (VOID*)ExitData
    );
}

/**
  Terminates the currently loaded EFI image and returns control to boot services.

  @param  ImageHandle             Handle that identifies the image. This
                                  parameter is passed to the image on entry.
  @param  Status                  The image's exit code.
  @param  ExitDataSize            The size, in bytes, of ExitData. Ignored if
                                  ExitStatus is EFI_SUCCESS.
  @param  ExitData                Pointer to a data buffer that includes a
                                  Null-terminated Unicode string, optionally
                                  followed by additional binary data. The string
                                  is a description that the caller may use to
                                  further indicate the reason for the image's
                                  exit.

  @retval EFI_INVALID_PARAMETER   Image handle is NULL or it is not current
                                  image.
  @retval EFI_SUCCESS             Successfully terminates the currently loaded
                                  EFI image.
  @retval EFI_ACCESS_DENIED       Should never reach there.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate pool

**/
STATIC
EFI_STATUS
EFIAPI
ProxyExitStub(
        IN EFI_HANDLE ImageHandle,
        IN EFI_STATUS Status,
        IN UINTN ExitDataSize,
        IN CHAR16 *ExitData  OPTIONAL
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->Exit,
            (VOID*)ImageHandle,
            (VOID*)Status,
            (VOID*)ExitDataSize,
            (VOID*)ExitData
    );
}

/**
  Unloads an image.

  @param  ImageHandle             Handle that identifies the image to be
                                  unloaded.

  @retval EFI_SUCCESS             The image has been unloaded.
  @retval EFI_UNSUPPORTED         The image has been started, and does not support
                                  unload.
  @retval EFI_INVALID_PARAMPETER  ImageHandle is not a valid image handle.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyUnloadImageStub(
        IN EFI_HANDLE ImageHandle
) {
    return (EFI_STATUS)ProxyStubCall1(
            (VOID*)gBS->UnloadImage,
            (VOID*)ImageHandle
    );
}

/**
  Terminates all boot services.

  @param  ImageHandle            Handle that identifies the exiting image.
  @param  MapKey                 Key to the latest memory map.

  @retval EFI_SUCCESS            Boot Services terminated
  @retval EFI_INVALID_PARAMETER  MapKey is incorrect.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyExitBootServicesStub(
        IN EFI_HANDLE ImageHandle,
        IN UINTN MapKey
) {
    return (EFI_STATUS)ProxyStubCall2(
            (VOID*)gBS->ExitBootServices,
            (VOID*)ImageHandle,
            (VOID*)MapKey
    );
}

///**
//  Returns a monotonically increasing count for the platform.
//
//  @param[out]  Count            The pointer to returned value.
//
//  @retval EFI_SUCCESS           The next monotonic count was returned.
//  @retval EFI_INVALID_PARAMETER Count is NULL.
//  @retval EFI_DEVICE_ERROR      The device is not functioning properly.
//
//**/
//STATIC
//EFI_STATUS
//EFIAPI
//ProxyGetNextMonotonicCountStub(
//        OUT UINT64 *Count
//) {
//    return (EFI_STATUS)ProxyStubCall1(
//            (VOID*)gBS->GetNextMonotonicCount,
//            (VOID*)Count
//    );
//}

/**
  Introduces a fine-grained stall.

  @param  Microseconds           The number of microseconds to stall execution.

  @retval EFI_SUCCESS            Execution was stalled for at least the requested
                                 amount of microseconds.
  @retval EFI_NOT_AVAILABLE_YET  gMetronome is not available yet

**/
STATIC
EFI_STATUS
EFIAPI
ProxyStallStub(
        IN UINTN Microseconds
) {
    return (EFI_STATUS)ProxyStubCall1(
            (VOID*)gBS->Stall,
            (VOID*)Microseconds
    );
}

/**
  Sets the system's watchdog timer.

  @param  Timeout         The number of seconds to set the watchdog timer to.
                          A value of zero disables the timer.
  @param  WatchdogCode    The numeric code to log on a watchdog timer timeout
                          event. The firmware reserves codes 0x0000 to 0xFFFF.
                          Loaders and operating systems may use other timeout
                          codes.
  @param  DataSize        The size, in bytes, of WatchdogData.
  @param  WatchdogData    A data buffer that includes a Null-terminated Unicode
                          string, optionally followed by additional binary data.
                          The string is a description that the call may use to
                          further indicate the reason to be logged with a
                          watchdog event.

  @return EFI_SUCCESS               Timeout has been set
  @return EFI_NOT_AVAILABLE_YET     WatchdogTimer is not available yet
  @return EFI_UNSUPPORTED           System does not have a timer (currently not used)
  @return EFI_DEVICE_ERROR          Could not complete due to hardware error

**/
STATIC
EFI_STATUS
EFIAPI
ProxySetWatchdogTimerStub(
        IN UINTN Timeout,
        IN UINT64 WatchdogCode,
        IN UINTN DataSize,
        IN CHAR16 *WatchdogData OPTIONAL
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->SetWatchdogTimer,
            (VOID*)Timeout,
            (VOID*)WatchdogCode,
            (VOID*)DataSize,
            (VOID*)WatchdogData
    );
}

/**
  Connects one or more drivers to a controller.

  @param  ControllerHandle      The handle of the controller to which driver(s) are to be connected.
  @param  DriverImageHandle     A pointer to an ordered list handles that support the
                                EFI_DRIVER_BINDING_PROTOCOL.
  @param  RemainingDevicePath   A pointer to the device path that specifies a child of the
                                controller specified by ControllerHandle.
  @param  Recursive             If TRUE, then ConnectController() is called recursively
                                until the entire tree of controllers below the controller specified
                                by ControllerHandle have been created. If FALSE, then
                                the tree of controllers is only expanded one level.

  @retval EFI_SUCCESS           1) One or more drivers were connected to ControllerHandle.
                                2) No drivers were connected to ControllerHandle, but
                                RemainingDevicePath is not NULL, and it is an End Device
                                Path Node.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_NOT_FOUND         1) There are no EFI_DRIVER_BINDING_PROTOCOL instances
                                present in the system.
                                2) No drivers were connected to ControllerHandle.
  @retval EFI_SECURITY_VIOLATION
                                The user has no permission to start UEFI device drivers on the device path
                                associated with the ControllerHandle or specified by the RemainingDevicePath.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyConnectControllerStub(
        IN  EFI_HANDLE ControllerHandle,
        IN  EFI_HANDLE *DriverImageHandle    OPTIONAL,
        IN  EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath  OPTIONAL,
        IN  BOOLEAN Recursive
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->ConnectController,
            (VOID*)ControllerHandle,
            (VOID*)DriverImageHandle,
            (VOID*)RemainingDevicePath,
            (VOID*)Recursive
    );
}

/**
  Disconnects a controller from a driver

  @param  ControllerHandle                      ControllerHandle The handle of
                                                the controller from which
                                                driver(s)  are to be
                                                disconnected.
  @param  DriverImageHandle                     DriverImageHandle The driver to
                                                disconnect from ControllerHandle.
  @param  ChildHandle                           ChildHandle The handle of the
                                                child to destroy.

  @retval EFI_SUCCESS                           One or more drivers were
                                                disconnected from the controller.
  @retval EFI_SUCCESS                           On entry, no drivers are managing
                                                ControllerHandle.
  @retval EFI_SUCCESS                           DriverImageHandle is not NULL,
                                                and on entry DriverImageHandle is
                                                not managing ControllerHandle.
  @retval EFI_INVALID_PARAMETER                 ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER                 DriverImageHandle is not NULL,
                                                and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER                 ChildHandle is not NULL, and it
                                                is not a valid EFI_HANDLE.
  @retval EFI_OUT_OF_RESOURCES                  There are not enough resources
                                                available to disconnect any
                                                drivers from ControllerHandle.
  @retval EFI_DEVICE_ERROR                      The controller could not be
                                                disconnected because of a device
                                                error.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyDisconnectControllerStub(
        IN  EFI_HANDLE ControllerHandle,
        IN  EFI_HANDLE DriverImageHandle  OPTIONAL,
        IN  EFI_HANDLE ChildHandle        OPTIONAL
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->DisconnectController,
            (VOID*)ControllerHandle,
            (VOID*)DriverImageHandle,
            (VOID*)ChildHandle
    );
}

/**
  Locates the installed protocol handler for the handle, and
  invokes it to obtain the protocol interface. Usage information
  is registered in the protocol data base.

  @param  UserHandle             The handle to obtain the protocol interface on
  @param  Protocol               The ID of the protocol
  @param  Interface              The location to return the protocol interface
  @param  ImageHandle            The handle of the Image that is opening the
                                 protocol interface specified by Protocol and
                                 Interface.
  @param  ControllerHandle       The controller handle that is requiring this
                                 interface.
  @param  Attributes             The open mode of the protocol interface
                                 specified by Handle and Protocol.

  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_SUCCESS            Get the protocol interface.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyOpenProtocolStub(
        IN  EFI_HANDLE UserHandle,
        IN  EFI_GUID *Protocol,
        OUT VOID **Interface OPTIONAL,
        IN  EFI_HANDLE ImageHandle,
        IN  EFI_HANDLE ControllerHandle,
        IN  UINT32 Attributes
) {
    return (EFI_STATUS)ProxyStubCall6(
            (VOID*)gBS->OpenProtocol,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)Interface,
            (VOID*)ImageHandle,
            (VOID*)ControllerHandle,
            (VOID*)Attributes
    );
}

/**
  Closes a protocol on a handle that was opened using OpenProtocol().

  @param  UserHandle             The handle for the protocol interface that was
                                 previously opened with OpenProtocol(), and is
                                 now being closed.
  @param  Protocol               The published unique identifier of the protocol.
                                 It is the caller's responsibility to pass in a
                                 valid GUID.
  @param  AgentHandle            The handle of the agent that is closing the
                                 protocol interface.
  @param  ControllerHandle       If the agent that opened a protocol is a driver
                                 that follows the EFI Driver Model, then this
                                 parameter is the controller handle that required
                                 the protocol interface. If the agent does not
                                 follow the EFI Driver Model, then this parameter
                                 is optional and may be NULL.

  @retval EFI_SUCCESS            The protocol instance was closed.
  @retval EFI_INVALID_PARAMETER  Handle, AgentHandle or ControllerHandle is not a
                                 valid EFI_HANDLE.
  @retval EFI_NOT_FOUND          Can not find the specified protocol or
                                 AgentHandle.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyCloseProtocolStub(
        IN  EFI_HANDLE UserHandle,
        IN  EFI_GUID *Protocol,
        IN  EFI_HANDLE AgentHandle,
        IN  EFI_HANDLE ControllerHandle
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->CloseProtocol,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)AgentHandle,
            (VOID*)ControllerHandle
    );
}

/**
  Return information about Opened protocols in the system

  @param  UserHandle             The handle to close the protocol interface on
  @param  Protocol               The ID of the protocol
  @param  EntryBuffer            A pointer to a buffer of open protocol
                                 information in the form of
                                 EFI_OPEN_PROTOCOL_INFORMATION_ENTRY structures.
  @param  EntryCount             Number of EntryBuffer entries

**/
STATIC
EFI_STATUS
EFIAPI
ProxyOpenProtocolInformationStub(
        IN  EFI_HANDLE UserHandle,
        IN  EFI_GUID *Protocol,
        OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
        OUT UINTN *EntryCount
) {
    return (EFI_STATUS)ProxyStubCall4(
            (VOID*)gBS->OpenProtocolInformation,
            (VOID*)UserHandle,
            (VOID*)Protocol,
            (VOID*)EntryBuffer,
            (VOID*)EntryCount
    );
}

/**
  Retrieves the list of protocol interface GUIDs that are installed on a handle in a buffer allocated
  from pool.

  @param  UserHandle             The handle from which to retrieve the list of
                                 protocol interface GUIDs.
  @param  ProtocolBuffer         A pointer to the list of protocol interface GUID
                                 pointers that are installed on Handle.
  @param  ProtocolBufferCount    A pointer to the number of GUID pointers present
                                 in ProtocolBuffer.

  @retval EFI_SUCCESS            The list of protocol interface GUIDs installed
                                 on Handle was returned in ProtocolBuffer. The
                                 number of protocol interface GUIDs was returned
                                 in ProtocolBufferCount.
  @retval EFI_INVALID_PARAMETER  Handle is NULL.
  @retval EFI_INVALID_PARAMETER  Handle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER  ProtocolBuffer is NULL.
  @retval EFI_INVALID_PARAMETER  ProtocolBufferCount is NULL.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the
                                 results.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyProtocolsPerHandleStub(
        IN EFI_HANDLE UserHandle,
        OUT EFI_GUID ***ProtocolBuffer,
        OUT UINTN *ProtocolBufferCount
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->ProtocolsPerHandle,
            (VOID*)UserHandle,
            (VOID*)ProtocolBuffer,
            (VOID*)ProtocolBufferCount
    );
}

/**
  Function returns an array of handles that support the requested protocol
  in a buffer allocated from pool. This is a version of ProxyLocateHandleStub()
  that allocates a buffer for the caller.

  @param  SearchType             Specifies which handle(s) are to be returned.
  @param  Protocol               Provides the protocol to search by.    This
                                 parameter is only valid for SearchType
                                 ByProtocol.
  @param  SearchKey              Supplies the search key depending on the
                                 SearchType.
  @param  NumberHandles          The number of handles returned in Buffer.
  @param  Buffer                 A pointer to the buffer to return the requested
                                 array of  handles that support Protocol.

  @retval EFI_SUCCESS            The result array of handles was returned.
  @retval EFI_NOT_FOUND          No handles match the search.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the
                                 matching results.
  @retval EFI_INVALID_PARAMETER  One or more parameters are not valid.

**/
STATIC
EFI_STATUS
EFIAPI
ProxyLocateHandleBufferStub(
        IN EFI_LOCATE_SEARCH_TYPE SearchType,
        IN EFI_GUID *Protocol OPTIONAL,
        IN VOID *SearchKey OPTIONAL,
        IN OUT UINTN *NumberHandles,
        OUT EFI_HANDLE **Buffer
) {
    return (EFI_STATUS)ProxyStubCall5(
            (VOID*)gBS->LocateHandleBuffer,
            (VOID*)SearchType,
            (VOID*)Protocol,
            (VOID*)SearchKey,
            (VOID*)NumberHandles,
            (VOID*)Buffer
    );
}

/**
  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is passed in, return a Protocol Instance that was just add
  to the system. If Registration is NULL return the first Protocol Interface
  you find.

  @param  Protocol               The protocol to search for
  @param  Registration           Optional Registration Key returned from
                                 RegisterProtocolNotify()
  @param  Interface              Return the Protocol interface (instance).

  @retval EFI_SUCCESS            If a valid Interface is returned
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Protocol interface not found

**/
STATIC
EFI_STATUS
EFIAPI
ProxyLocateProtocolStub(
        IN  EFI_GUID *Protocol,
        IN  VOID *Registration OPTIONAL,
        OUT VOID **Interface
) {
    return (EFI_STATUS)ProxyStubCall3(
            (VOID*)gBS->LocateProtocol,
            (VOID*)Protocol,
            (VOID*)Registration,
            (VOID*)Interface
    );
}

EFI_BOOT_SERVICES m_boot_services_template = {
        {
                EFI_BOOT_SERVICES_SIGNATURE,                                                          // Signature
                EFI_BOOT_SERVICES_REVISION,                                                           // Revision
                sizeof(EFI_BOOT_SERVICES),                                                           // HeaderSize
                0,                                                                                    // CRC32
                0                                                                                     // Reserved
        },
        (EFI_RAISE_TPL) ProxyRaiseTplStub,                                                            // RaiseTPL
        (EFI_RESTORE_TPL) ProxyRestoreTplStub,                                                        // RestoreTPL
        (EFI_ALLOCATE_PAGES) ProxyAllocatePagesStub,                                                  // AllocatePages
        (EFI_FREE_PAGES) ProxyFreePagesStub,                                                          // FreePages
        (EFI_GET_MEMORY_MAP) ProxyGetMemoryMapStub,                                                   // GetMemoryMap
        (EFI_ALLOCATE_POOL) ProxyAllocatePoolStub,                                                    // AllocatePool
        (EFI_FREE_POOL) ProxyFreePoolStub,                                                            // FreePool
        (EFI_CREATE_EVENT) ProxyCreateEventStub,                                                      // CreateEvent
        (EFI_SET_TIMER) ProxySetTimerStub,                                                            // SetTimer
        (EFI_WAIT_FOR_EVENT) ProxyWaitForEventStub,                                                   // WaitForEvent
        (EFI_SIGNAL_EVENT) ProxySignalEventStub,                                                      // SignalEvent
        (EFI_CLOSE_EVENT) ProxyCloseEventStub,                                                        // CloseEvent
        (EFI_CHECK_EVENT) ProxyCheckEventStub,                                                        // CheckEvent
        (EFI_INSTALL_PROTOCOL_INTERFACE) ProxyInstallProtocolInterfaceStub,                           // InstallProtocolInterface
        (EFI_REINSTALL_PROTOCOL_INTERFACE) ProxyReinstallProtocolInterfaceStub,                       // ReinstallProtocolInterface
        (EFI_UNINSTALL_PROTOCOL_INTERFACE) ProxyUninstallProtocolInterfaceStub,                       // UninstallProtocolInterface
        (EFI_HANDLE_PROTOCOL) ProxyHandleProtocolStub,                                                // HandleProtocol
        (VOID *) NULL,                                                                           // Reserved
        (EFI_REGISTER_PROTOCOL_NOTIFY) ProxyRegisterProtocolNotifyStub,                               // RegisterProtocolNotify
        (EFI_LOCATE_HANDLE) ProxyLocateHandleStub,                                                    // LocateHandle
        (EFI_LOCATE_DEVICE_PATH) ProxyLocateDevicePathStub,                                           // LocateDevicePath
        (EFI_INSTALL_CONFIGURATION_TABLE) ProxyInstallConfigurationTableStub,                         // InstallConfigurationTable
        (EFI_IMAGE_LOAD) ProxyLoadImageStub,                                                          // LoadImage
        (EFI_IMAGE_START) ProxyStartImageStub,                                                        // StartImage
        (EFI_EXIT) ProxyExitStub,                                                                     // Exit
        (EFI_IMAGE_UNLOAD) ProxyUnloadImageStub,                                                      // UnloadImage
        (EFI_EXIT_BOOT_SERVICES) ProxyExitBootServicesStub,                                           // ExitBootServices
        (EFI_GET_NEXT_MONOTONIC_COUNT) NULL,                               // GetNextMonotonicCount
        (EFI_STALL) ProxyStallStub,                                                                   // Stall
        (EFI_SET_WATCHDOG_TIMER) ProxySetWatchdogTimerStub,                                           // SetWatchdogTimer
        (EFI_CONNECT_CONTROLLER) ProxyConnectControllerStub,                                          // ConnectController
        (EFI_DISCONNECT_CONTROLLER) ProxyDisconnectControllerStub,                                    // DisconnectController
        (EFI_OPEN_PROTOCOL) ProxyOpenProtocolStub,                                                    // OpenProtocol
        (EFI_CLOSE_PROTOCOL) ProxyCloseProtocolStub,                                                  // CloseProtocol
        (EFI_OPEN_PROTOCOL_INFORMATION) ProxyOpenProtocolInformationStub,                             // OpenProtocolInformation
        (EFI_PROTOCOLS_PER_HANDLE) ProxyProtocolsPerHandleStub,                                       // ProtocolsPerHandle
        (EFI_LOCATE_HANDLE_BUFFER) ProxyLocateHandleBufferStub,                                       // LocateHandleBuffer
        (EFI_LOCATE_PROTOCOL) ProxyLocateProtocolStub,                                                // LocateProtocol
        (EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES) NULL,        // InstallMultipleProtocolInterfaces
        (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES) NULL,    // UninstallMultipleProtocolInterfaces
        (EFI_CALCULATE_CRC32) NULL,                                        // CalculateCrc32
        (EFI_COPY_MEM) CopyMem,                                                                  // CopyMem
        (EFI_SET_MEM) SetMem,                                                                    // SetMem
        (EFI_CREATE_EVENT_EX) ProxyCreateEventExStub,                                                  // CreateEventEx
};
