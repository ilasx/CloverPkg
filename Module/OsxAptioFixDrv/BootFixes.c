/**

  Methods for setting callback jump from kernel entry point, callback, fixes to kernel boot image.

  by dmazar

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/Common/DeviceTreeLib.h>

#include "BootFixes.h"
#include "AsmFuncs.h"
#include "VMem.h"
#include "Lib.h"

// buffer and size for original kernel entry code
UINT8                   gOrigKernelCode[32];
UINTN                   gOrigKernelCodeSize = 0;

// buffer for virtual address map - only for RT areas
// note: DescriptorSize is usually > sizeof (EFI_MEMORY_DESCRIPTOR)
// so this buffer can hold less then 64 descriptors
EFI_MEMORY_DESCRIPTOR   gVirtualMemoryMap[64];
UINTN                   gVirtualMapSize = 0;
UINTN                   gVirtualMapDescriptorSize = 0;

EFI_PHYSICAL_ADDRESS    gSysTableRtArea;
EFI_PHYSICAL_ADDRESS    gRelocatedSysTableRtArea;

#if 0
STATIC
VOID
PrintSample2 (
  UINT8   *sample,
  INT32   size
) {
  INT32 i;

  for (i = 0; i < size; i++) {
    //DBG (" %02x", *sample);
    sample++;
  }
}
#endif

/** Adds Offset bytes to SourcePtr and returns new pointer as ReturnType. */
#define PTR_OFFSET(SourcePtr, Offset, ReturnType) ((ReturnType)(((UINT8 *)SourcePtr) + Offset))

/** Returns Mach-O entry point from LC_UNIXTHREAD loader command. */
UINTN
EFIAPI
MachOGetEntryAddress (
  IN VOID   *MachOImage
) {
  struct  mach_header_64        *MHdr64;
          BOOLEAN               Is64Bit;
          UINT32                NCmds;
  struct  load_command          *LCmd;
          UINTN                 Index;
          i386_thread_state_t   *ThreadState;
          x86_thread_state64_t  *ThreadState64;
          UINTN                 Address;

  Address = 0;
  MHdr64 = (struct mach_header_64 *)MachOImage;
  DBG ("MachOImage: %p, magic: %x", MachOImage, MHdr64->magic);

  if ((MHdr64->magic == MH_MAGIC_64) || (MHdr64->magic == MH_CIGAM_64)) {
    // 64 bit header
    DBG (" -> 64 bit\n");
    Is64Bit = TRUE;
    NCmds = MHdr64->ncmds;
    LCmd = PTR_OFFSET (MachOImage, sizeof (struct mach_header_64), struct load_command *);
  } else {
    // invalid MachOImage
    return Address;
  }

  DBG ("ncmds: %d\n", NCmds, LCmd);
  //gBS->Stall (10 * 1000000);

  // iterate over load commands
  for (Index = 0; Index < NCmds; Index++) {

    DBG ("%d. LCmd: %p, cmd: %x, size: %d\n", Index, LCmd, LCmd->cmd, LCmd->cmdsize);

    if (LCmd->cmd == LC_UNIXTHREAD) {

      DBG ("LC_UNIXTHREAD\n");
      //
      // extract thread state
      // LCmd =
      //  struct load_command {
      //   uint32_t cmd
      //   uint32_t cmdsize
      //  }
      //  uint32_t flavor      flavor of thread state */
      //  uint32_t count       count of longs in thread state */
      //  struct XXX_thread_state state   thread state for this flavor */
      //
      ThreadState = PTR_OFFSET (LCmd, sizeof (struct load_command) + 2 * sizeof (UINT32), i386_thread_state_t *);
      ThreadState64 = (x86_thread_state64_t *)ThreadState;
      Address = (UINTN)(Is64Bit ? ThreadState64->rip : ThreadState->eip);
      break;
    }

    // next command
    LCmd = PTR_OFFSET (LCmd, LCmd->cmdsize, struct load_command *);
  }

  DBG ("Address: %lx\n", Address);
  //gBS->Stall (20 * 1000000);
  return Address;
}

//
// Kernel entry patching
//

/** Saves current 64 bit state and copies MyAsmCopyAndJumpToKernel32 function to higher mem
  * (for copying kernel back to proper place and jumping back to it).
  */
EFI_STATUS
PrepareJumpFromKernel () {
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    HigherMem;
  UINTN                   Size;
  EFI_SYSTEM_TABLE        *Src, *Dest;

  //
  // chek if already prepared
  //
  if (MyAsmCopyAndJumpToKernel32Addr != 0) {
    DBG ("PrepareJumpFromKernel () - already prepared\n");
    return EFI_SUCCESS;
  }

  //
  // save current 64bit state - will be restored later in callback from kernel jump
  //
  MyAsmPrepareJumpFromKernel ();

  //
  // allocate higher memory for MyAsmCopyAndJumpToKernel code
  //
  HigherMem = 0x100000000;
  Status = AllocatePagesFromTop (EfiBootServicesCode, 1, &HigherMem);
  if (Status != EFI_SUCCESS) {
    Print (L"%a: can not allocate mem for MyAsmCopyAndJumpToKernel (0x%x pages on mem top): %r\n",
        __FUNCTION__, 1, Status);

    return Status;
  }

  //
  // and relocate it to higher mem
  //
  MyAsmCopyAndJumpToKernel32Addr = HigherMem + ((UINT8 *)(UINTN)&MyAsmCopyAndJumpToKernel32 - (UINT8 *)(UINTN)&MyAsmCopyAndJumpToKernel);
  MyAsmCopyAndJumpToKernel64Addr = HigherMem + ((UINT8 *)(UINTN)&MyAsmCopyAndJumpToKernel64 - (UINT8 *)(UINTN)&MyAsmCopyAndJumpToKernel);

  Size = (UINT8 *)&MyAsmCopyAndJumpToKernelEnd - (UINT8 *)&MyAsmCopyAndJumpToKernel;
  if (Size > EFI_PAGES_TO_SIZE (1)) {
    Print (L"Size of MyAsmCopyAndJumpToKernel32 code is too big\n");
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem ((VOID *)(UINTN)HigherMem, (VOID *)&MyAsmCopyAndJumpToKernel, Size);

  // Allocate 1 RT data page for copy of EFI system table for kernel
  gSysTableRtArea = 0x100000000;
  Status = AllocatePagesFromTop (EfiRuntimeServicesData, 1, &gSysTableRtArea);
  if (Status != EFI_SUCCESS) {
    Print (L"%a: can not allocate mem for RT area for EFI system table: %r\n",
        __FUNCTION__, Status);
    return Status;
  }
  //DBG ("gSysTableRtArea = %lx\n", gSysTableRtArea);

  // Copy sys table to our location
  Src = (EFI_SYSTEM_TABLE *)(UINTN)gST;
  Dest = (EFI_SYSTEM_TABLE *)(UINTN)gSysTableRtArea;
  //DBG ("-Copy %p <- %p, size=0x%lx\n", Dest, Src, Src->Hdr.HeaderSize);
  CopyMem (Dest, Src, Src->Hdr.HeaderSize);

  return Status;
}

/** Patches kernel entry point with jump to MyAsmJumpFromKernel (AsmFuncsX64). This will then call KernelEntryPatchJumpBack. */
EFI_STATUS
KernelEntryPatchJump (
  UINT32    KernelEntry
) {
  EFI_STATUS    Status = EFI_SUCCESS;

  //DBG ("KernelEntryPatchJump KernelEntry (reloc): %lx (%lx)\n", KernelEntry, KernelEntry + gRelocBase);

  // Size of MyAsmEntryPatchCode code
  gOrigKernelCodeSize = (UINT8 *)&MyAsmEntryPatchCodeEnd - (UINT8 *)&MyAsmEntryPatchCode;
  if (gOrigKernelCodeSize > sizeof (gOrigKernelCode)) {
    Print (L"%a: not enough space for orig. kernel entry code: size needed: %d\n", __FUNCTION__, gOrigKernelCodeSize);
    return EFI_NOT_FOUND;
  }

  //DBG ("MyAsmEntryPatchCode: %p, Size: %d, MyAsmJumpFromKernel: %p\n", &MyAsmEntryPatchCode, gOrigKernelCodeSize, &MyAsmJumpFromKernel);

  // Save original kernel entry code
  CopyMem ((VOID *)gOrigKernelCode, (VOID *)(UINTN)KernelEntry, gOrigKernelCodeSize);

  // Copy MyAsmEntryPatchCode code to kernel entry address
  CopyMem ((VOID *)(UINTN)KernelEntry, (VOID *)&MyAsmEntryPatchCode, gOrigKernelCodeSize);

  //DBG ("Entry point %x is now: ", KernelEntry);
  //PrintSample2 ((UINT8 *)(UINTN) KernelEntry, 12);
  //DBG ("\n");

  // pass KernelEntry to assembler funcs
  // this is not needed really, since asm code will determine
  // kernel entry address from the stack
  AsmKernelEntry = KernelEntry;

  return Status;
}

#if APTIOFIX_VER == 1
/** Reads kernel entry from Mach-O load command and patches it with jump to MyAsmJumpFromKernel. */
EFI_STATUS
KernelEntryFromMachOPatchJump (
  VOID    *MachOImage,
  UINTN   SlideAddr
) {
  UINTN         KernelEntry;

  //DBG ("KernelEntryFromMachOPatchJump: MachOImage = %p, SlideAddr = %x\n", MachOImage, SlideAddr);

  KernelEntry = MachOGetEntryAddress (MachOImage);
  //DBG ("KernelEntryFromMachOPatchJump: KernelEntry = %x\n", KernelEntry);

  if (KernelEntry == 0) {
    return EFI_NOT_FOUND;
  }

  if (SlideAddr > 0) {
    KernelEntry += SlideAddr;
    //DBG ("KernelEntryFromMachOPatchJump: Slided KernelEntry = %x\n", KernelEntry);
  }

  return KernelEntryPatchJump ((UINT32)KernelEntry);
}
#endif

#if 0
/** Patches kernel entry point with HLT - used for testing to cause system halt. */
EFI_STATUS
KernelEntryPatchHalt (
  UINT32    KernelEntry
) {
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT8       *p;

  //DBG ("KernelEntryPatchHalt KernelEntry (reloc): %lx (%lx)", KernelEntry, KernelEntry + gRelocBase);
  p = (UINT8 *)(UINTN) KernelEntry;
  *p= 0xf4; // HLT instruction
  PrintSample2 (p, 4);
  //DBG ("\n");

  return Status;
}

/** Patches kernel entry point with zeros - used for testing to cause restart. */
EFI_STATUS
KernelEntryPatchZero (
  UINT32    KernelEntry
) {
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT8       *p;

  Status = EFI_SUCCESS;

  //DBG ("KernelEntryPatchZero KernelEntry (reloc): %lx (%lx)", KernelEntry, KernelEntry + gRelocBase);
  p = (UINT8 *)(UINTN) KernelEntry;
  //*p= 0xf4;
  p[0]= 0; p[1]= 0; p[2]= 0; p[3]= 0; // invalid instruction
  PrintSample2 (p, 4);
  //DBG ("\n");

  return Status;
}
#endif

//
// Boot fixes
//

/** Copies RT flagged areas to separate memmap, defines virtual to phisycal address mapping
 * and calls SetVirtualAddressMap () only with that partial memmap.
 *
 * About partial memmap:
 * Some UEFIs are converting pointers to virtual addresses even if they do not
 * point to regions with RT flag. This means that those UEFIs are using
 * Desc->VirtualStart even for non-RT regions. Linux had issues with this:
 * http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=7cb00b72876ea2451eb79d468da0e8fb9134aa8a
 * They are doing it Windows way now - copying RT descriptors to separate
 * mem map and passing that stripped map to SetVirtualAddressMap ().
 * We'll do the same, although it seems that just assigning
 * VirtualStart = PhysicalStart for non-RT areas also does the job.
 *
 * About virtual to phisycal mappings:
 * Also adds virtual to phisycal address mappings for RT areas. This is needed since
 * SetVirtualAddressMap () does not work on my Aptio without that. Probably because some driver
 * has a bug and is trying to access new virtual addresses during the change.
 * Linux and Windows are doing the same thing and problem is
 * not visible there.
 */
EFI_STATUS
ExecSetVirtualAddressesToMemMap (
  IN UINTN                    MemoryMapSize,
  IN UINTN                    DescriptorSize,
  IN UINT32                   DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR    *MemoryMap
) {
  UINTN                           NumEntries, Index, Flags, BlockSize;
  EFI_MEMORY_DESCRIPTOR           *Desc, *VirtualDesc;
  EFI_STATUS                      Status;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable;

  Desc = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;
  VirtualDesc = gVirtualMemoryMap;
  gVirtualMapSize = 0;
  gVirtualMapDescriptorSize = DescriptorSize;
  //DBG ("ExecSetVirtualAddressesToMemMap: Size=%d, Addr=%p, DescSize=%d\n", MemoryMapSize, MemoryMap, DescriptorSize);

  // get current VM page table
  GetCurrentPageTable (&PageTable, &Flags);

  for (Index = 0; Index < NumEntries; Index++) {
    if ((Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      // check if there is enough space in gVirtualMemoryMap
      if (gVirtualMapSize + DescriptorSize > sizeof (gVirtualMemoryMap)) {
        return EFI_OUT_OF_RESOURCES;
      }

      // copy region with EFI_MEMORY_RUNTIME flag to gVirtualMemoryMap
      CopyMem ((VOID *)VirtualDesc, (VOID *)Desc, DescriptorSize);

      // define virtual to phisical mapping
      //DBG ("Map pages: %lx (%x) -> %lx\n", Desc->VirtualStart, Desc->NumberOfPages, Desc->PhysicalStart);
      VmMapVirtualPages (PageTable, Desc->VirtualStart, Desc->NumberOfPages, Desc->PhysicalStart);

      // next gVirtualMemoryMap slot
      VirtualDesc = NEXT_MEMORY_DESCRIPTOR (VirtualDesc, DescriptorSize);
      gVirtualMapSize += DescriptorSize;

      // Remember future physical address for our special relocated
      // efi system table
      BlockSize = EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages);
      if ((Desc->PhysicalStart <= gSysTableRtArea) &&  (gSysTableRtArea < (Desc->PhysicalStart + BlockSize))) {
        // block contains our future sys table - remember new address
        // future physical = VirtualStart & 0x7FFFFFFFFF
        gRelocatedSysTableRtArea = (Desc->VirtualStart & 0x7FFFFFFFFF) + (gSysTableRtArea - Desc->PhysicalStart);
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  VmFlashCaches ();

  //gVirtualMapSize, MemoryMap, DescriptorSize);
  Status = gRT->SetVirtualAddressMap (gVirtualMapSize, DescriptorSize, DescriptorVersion, gVirtualMemoryMap);

  return Status;
}

VOID
CopyEfiSysTableToSeparateRtDataArea (
  IN OUT UINT32   *EfiSystemTable
) {
  EFI_SYSTEM_TABLE    *Src  = (EFI_SYSTEM_TABLE *)(UINTN)*EfiSystemTable,
                      *Dest = (EFI_SYSTEM_TABLE *)(UINTN)gSysTableRtArea;

  //DBG ("-Copy %p <- %p, size=0x%lx\n", Dest, Src, Src->Hdr.HeaderSize);
  CopyMem (Dest, Src, Src->Hdr.HeaderSize);

  *EfiSystemTable = (UINT32)(UINTN)Dest;
}

/** Protect RT data from relocation by marking them MemMapIO. Except area with EFI system table.
 *  This one must be relocated into kernel boot image or kernel will crash (kernel accesses it
 *  before RT areas are mapped into vm).
 *  This fixes NVRAM issues on some boards where access to nvram after boot services is possible
 *  only in SMM mode. RT driver passes data to SM handler through previously negotiated buffer
 *  and this buffer must not be relocated.
 *  Explained and examined in detail by CodeRush and night199uk:
 *  http://www.projectosx.com/forum/index.php?showtopic=3298
 *
 *  It seems this does not do any harm to others where this is not needed,
 *  so it's added as standard fix for all.
 */
VOID
ProtectRtDataFromRelocation (
  IN UINTN                    MemoryMapSize,
  IN UINTN                    DescriptorSize,
  IN UINT32                   DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR    *MemoryMap
) {
  UINTN                   NumEntries, Index; //  BlockSize;
  EFI_MEMORY_DESCRIPTOR   *Desc;

  Desc = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;
  //DBG ("FixNvramRelocation\n");

  for (Index = 0; Index < NumEntries; Index++) {
    //BlockSize = EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages);

    if ((Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      if ((Desc->Type == EfiRuntimeServicesData) && (Desc->PhysicalStart != gSysTableRtArea)) {
        //DBG (" RT data %lx (0x%x) -> MemMapIO\n", Desc->PhysicalStart, Desc->NumberOfPages);
        Desc->Type = EfiMemoryMappedIO;
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }
}

/** Assignes OSX virtual addresses to runtime areas in memory map. */
VOID
AssignVirtualAddressesToMemMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN EFI_PHYSICAL_ADDRESS   KernelRTAddress
) {
  UINTN                   NumEntries, Index, BlockSize;
  EFI_MEMORY_DESCRIPTOR   *Desc;

  Desc = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;
  //DBG ("AssignVirtualAddressesToMemMap: Size=%d, Addr=%p, DescSize=%d\n", MemoryMapSize, MemoryMap, DescriptorSize);

  for (Index = 0; Index < NumEntries; Index++) {
    BlockSize = EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages);

    // assign virtual addresses to all EFI_MEMORY_RUNTIME marked pages (including MMIO)
    if ((Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      //BlockSize = EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages);
      if ((Desc->Type == EfiRuntimeServicesCode) || (Desc->Type == EfiRuntimeServicesData)) {
        // for RT block - assign from kernel block
        Desc->VirtualStart = KernelRTAddress + 0xffffff8000000000;
        // next kernel block
        KernelRTAddress += BlockSize;
      } else if ((Desc->Type == EfiMemoryMappedIO) || (Desc->Type == EfiMemoryMappedIOPortSpace)) {
        // for MMIO block - assign from kernel block
        Desc->VirtualStart = KernelRTAddress + 0xffffff8000000000;
        // next kernel block
        KernelRTAddress += BlockSize;
      } else {
        // runtime flag, but not RT and not MMIO - what now???
        //DBG (" %s with RT flag: %lx (0x%x) - ???\n", EfiMemoryTypeDesc[Desc->Type], Desc->PhysicalStart, Desc->NumberOfPages);
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }
}

/** Copies RT code and data blocks to reserved area inside kernel boot image. */
VOID
DefragmentRuntimeServices (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN OUT UINT32             *EfiSystemTable,
  IN BOOLEAN                SkipOurSysTableRtArea
) {
  UINTN                   NumEntries, Index, BlockSize;
  EFI_MEMORY_DESCRIPTOR   *Desc;
  UINT8                   *KernelRTBlock;

  Desc = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;

  //DBG ("DefragmentRuntimeServices: pBootArgs->efiSystemTable = %x\n", *EfiSystemTable);

  for (Index = 0; Index < NumEntries; Index++) {
    // defragment only RT blocks
    if ((Desc->Type == EfiRuntimeServicesCode) || (Desc->Type == EfiRuntimeServicesData)) {

      // skip our block with sys table copy if required
      if (SkipOurSysTableRtArea && (Desc->PhysicalStart == gSysTableRtArea)) {
        Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
        continue;
      }

      // physical addr from virtual
      KernelRTBlock = (UINT8 *)(UINTN)(Desc->VirtualStart & 0x7FFFFFFFFF);

      BlockSize = EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages);

      //DBG ("-Copy %p <- %p, size=0x%lx\n", KernelRTBlock + gRelocBase, (VOID *)(UINTN)Desc->PhysicalStart, BlockSize);
      CopyMem (KernelRTBlock + gRelocBase, (VOID *)(UINTN)Desc->PhysicalStart, BlockSize);

      // boot.efi zeros old RT areas, but we must not do that because that brakes sleep
      // on some UEFIs. why?
      //SetMem ((VOID *)(UINTN)Desc->PhysicalStart, BlockSize, 0);

      if (
        (EfiSystemTable != NULL) &&
        (Desc->PhysicalStart <= *EfiSystemTable) &&
        (*EfiSystemTable < (Desc->PhysicalStart + BlockSize))
      ) {
        // block contains sys table - update bootArgs with new address
        *EfiSystemTable = (UINT32)((UINTN)KernelRTBlock + (*EfiSystemTable - Desc->PhysicalStart));
        //DBG ("new pBootArgs->efiSystemTable = %x\n", *EfiSystemTable);
      }

      // mark old RT block in MemMap as free mem
      //Desc->Type = EfiConventionalMemory;

      // mark old RT block in MemMap as ACPI NVS
      // if sleep is broken if if those areas are zeroed, maybe
      // it's safer to mark it ACPI NVS then make it free
      Desc->Type = EfiACPIMemoryNVS;

      // and remove RT attribute
      Desc->Attribute = Desc->Attribute & (~EFI_MEMORY_RUNTIME);
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }
}

/** Fixes RT vars in bootArgs, virtualizes and defragments RT blocks. */
VOID
RuntimeServicesFix (
  InternalBootArgs    *BA
) {
  EFI_STATUS              Status;
  UINT32                  gRelocBasePage = (UINT32)EFI_SIZE_TO_PAGES (gRelocBase),
                          DescriptorVersion;
  UINTN                   MemoryMapSize, DescriptorSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;

  MemoryMapSize = *BA->MemoryMapSize;
  MemoryMap = (EFI_MEMORY_DESCRIPTOR *)(UINTN)(*BA->MemoryMap);
  DescriptorSize = *BA->MemoryMapDescriptorSize;
  DescriptorVersion = *BA->MemoryMapDescriptorVersion;

  //DBG ("RuntimeServicesFix: efiRSPageStart=%x, efiRSPageCount=%x, efiRSVirtualPageStart=%lx\n",
  //*BA->efiRuntimeServicesPageStart, *BA->efiRuntimeServicesPageCount, *BA->efiRuntimeServicesVirtualPageStart);

  // fix runtime entries
  *BA->efiRuntimeServicesPageStart -= gRelocBasePage;
  // VirtualPageStart is ok in boot args (a miracle!), but we'll do it anyway
  *BA->efiRuntimeServicesVirtualPageStart = 0x000ffffff8000000 + *BA->efiRuntimeServicesPageStart;
  //DBG ("RuntimeServicesFix: efiRSPageStart=%x, efiRSPageCount=%x, efiRSVirtualPageStart=%lx\n",
  //*BA->efiRuntimeServicesPageStart, *BA->efiRuntimeServicesPageCount, *BA->efiRuntimeServicesVirtualPageStart);

  // Protect RT data areas from relocation by marking then MemMapIO
  ProtectRtDataFromRelocation (MemoryMapSize, DescriptorSize, DescriptorVersion, MemoryMap);

  // assign virtual addresses
  AssignVirtualAddressesToMemMap (MemoryMapSize, DescriptorSize, DescriptorVersion, MemoryMap, EFI_PAGES_TO_SIZE (*BA->efiRuntimeServicesPageStart));

  //PrintMemMap (MemoryMapSize, MemoryMap, DescriptorSize, DescriptorVersion);
  //PrintSystemTable (gST);

  // virtualize RT services with all needed fixes
  Status = ExecSetVirtualAddressesToMemMap (MemoryMapSize, DescriptorSize, DescriptorVersion, MemoryMap);

  //DBG ("SetVirtualAddressMap () = Status: %r\n", Status);
  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
  }

  CopyEfiSysTableToSeparateRtDataArea (BA->efiSystemTable);

  //PrintSystemTable (gST);

  // and defragment
  DefragmentRuntimeServices (MemoryMapSize, DescriptorSize, DescriptorVersion, MemoryMap, BA->efiSystemTable, FALSE);
}

/** DevTree contains /chosen/memory-map with properties with 8 byte values
 * (DTMemMapEntry: UINT32 Address, UINT32 Length):
 * "name" = this is exception - not DTMemMapEntry
 * "BootCLUT" = 8bit boot time colour lookup table
 * "Pict-FailedBoot" = picture shown if booting fails
 * "RAMDisk" = ramdisk
 * "Driver-<hex addr of BooterKextFileInfo>" = Kext, UINT32 Address points to BooterKextFileInfo
 * "DriversPackage-..." = MKext, UINT32 Address points to mkext_header (libkern/libkern/mkext.h), UINT32 length
 *
 * We are fixing here DTMemMapEntry.Address for all those entries.
 * Plus, for every loaded kext, Address points to BooterKextFileInfo,
 * and we are fixing it's pointers also.
*/
VOID
DevTreeFix (
  InternalBootArgs    *BA
) {
          DTEntry                   DevTree, MemMap;
  struct  OpaqueDTPropertyIterator  OPropIter;
          DTPropertyIterator        PropIter = &OPropIter;
          CHAR8                     *PropName;
          DTMemMapEntry             *PropValue;
          BooterKextFileInfo        *KextInfo;

  DevTree = (DTEntry)(UINTN)(*BA->deviceTreeP);

  //DBG ("Fixing DevTree at %p\n", DevTree);
  DTInit (DevTree);
  if (DTLookupEntry (NULL, "/chosen/memory-map", &MemMap) == kSuccess) {
    //DBG ("Found /chosen/memory-map\n");
    if (DTCreatePropertyIteratorNoAlloc (MemMap, PropIter) == kSuccess) {
      //DBG ("DTCreatePropertyIterator OK\n");
      while (DTIterateProperties (PropIter, &PropName) == kSuccess) {
        //DBG ("= %a, val len=%d: ", PropName, PropIter->currentProperty->length);
        // all /chosen/memory-map props have DTMemMapEntry (address, length)
        // values. we need to correct the address

        // basic check that value is 2 * UINT32
        if (PropIter->currentProperty->length != (2 * sizeof (UINT32))) {
          // not DTMemMapEntry, usually "name" property
          //DBG ("NOT DTMemMapEntry\n");
          continue;
        }

        // get value (Address and Length)
        PropValue = (DTMemMapEntry *)(((UINT8 *)PropIter->currentProperty) + sizeof (DeviceTreeNodeProperty));
        //DBG ("MM Addr = %x, Len = %x ", PropValue->Address, PropValue->Length);

        // second check - Address is in our reloc block
        // (note: *BA->kaddr is not fixed yet and points to reloc block)
        if ((PropValue->Address < *BA->kaddr) || (PropValue->Address >= *BA->kaddr + *BA->ksize)) {
          //DBG ("DTMemMapEntry->Address is not in reloc block, skipping\n");
          continue;
        }

        // check if this is Kext entry
        if (AsciiStrnCmp (PropName, BOOTER_KEXT_PREFIX, AsciiStrLen (BOOTER_KEXT_PREFIX)) == 0) {
          // yes - fix kext pointers
          KextInfo = (BooterKextFileInfo *)(UINTN)PropValue->Address;
          //DBG (" = KEXT %a at %x ", (CHAR8 *)(UINTN)KextInfo->bundlePathPhysAddr, KextInfo->infoDictPhysAddr);
          KextInfo->infoDictPhysAddr    -= (UINT32)gRelocBase;
          KextInfo->executablePhysAddr  -= (UINT32)gRelocBase;
          KextInfo->bundlePathPhysAddr  -= (UINT32)gRelocBase;
          //DBG ("-> %x ", KextInfo->infoDictPhysAddr);
        }

        // fix address in mem map entry
        PropValue->Address -= (UINT32)gRelocBase;
        //DBG ("=> Fixed MM Addr = %x\n", PropValue->Address);
      }
    }
  }
}

#if APTIOFIX_VER == 1

/** Fixes stuff when booting with relocation block. Called when boot.efi jumps to kernel. */
UINTN
FixBootingWithRelocBlock (
  UINTN     bootArgs,
  BOOLEAN   ModeX64
) {
  VOID                    *pBootArgs = (VOID *)bootArgs;
  InternalBootArgs        *BA;
  UINTN                   MemoryMapSize, DescriptorSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  UINT32                  DescriptorVersion;

  BootArgsPrint (pBootArgs);

  BA = GetBootArgs (pBootArgs);

  MemoryMapSize = *BA->MemoryMapSize;
  MemoryMap = (EFI_MEMORY_DESCRIPTOR *)(UINTN)(*BA->MemoryMap);
  DescriptorSize = *BA->MemoryMapDescriptorSize;
  DescriptorVersion = *BA->MemoryMapDescriptorVersion;

  // make memmap smaller
  ShrinkMemMap (&MemoryMapSize, MemoryMap, DescriptorSize, DescriptorVersion);

  *BA->MemoryMapSize = (UINT32)MemoryMapSize;

  // fix runtime stuff
  RuntimeServicesFix (BA);

  // fix some values in dev tree
  DevTreeFix (BA);

  // fix boot args
  BootArgsFix (BA, gRelocBase);

  BootArgsPrint (pBootArgs);

  bootArgs = bootArgs - gRelocBase;
  //pBootArgs = (VOID *)bootArgs;

  // set vars for copying kernel
  // note: *BA->kaddr is fixed in BootArgsFix () and points to real kaddr
  AsmKernelImageStartReloc = *BA->kaddr + (UINT32)gRelocBase;
  AsmKernelImageStart = *BA->kaddr;
  AsmKernelImageSize = *BA->ksize;

  return bootArgs;
}

#else

/** Fixes stuff when booting without relocation block. Called when boot.efi jumps to kernel. */
UINTN
FixBootingWithoutRelocBlock (
  UINTN     bootArgs,
  BOOLEAN   ModeX64
) {
  VOID              *pBootArgs = (VOID *)bootArgs;
  //InternalBootArgs  *BA;
  /*
    UINTN         MemoryMapSize;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    UINTN         DescriptorSize;
    UINT32          DescriptorVersion;
  */

  //DBG ("FixBootingWithoutRelocBlock:\n");

  BootArgsPrint (pBootArgs);

  //BA = GetBootArgs (pBootArgs);

  /*
    // Set boot args efi system table to our copied system table
    DBG (" old BA->efiSystemTable = %x:\n", *BA->efiSystemTable);
    *BA->efiSystemTable = (UINT32)gRelocatedSysTableRtArea;
    DBG (" new BA->efiSystemTable = %x:\n", *BA->efiSystemTable);

    // instead of looking into boot args, we'll use
    // last taken memmap with MOGetMemoryMap ().
    // this makes this kind of booting not dependent on boot args format.
    MemoryMapSize = gLastMemoryMapSize;
    MemoryMap = gLastMemoryMap;
    DescriptorSize = gLastDescriptorSize;
    DescriptorVersion = gLastDescriptorVersion;

    // boot.efi zeroed original RT areas, but we need to return them back
    // to fix sleep on some UEFIs
    ReturnPreviousRTAreasContent (MemoryMapSize, MemoryMap);

    // we need to remove RT_code and RT_data flags since they causes GPF on some UEFIs.
    // OSX maps RT_code as Read+Exec only while faulty frivers writes to their
    // static vars which are in RT_code
    RemoveRTFlagMappings (MemoryMapSize, DescriptorSize, DescriptorVersion, MemoryMap);
  */

  // Restore original kernel entry code
  CopyMem ((VOID *)(UINTN)AsmKernelEntry, (VOID *)gOrigKernelCode, gOrigKernelCodeSize);

  // no need to copy anything here
  AsmKernelImageStartReloc = 0x100000;
  AsmKernelImageStart = 0x100000;
  AsmKernelImageSize = 0;

  return bootArgs;
}

#endif
