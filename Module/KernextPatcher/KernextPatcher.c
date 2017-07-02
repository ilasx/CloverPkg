/**
  KernextPatcher (kernel & extensions) patcher for Ozmosis
  Based on Memfix UEFI driver by dmazar
  https://sourceforge.net/p/cloverefiboot/

  @cecekpawon Wed Jun 28 22:56:38 2017
**/

#include <Library/BaseMemoryLib.h>
#include <Library/Common/CommonLib.h>
#include <Library/Common/LoaderUefi.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/Platform/Plist.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/FileInfo.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#define DEBUG_KEXT_PATCHER                          1

#if DEBUG_KEXT_PATCHER
#define DBG(...)                                    AsciiPrint(__VA_ARGS__)
#define DBG_PAUSE(n)                                gBS->Stall(n * 1000000)
#else
#define DBG(...)
#define DBG_PAUSE(...)
#endif


#define PoolPrint(...)                              CatSPrint(NULL, __VA_ARGS__)

#define OZ_PLIST                                    L"\\EFI\\Oz\\Defaults.plist"
#define PropCFBundleIdentifierKey                   "<key>" kPropCFBundleIdentifier "</key>"
#define PropCFBundleVersionKey                      "<key>" kPropCFBundleVersion "</key>"
#define OZ_PATCHES_PLIST_KEY                        "Defaults:99665243-5AED-4D57-92AF-8C785FBC7558"

#define KERNEL_MAX_SIZE                             40000000
#define MAX_FILE_SIZE                               (1024 * 1024 * 1024)

#define MACH_GET_MAGIC(hdr)                         (((struct mach_header_64 *)(hdr))->magic)
#define MACH_GET_NCMDS(hdr)                         (((struct mach_header_64 *)(hdr))->ncmds)
#define MACH_GET_CPU(hdr)                           (((struct mach_header_64 *)(hdr))->cputype)
#define MACH_GET_FLAGS(hdr)                         (((struct mach_header_64 *)(hdr))->flags)
#define SC_GET_CMD(hdr)                             (((struct segment_command_64 *)(hdr))->cmd)

#define PTR_OFFSET(SourcePtr, Offset, ReturnType)   ((ReturnType)(((UINT8 *)SourcePtr) + Offset))

#define kLinkeditSegment                            "__LINKEDIT"

#define kKldSegment                                 "__KLD"
#define kKldTextSection                             "__text"

#define kDataSegment                                "__DATA"
#define kDataDataSection                            "__data"
#define kDataCommonSection                          "__common"

#define kTextSegment                                "__TEXT"
#define kTextTextSection                            "__text"
#define kTextConstSection                           "__const"
#define kTextCstringSection                         "__cstring"

#define kPrelinkTextSegment                         "__PRELINK_TEXT"
#define kPrelinkTextSection                         "__text"

#define kPrelinkLinkStateSegment                    "__PRELINK_STATE"
#define kPrelinkKernelLinkStateSection              "__kernel"
#define kPrelinkKextsLinkStateSection               "__kexts"

#define kPrelinkInfoSegment                         "__PRELINK_INFO"
#define kPrelinkInfoSection                         "__info"

#define kPrelinkBundlePathKey                       "_PrelinkBundlePath"
#define kPrelinkExecutableRelativePathKey           "_PrelinkExecutableRelativePath"
#define kPrelinkExecutableLoadKey                   "_PrelinkExecutableLoadAddr"
#define kPrelinkExecutableSourceKey                 "_PrelinkExecutableSourceAddr"
#define kPrelinkExecutableSizeKey                   "_PrelinkExecutableSize"
#define kPrelinkInfoDictionaryKey                   "_PrelinkInfoDictionary"
#define kPrelinkInterfaceUUIDKey                    "_PrelinkInterfaceUUID"
#define kPrelinkKmodInfoKey                         "_PrelinkKmodInfo"
#define kPrelinkLinkStateKey                        "_PrelinkLinkState"
#define kPrelinkLinkStateSizeKey                    "_PrelinkLinkStateSize"

#define kPropCFBundleIdentifier                     "CFBundleIdentifier"
#define kPropCFBundleVersion                        "CFBundleVersion"
#define kPropCFBundleExecutable                     "CFBundleExecutable"
#define kPropOSBundleRequired                       "OSBundleRequired"
#define kPropOSBundleLibraries                      "OSBundleLibraries"
#define kPropIOKitPersonalities                     "IOKitPersonalities"
#define kPropIONameMatch                            "IONameMatch"

typedef struct KERNEL_INFO {
  UINT32                Slide;
  UINT32                KldAddr;
  UINT32                KldSize;
  UINT32                KldOff;
  UINT8                 KldIndex;
  UINT32                TextAddr;
  UINT32                TextSize;
  UINT32                TextOff;
  UINT8                 TextIndex;
  UINT32                ConstAddr;
  UINT32                ConstSize;
  UINT32                ConstOff;
  UINT8                 ConstIndex;
  //UINT32                CStringAddr;
  //UINT32                CStringSize;
  //UINT32                CStringOff;
  UINT32                DataAddr;
  UINT32                DataSize;
  UINT32                DataOff;
  UINT8                 DataIndex;
  //UINT32                DataCommonAddr;
  //UINT32                DataCommonSize;
  //UINT32                DataCommonOff;
  //UINT8                 DataCommonIndex;
                        // notes:
                        // - 64bit segCmd64->vmaddr is 0xffffff80xxxxxxxx and we are taking
                        //   only lower 32bit part into PrelinkTextAddr
                        // - PrelinkTextAddr is segCmd64->vmaddr + KernelRelocBase
  UINT32                PrelinkTextAddr;
  UINT32                PrelinkTextSize;
  UINT32                PrelinkTextOff;
  UINT8                 PrelinkTextIndex;
                        // notes:
                        // - 64bit sect->addr is 0xffffff80xxxxxxxx and we are taking
                        //   only lower 32bit part into PrelinkInfoAddr
                        // - PrelinkInfoAddr is sect->addr + KernelRelocBase
  UINT32                PrelinkInfoAddr;
  UINT32                PrelinkInfoSize;
  UINT32                PrelinkInfoOff;
  UINT8                 PrelinkInfoIndex;
  UINT32                LoadEXEStart;
  UINT32                LoadEXEEnd;
  UINT32                LoadEXESize;
  UINT32                StartupExtStart;
  UINT32                StartupExtEnd;
  UINT32                StartupExtSize;
  //UINT32                PrelinkedStart;
  //UINT32                PrelinkedEnd;
  //UINT32                PrelinkedSize;
  UINT32                XCPMStart;
  UINT32                XCPMEnd;
  UINT32                XCPMSize;
  UINT32                CPUInfoStart;
  UINT32                CPUInfoEnd;
  UINT32                CPUInfoSize;
  UINT32                VersionMajor;
  UINT32                VersionMinor;
  UINT32                Revision;
  CHAR8                 *Version;
  BOOLEAN               Cached;
  BOOLEAN               A64Bit;
  //BOOLEAN               SSSE3,
  BOOLEAN               PatcherInited;
  //EFI_PHYSICAL_ADDRESS  RelocBase;
  VOID                  *Bin;
} KERNEL_INFO;

typedef enum {
  kLoadEXEStart,
  kLoadEXEEnd,
  kCPUInfoStart,
  kCPUInfoEnd,
  kVersion,
  kVersionMajor,
  kVersionMinor,
  kRevision,
  kXCPMStart,
  kXCPMEnd,
  kStartupExtStart,
  kStartupExtEnd,
  //kPrelinkedStart,
  //kPrelinkedEnd
} KernelPatchSymbolLookupIndex;

typedef struct KERNEL_PATCH_SYMBOL_LOOKUP {
  KernelPatchSymbolLookupIndex    Index;
  CHAR8                           *Name;
} KERNEL_PATCH_SYMBOL_LOOKUP;

STATIC KERNEL_PATCH_SYMBOL_LOOKUP KernelPatchSymbolLookup[] = {
  { kLoadEXEStart, "__ZN6OSKext14loadExecutableEv" },
  { kLoadEXEEnd, "__ZN6OSKext23jettisonLinkeditSegmentEv" },
  { kCPUInfoStart, "_cpuid_set_info" },
  { kCPUInfoEnd, "_cpuid_info" },
  { kVersion, "_version" },
  { kVersionMajor, "_version_major" },
  { kVersionMinor, "_version_minor" },
  { kRevision, "_version_revision" },
  { kXCPMStart, "_xcpm_core_scope_msrs" },
  { kXCPMEnd, "_xcpm_SMT_scope_msrs" },
  { kStartupExtStart, "__ZN12KLDBootstrap21readStartupExtensionsEv" },
  { kStartupExtEnd, "__ZN12KLDBootstrap23readPrelinkedExtensionsEP10section_64" },
  //{ kPrelinkedStart, "__ZN12KLDBootstrap23readPrelinkedExtensionsEP10section_64" },
  //{ kPrelinkedEnd, "__ZN12KLDBootstrap20readBooterExtensionsEv" }
};

STATIC CONST UINTN KernelPatchSymbolLookupCount = ARRAY_SIZE (KernelPatchSymbolLookup);


typedef struct {
  CHAR8     *Name;
  CHAR8     *Label;
  CHAR8     *Filename; // Private
  BOOLEAN   IsPlistPatch;
  INTN      DataLen;
  UINT8     *Data;
  UINT8     *Patch;
  UINT8     Wildcard;
  CHAR8     *MatchOS;
  CHAR8     *MatchBuild;
  BOOLEAN   Disabled;
  BOOLEAN   Patched;
  INTN      Count;
} KEXT_PATCH;

typedef struct {
  CHAR8     *Label;
  INTN      DataLen;
  UINT8     *Data;
  UINT8     *Patch;
  INTN      Count;
  UINT8     Wildcard;
  CHAR8     *MatchOS;
  CHAR8     *MatchBuild;
  BOOLEAN   Disabled;
} KERNEL_PATCH;


EFI_IMAGE_START   gKPStartImage = NULL;

EFI_EVENT         gExitBootServiceEvent = NULL;
KERNEL_INFO       *gKernelInfo;
EFI_FILE          *gSelfRootDir;
UINTN             NrKexts;
KEXT_PATCH        *KextPatches;
UINTN             NrKernels;
KERNEL_PATCH      *KernelPatches;


STATIC
UINT64
GetPlistHexValue (
  CHAR8     *Plist,
  CHAR8     *Key,
  CHAR8     *WholePlist
) {
  UINT64    NumValue = 0;
  CHAR8     *Value, *IntTag, *IDStart, *IDEnd, Buffer[48];
  UINTN     IDLen, BufferLen = ARRAY_SIZE (Buffer);

  // search for Key
  Value = AsciiStrStr (Plist, Key);
  if (Value == NULL) {
    return 0;
  }

  // search for <integer
  IntTag = AsciiStrStr (Value, "<integer");
  if (IntTag == NULL) {
    return 0;
  }

  // find <integer end
  Value = AsciiStrStr (IntTag, ">");
  if (Value == NULL) {
    return 0;
  }

  if (Value[-1] != '/') {
    // normal case: value is here
    NumValue = AsciiStrHexToUint64 (Value + 1);
    return NumValue;
  }

  // it might be a reference: IDREF="173"/>
  Value = AsciiStrStr (IntTag, "<integer IDREF=\"");
  if (Value != IntTag) {
    return 0;
  }

  // compose <integer ID="xxx" in the Buffer
  IDStart = AsciiStrStr (IntTag, "\"") + 1;
  IDEnd = AsciiStrStr (IDStart, "\"");
  IDLen = IDEnd - IDStart;

  if (IDLen > 8) {
    return 0;
  }

  AsciiStrCpyS (Buffer, BufferLen, "<integer ID=\"");
  AsciiStrnCatS (Buffer, BufferLen, IDStart, IDLen);
  AsciiStrCatS (Buffer, BufferLen, "\"");

  // and search whole plist for ID
  IntTag = AsciiStrStr (WholePlist, Buffer);
  if (IntTag == NULL) {
    return 0;
  }

  Value = AsciiStrStr (IntTag, ">");
  if (Value == NULL) {
    return 0;
  }

  if (Value[-1] == '/') {
    return 0;
  }

  // we should have value now
  NumValue = AsciiStrHexToUint64 (Value + 1);

  return NumValue;
}

STATIC
BOOLEAN
IsPatchNameMatch (
  CHAR8   *BundleIdentifier,
  CHAR8   *Name,
  CHAR8   *InfoPlist,
  INT32   *IsBundle
) {
  // Full BundleIdentifier: com.apple.driver.AppleHDA
  *IsBundle = (CountOccurrences (Name, '.') < 2) ? 0 : 1;
  return
    (InfoPlist != NULL) && !IsBundle
      ? (AsciiStrStr (InfoPlist, Name) != NULL)
      : (AsciiStrCmp (BundleIdentifier, Name) == 0);
}

//
// Searches Source for Search pattern of size SearchSize
// and replaces it with Replace up to MaxReplaces times.
// If MaxReplaces <= 0, then there is no restriction on number of replaces.
// Replace should have the same size as Search.
// Returns number of replaces done.
//
STATIC
BOOLEAN
FindWildcardPattern (
  UINT8     *Source,
  UINT8     *Search,
  UINTN     SearchSize,
  UINT8     *Replace,
  UINT8     **NewReplace,
  UINT8     Wildcard
) {
  UINTN     i, SearchCount = 0, ReplaceCount = 0;
  UINT8     *TmpReplace = AllocateZeroPool (SearchSize);
  BOOLEAN   Ret;

  for (i = 0; i < SearchSize; i++) {
    if ((Search[i] != Wildcard) && (Search[i] != Source[i])) {
      SearchCount = 0;
      break;
    }

    if (Replace[i] == Wildcard) {
      TmpReplace[i] = Source[i];
      ReplaceCount++;
    } else {
      TmpReplace[i] = Replace[i];
    }

    SearchCount++;
  }

  Ret = (SearchSize == SearchCount);

  if (!Ret || !ReplaceCount) {
    FreePool (TmpReplace);
  } else {
    *NewReplace = TmpReplace;
  }

  return Ret;
}

STATIC
UINTN
SearchAndReplace (
  UINT8     *Source,
  UINT32    SourceSize,
  UINT8     *Search,
  UINTN     SearchSize,
  UINT8     *Replace,
  UINT8     Wildcard,
  INTN      MaxReplaces
) {
  BOOLEAN   NoReplacesRestriction = (MaxReplaces <= 0);
  UINTN     NumReplaces = 0;
  UINT8     *End = Source + SourceSize;

  if (!Source || !Search || !Replace || !SearchSize) {
    return 0;
  }

  while ((Source < End) && (NoReplacesRestriction || (MaxReplaces > 0))) {
    UINT8   *NewReplace = NULL;

    if (
      ((Wildcard != 0xFF) && FindWildcardPattern (Source, Search, SearchSize, Replace, &NewReplace, Wildcard)) ||
      (CompareMem (Source, Search, SearchSize) == 0)
    ) {
      if (NewReplace != NULL) {
        CopyMem (Source, NewReplace, SearchSize);
        FreePool (NewReplace);
      } else {
        CopyMem (Source, Replace, SearchSize);
      }

      NumReplaces++;
      MaxReplaces--;
      Source += SearchSize;
    } else {
      Source++;
    }
  }

  return NumReplaces;
}

STATIC
UINTN
SearchAndReplaceTxt (
  UINT8     *Source,
  UINT32    SourceSize,
  UINT8     *Search,
  UINTN     SearchSize,
  UINT8     *Replace,
  UINT8     Wildcard,
  INTN      MaxReplaces
) {
  BOOLEAN   NoReplacesRestriction = (MaxReplaces <= 0);
  UINTN     NumReplaces = 0, Skip;
  UINT8     *End = Source + SourceSize, *SearchEnd = Search + SearchSize, *Pos = NULL, *FirstMatch = Source;

  if (!Source || !Search || !Replace || !SearchSize) {
    return 0;
  }

  while (
    ((Source + SearchSize) <= End) &&
    (NoReplacesRestriction || (MaxReplaces > 0))
  ) { // num replaces
    UINT8   *NewReplace = Replace;
    UINTN   i = 0;

    while (*Source != '\0') {  //comparison
      Pos = Search;
      FirstMatch = Source;
      Skip = 0;

      while ((*Source != '\0') && (Pos != SearchEnd)) {
        if (*Source <= 0x20) { //skip invisibles in sources
          Source++;
          Skip++;
          i++;
          continue;
        }

        if (Wildcard != 0xFF) {
          if ((*Source != *Pos) && (Wildcard != *Pos)) {
            break;
          }

          if (Wildcard == NewReplace[i]) {
            NewReplace[i] = *Source;
          }
        }

        Source++;
        Pos++;
        i++;
      }

      if (Pos == SearchEnd) { // pattern found
        Pos = FirstMatch;
        break;
      } else {
        Pos = NULL;
      }

      Source = FirstMatch + 1;
      /*
      if (Pos != Search) {
        AsciiPrint ("\n");
      }
      */
    }

    if (!Pos) {
      break;
    }

    CopyMem (Pos, NewReplace, SearchSize);
    SetMem (Pos + SearchSize, Skip, 0x20); //fill skip places with spaces
    NumReplaces++;
    MaxReplaces--;
    Source = FirstMatch + SearchSize + Skip;
  }

  return NumReplaces;
}

STATIC
VOID
GetTextSection (
  IN  UINT8         *binary,
  OUT UINT32        *Addr,
  OUT UINT32        *Size,
  OUT UINT32        *Off
) {
  struct  load_command        *LoadCommand;
  struct  segment_command_64  *SegCmd64;
  struct  section_64          *Sect64;
          UINT32              NCmds, CmdSize, BinaryIndex, SectionIndex;
          UINTN               i;

  if (MACH_GET_MAGIC (binary) != MH_MAGIC_64) {
    return;
  }

  BinaryIndex = sizeof (struct mach_header_64);

  NCmds = MACH_GET_NCMDS (binary);

  for (i = 0; i < NCmds; i++) {
    LoadCommand = (struct load_command *)(binary + BinaryIndex);

    if (LoadCommand->cmd != LC_SEGMENT_64) {
      continue;
    }

    CmdSize = LoadCommand->cmdsize;
    SegCmd64 = (struct segment_command_64 *)LoadCommand;
    SectionIndex = sizeof (struct segment_command_64);

    while (SectionIndex < SegCmd64->cmdsize) {
      Sect64 = (struct section_64 *)((UINT8 *)SegCmd64 + SectionIndex);
      SectionIndex += sizeof (struct section_64);

      if (
        (Sect64->size > 0) &&
        (AsciiStrCmp (Sect64->segname, kTextSegment) == 0) &&
        (AsciiStrCmp (Sect64->sectname, kTextTextSection) == 0)
      ) {
        *Addr = (UINT32)Sect64->addr;
        *Size = (UINT32)Sect64->size;
        *Off = Sect64->offset;
        //DBG ("%a, %a address 0x%x\n", kTextSegment, kTextTextSection, Off);
        break;
      }
    }

    BinaryIndex += CmdSize;
  }

  return;
}

STATIC
VOID
AnyKextPatch (
  UINT8         *Driver,
  UINT32        DriverSize,
  CHAR8         *InfoPlist,
  UINT32        InfoPlistSize,
  KEXT_PATCH    *KextPatches
) {
  UINTN   Num = 0;

  DBG ("AnyKextPatch: driverAddr = %x, driverSize = %x | AnyKext = %a",
         Driver, DriverSize, KextPatches->Label);

  if (KextPatches->IsPlistPatch) { // Info plist patch
    DBG (" | Info.plist patch");

    Num = SearchAndReplaceTxt (
            (UINT8 *)InfoPlist,
            InfoPlistSize,
            KextPatches->Data,
            KextPatches->DataLen,
            KextPatches->Patch,
            KextPatches->Wildcard,
            KextPatches->Count
          );
  } else { // kext binary patch
    UINT32    Addr, Size, Off;

    GetTextSection (Driver, &Addr, &Size, &Off);

    DBG (" | Binary patch");

    if (Off && Size) {
      Driver += Off;
      DriverSize = Size;
    }

    Num = SearchAndReplace (
            Driver,
            DriverSize,
            KextPatches->Data,
            KextPatches->DataLen,
            KextPatches->Patch,
            KextPatches->Wildcard,
            KextPatches->Count
          );
  }

  DBG (" | %a : %d replaces done\n", Num ? "Success" : "Error", Num);
  DBG_PAUSE (1);
}

//
// PatchKext is called for every kext from prelinked kernel (kernelcache) or from DevTree (booting with drivers).
// Add kext detection code here and call kext specific patch function.
//
STATIC
VOID
PatchKext (
  UINT8         *Driver,
  UINT32        DriverSize,
  CHAR8         *InfoPlist,
  UINT32        InfoPlistSize,
  CHAR8         *BundleIdentifier
) {
  INT32  i, IsBundle = 0;

  for (i = 0; i < NrKexts; i++) {
    if (
      KextPatches[i].Patched ||
      KextPatches[i].Disabled || // avoid redundant: if unique / IsBundle
      !IsPatchNameMatch (BundleIdentifier, KextPatches[i].Name, InfoPlist, &IsBundle)
    ) {
      continue;
    }

    AnyKextPatch (Driver, DriverSize, InfoPlist, InfoPlistSize, &KextPatches[i]);

    if (IsBundle) {
      KextPatches[i].Patched = TRUE;
    }
  }

  DBG ("- %a\n", BundleIdentifier);
  gBS->Stall (10000);
}

/*
STATIC
CHAR16 *
FindExtension (
  IN CHAR16   *FileName
) {
  UINTN   i, len = StrLen (FileName);

  for (i = len; i >= 0; i--) {
    if (FileName[i] == '.') {
      return FileName + i + 1;
    }

    if ((FileName[i] == '/') || (FileName[i] == '\\')) {
      break;
    }
  }

  return FileName;
}
*/


STATIC
BOOLEAN
KernelUserPatch () {
  UINTN    Num, i = 0, y = 0;

  DBG ("%a: Start\n", __FUNCTION__);

  for (i = 0; i < NrKernels; ++i) {
    DBG ("KernelUserPatch[%02d]: %a", i, KernelPatches[i].Label);

    if (KernelPatches[i].Disabled) {
      DBG (" | DISABLED!\n");
      continue;
    }

    Num = SearchAndReplace (
      gKernelInfo->Bin,
      KERNEL_MAX_SIZE,
      KernelPatches[i].Data,
      KernelPatches[i].DataLen,
      KernelPatches[i].Patch,
      KernelPatches[i].Wildcard,
      KernelPatches[i].Count
    );

    if (Num) {
      y++;
    }

    DBG (" | %a : %d replaces done\n", Num ? "Success" : "Error", Num);
    DBG_PAUSE (1);
  }

  DBG ("%a: End\n", __FUNCTION__);

  return (y != 0);
}

STATIC
VOID
FilterKernelPatches () {
  if (
    (KernelPatches != NULL) &&
    NrKernels
  ) {
    UINTN    i = 0, y = 0;

    DBG ("Filtering KernelPatches:\n");

    for (; i < NrKernels; ++i) {
      //BOOLEAN   NeedBuildVersion = (
      //            (Entry->OSBuildVersion != NULL) &&
      //            (KernelPatches[i].MatchBuild != NULL)
      //          );

      DBG (" - [%02d]: %a | [MatchOS: %a | MatchBuild: %a]",
        i,
        KernelPatches[i].Label,
        KernelPatches[i].MatchOS
          ? KernelPatches[i].MatchOS
          : "All",/*
        NeedBuildVersion
          ? KernelPatches[i].MatchBuild
          :*/ "All"
      );

      /*
      if (NeedBuildVersion) {
        KernelPatches[i].Disabled = !IsPatchEnabled (
          KernelPatches[i].MatchBuild, Entry->OSBuildVersion);

        DBG (" ==> %a\n", KernelPatches[i].Disabled ? "not allowed" : "allowed");

        //if (!KernelPatches[i].Disabled) {
          continue; // If user give MatchOS, should we ignore MatchOS / keep reading 'em?
        //}
      }

      KernelPatches[i].Disabled = !IsPatchEnabled (
        KernelPatches[i].MatchOS, Entry->OSVersion);
      */

      DBG (" ==> %a\n", KernelPatches[i].Disabled ? "not allowed" : "allowed");

      if (!KernelPatches[i].Disabled) {
        y++;
      }
    }

    if (y > 0) {
      KernelUserPatch ();
      DBG_PAUSE (3);
    }
  }
}



STATIC
VOID
ExtractKextPropString (
  OUT CHAR8   *Res,
  IN  INTN    Len,
  IN  CHAR8   *Key,
  IN  CHAR8   *Plist
) {
  CHAR8     *Tag, *BIStart, *BIEnd;
  UINTN     DictLevel = 0, KeyLen = AsciiStrLen (Key);

  Res[0] = '\0';

  // start with first <dict>
  Tag = AsciiStrStr (Plist, "<dict>");
  if (Tag == NULL) {
    return;
  }

  Tag += 6;
  DictLevel++;

  while (*Tag != '\0') {
    if (AsciiStrnCmp (Tag, "<dict>", 6) == 0) {
      // opening dict
      DictLevel++;
      Tag += 6;
    } else if (AsciiStrnCmp (Tag, "</dict>", 7) == 0) {
      // closing dict
      DictLevel--;
      Tag += 7;
    } else if ((DictLevel == 1) && (AsciiStrnCmp (Tag, Key, KeyLen) == 0)) {
      // StringValue is next <string>...</string>
      BIStart = AsciiStrStr (Tag + KeyLen, "<string>");
      if (BIStart != NULL) {
        BIStart += 8; // skip "<string>"
        BIEnd = AsciiStrStr (BIStart, "</string>");
        if ((BIEnd != NULL) && ((BIEnd - BIStart + 1) < Len)) {
          CopyMem (Res, BIStart, BIEnd - BIStart);
          Res[BIEnd - BIStart] = '\0';
          break;
        }
      }

      Tag++;
    } else {
      Tag++;
    }

    // advance to next tag
    while ((*Tag != '<') && (*Tag != '\0')) {
      Tag++;
    }
  }
}

STATIC
VOID
PatchPrelinkedKexts () {
  CHAR8     *WholePlist, *DictPtr, *InfoPlistStart = NULL,
            *InfoPlistEnd = NULL, SavedValue;
  INTN      DictLevel = 0;
  UINT32    KextAddr, KextSize;

  WholePlist = (CHAR8 *)(UINTN)gKernelInfo->PrelinkInfoAddr;

  //
  // Detect FakeSMC and if present then
  // disable kext injection InjectKexts ().
  // There is some bug in the folowing code that
  // searches for individual kexts in prelink info
  // and FakeSMC is not found on my SnowLeo although
  // it is present in kernelcache.
  // But searching through the whole prelink info
  // works and that's the reason why it is here.
  //

  //CheckForFakeSMC (WholePlist, Entry);

  DictPtr = WholePlist;

  while ((DictPtr = AsciiStrStr (DictPtr, "dict>")) != NULL) {
    if (DictPtr[-1] == '<') {
      // opening dict
      DictLevel++;
      if (DictLevel == 2) {
        // kext start
        InfoPlistStart = DictPtr - 1;
      }

    } else if ((DictPtr[-2] == '<') && (DictPtr[-1] == '/')) {
      // closing dict
      if ((DictLevel == 2) && (InfoPlistStart != NULL)) {
        CHAR8   KextBundleIdentifier[AVALUE_MAX_SIZE];

        // kext end
        InfoPlistEnd = DictPtr + 5 /* "dict>" */;

        // terminate Info.plist with 0
        SavedValue = *InfoPlistEnd;
        *InfoPlistEnd = '\0';

        // get kext address from _PrelinkExecutableSourceAddr
        // truncate to 32 bit to get physical addr
        KextAddr = (UINT32)GetPlistHexValue (InfoPlistStart, kPrelinkExecutableSourceKey, WholePlist);
        // KextAddr is always relative to 0x200000
        // and if KernelSlide is != 0 then KextAddr must be adjusted
        KextAddr += gKernelInfo->Slide;

        KextSize = (UINT32)GetPlistHexValue (InfoPlistStart, kPrelinkExecutableSizeKey, WholePlist);

        ExtractKextPropString (KextBundleIdentifier, ARRAY_SIZE (KextBundleIdentifier), PropCFBundleIdentifierKey, InfoPlistStart);

        // patch it
        PatchKext (
          (UINT8 *)(UINTN)KextAddr,
          KextSize,
          InfoPlistStart,
          (UINT32)(InfoPlistEnd - InfoPlistStart),
          KextBundleIdentifier
        );

        // return saved char
        *InfoPlistEnd = SavedValue;
      }

      DictLevel--;
    }

    DictPtr += 5;
  }
}

STATIC
VOID
FilterKextPatches () {
  if (
    (KextPatches != NULL) &&
    NrKexts
  ) {
    UINTN    i = 0, y = 0;

    DBG ("Filtering KextPatches:\n");

    for (; i < NrKexts; ++i) {
      //BOOLEAN   NeedBuildVersion = (
      //            (Entry->OSBuildVersion != NULL) &&
      //            (KextPatches[i].MatchBuild != NULL)
      //          );
      //BOOLEAN   NeedBuildVersion = FALSE;

      /*
      // If dot exist in the patch name, store string after last dot to Filename for FSInject to load kext
      if (CountOccurrences (KextPatches[i].Name, '.') >= 2) {
        CHAR16  *Str = AllocateZeroPool (SVALUE_MAX_SIZE);
        UINTN   Len;

        Str = FindExtension (PoolPrint (L"%a", KextPatches[i].Name));
        Len = StrLen (Str) + 1;
        KextPatches[i].Filename = AllocateZeroPool (Len);

        UnicodeStrToAsciiStrS (
          Str,
          KextPatches[i].Filename,
          Len
        );

        FreePool (Str);
      }
      */

      DBG (" - [%02d]: %a | %a | [MatchOS: %a | MatchBuild: %a]",
        i,
        KextPatches[i].Label,
        KextPatches[i].IsPlistPatch ? "PlistPatch" : "BinPatch",
        KextPatches[i].MatchOS
          ? KextPatches[i].MatchOS
          : "All",/*
        NeedBuildVersion
          ? KextPatches[i].MatchBuild
          :*/ "All"
      );

      /*
      if (NeedBuildVersion) {
        KextPatches[i].Disabled = !IsPatchEnabled (
          KextPatches[i].MatchBuild, Entry->OSBuildVersion);

        DBG (" ==> %a\n", KextPatches[i].Disabled ? "not allowed" : "allowed");

        //if (!KextPatches[i].Disabled) {
          continue; // If user give MatchOS, should we ignore MatchOS / keep reading 'em?
        //}
      }

      KextPatches[i].Disabled = !IsPatchEnabled (
        KextPatches[i].MatchOS, Entry->OSVersion);
      */

      DBG (" ==> %a\n", KextPatches[i].Disabled ? "not allowed" : "allowed");

      if (!KextPatches[i].Disabled) {
        y++;
      }
    }

    if (y > 0) {
      PatchPrelinkedKexts ();
      DBG_PAUSE (3);
    }
  }
}

STATIC
VOID
InitKernel () {
  struct  nlist_64            *SysTabEntry;
  struct  symtab_command      *ComSymTab;
  struct  load_command        *LoadCommand;
  struct  segment_command_64  *SegCmd64;
  struct  section_64          *Sect64;
          UINT32              NCmds, CmdSize, BinaryIndex,
                              SectionIndex, Addr, Size, Off,
                              LinkeditAddr = 0, LinkeditFileOff = 0,
                              SymOff = 0, NSyms = 0, StrOff = 0, StrSize = 0;
          UINTN               Cnt;
          UINT8               *Data = (UINT8 *)gKernelInfo->Bin,
                              *SymBin, *StrBin, ISectionIndex = 0;

  BinaryIndex = sizeof (struct mach_header_64);

  NCmds = MACH_GET_NCMDS (Data);

  for (Cnt = 0; Cnt < NCmds; Cnt++) {
    LoadCommand = (struct load_command *)(Data + BinaryIndex);
    CmdSize = LoadCommand->cmdsize;

    switch (LoadCommand->cmd) {
      case LC_SEGMENT_64:
        SegCmd64 = (struct segment_command_64 *)LoadCommand;
        SectionIndex = sizeof (struct segment_command_64);

        if (SegCmd64->nsects == 0) {
          if (AsciiStrCmp (SegCmd64->segname, kLinkeditSegment) == 0) {
            LinkeditAddr = (UINT32)SegCmd64->vmaddr;
            LinkeditFileOff = (UINT32)SegCmd64->fileoff;
            //DBG ("%a: Segment = %a, Addr = 0x%x, Size = 0x%x, FileOff = 0x%x\n", __FUNCTION__,
            //  SegCmd64->segname, LinkeditAddr, SegCmd64->vmsize, LinkeditFileOff
            //);
          }
        }

        if (!ISectionIndex) {
          ISectionIndex++; // Start from 1
        }

        while (SectionIndex < SegCmd64->cmdsize) {
          Sect64 = (struct section_64 *)((UINT8 *)SegCmd64 + SectionIndex);
          SectionIndex += sizeof (struct section_64);

          if (Sect64->size > 0) {
            Addr = (UINT32)(Sect64->addr ? Sect64->addr : 0);
            Size = (UINT32)Sect64->size;
            Off = Sect64->offset;

            if (
              (AsciiStrCmp (Sect64->segname, kPrelinkTextSegment) == 0) &&
              (AsciiStrCmp (Sect64->sectname, kPrelinkTextSection) == 0)
            ) {
              gKernelInfo->PrelinkTextAddr = Addr;
              gKernelInfo->PrelinkTextSize = Size;
              gKernelInfo->PrelinkTextOff = Off;
              gKernelInfo->PrelinkTextIndex = ISectionIndex;
            }
            else if (
              (AsciiStrCmp (Sect64->segname, kPrelinkInfoSegment) == 0) &&
              (AsciiStrCmp (Sect64->sectname, kPrelinkInfoSection) == 0)
            ) {
              gKernelInfo->PrelinkInfoAddr = Addr;
              gKernelInfo->PrelinkInfoSize = Size;
              gKernelInfo->PrelinkInfoOff = Off;
              gKernelInfo->PrelinkInfoIndex = ISectionIndex;
            }
            else if (
              (AsciiStrCmp (Sect64->segname, kKldSegment) == 0) &&
              (AsciiStrCmp (Sect64->sectname, kKldTextSection) == 0)
            ) {
              gKernelInfo->KldAddr = Addr;
              gKernelInfo->KldSize = Size;
              gKernelInfo->KldOff = Off;
              gKernelInfo->KldIndex = ISectionIndex;
            }
            else if (
              (AsciiStrCmp (Sect64->segname, kDataSegment) == 0) &&
              (AsciiStrCmp (Sect64->sectname, kDataDataSection) == 0)
            ) {
              gKernelInfo->DataAddr = Addr;
              gKernelInfo->DataSize = Size;
              gKernelInfo->DataOff = Off;
              gKernelInfo->DataIndex = ISectionIndex;
            }/*
            else if (
              (AsciiStrCmp (Sect64->segname, kDataSegment) == 0) &&
              (AsciiStrCmp (Sect64->sectname, kDataCommonSection) == 0)
            ) {
              gKernelInfo->DataCommonAddr = Addr;
              gKernelInfo->DataCommonSize = Size;
              gKernelInfo->DataCommonOff = Off;
              gKernelInfo->DataCommonIndex = ISectionIndex;
            }*/
            else if (AsciiStrCmp (Sect64->segname, kTextSegment) == 0) {
              if (AsciiStrCmp (Sect64->sectname, kTextTextSection) == 0) {
                gKernelInfo->TextAddr = Addr;
                gKernelInfo->TextSize = Size;
                gKernelInfo->TextOff = Off;
                gKernelInfo->TextIndex = ISectionIndex;
              }
              else if (AsciiStrCmp (Sect64->sectname, kTextConstSection) == 0) {
                gKernelInfo->ConstAddr = Addr;
                gKernelInfo->ConstSize = Size;
                gKernelInfo->ConstOff = Off;
                gKernelInfo->ConstIndex = ISectionIndex;
              }
              #if 0
              else if (
                (AsciiStrCmp (Sect64->sectname, kTextConstSection) == 0) ||
                (AsciiStrCmp (Sect64->sectname, kTextCstringSection) == 0)
              ) {
                GetKernelVersion (Addr, Size, Entry);
              }
              #endif
            }
          }

          ISectionIndex++;
        }
        break;

      case LC_SYMTAB:
        ComSymTab = (struct symtab_command *)LoadCommand;
        SymOff = ComSymTab->symoff;
        NSyms = ComSymTab->nsyms;
        StrOff = ComSymTab->stroff;
        StrSize = ComSymTab->strsize;
        //DBG ("%a: SymOff = 0x%x, NSyms = %d, StrOff = 0x%x, StrSize = %d\n", __FUNCTION__, SymOff, NSyms, StrOff, StrSize);
        break;

      default:
        break;
    }

    BinaryIndex += CmdSize;
  }

  if (ISectionIndex && (LinkeditAddr != 0) && (SymOff != 0)) {
    UINTN     CntPatches = KernelPatchSymbolLookupCount /*+ 2*/;
    CHAR8     *SymbolName = NULL;
    UINT32    PatchLocation;

    Cnt = 0;
    SymBin = (UINT8 *)(UINTN)(LinkeditAddr + (SymOff - LinkeditFileOff));
    StrBin = (UINT8 *)(UINTN)(LinkeditAddr + (StrOff - LinkeditFileOff));

    //DBG ("%a: symaddr = 0x%x, straddr = 0x%x\n", __FUNCTION__, SymBin, StrBin);

    while ((Cnt < NSyms) && CntPatches) {
      SysTabEntry = (struct nlist_64 *)(SymBin);

      if (SysTabEntry->n_value) {
        SymbolName = (CHAR8 *)(StrBin + SysTabEntry->n_un.n_strx);
        Addr = (UINT32)SysTabEntry->n_value;
        PatchLocation = Addr - (UINT32)(UINTN)gKernelInfo->Bin;

        if (SysTabEntry->n_sect == gKernelInfo->TextIndex) {
          if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kLoadEXEStart].Name) == 0) {
            gKernelInfo->LoadEXEStart = PatchLocation;
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kLoadEXEEnd].Name) == 0) {
            gKernelInfo->LoadEXEEnd = PatchLocation;
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kCPUInfoStart].Name) == 0) {
            gKernelInfo->CPUInfoStart = PatchLocation;
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kCPUInfoEnd].Name) == 0) {
            gKernelInfo->CPUInfoEnd = PatchLocation;
            CntPatches--;
          }/*
          else if (AsciiStrCmp (SymbolName, "__ZN6OSKext25updateLoadedKextSummariesEv") == 0) {
            LOLAddr = PatchLocation;
            LOLVal = *(PTR_OFFSET (Data, PatchLocation, UINT64 *));
            CntPatches--;
          }*/
        } else if (SysTabEntry->n_sect == gKernelInfo->ConstIndex) {
          if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kVersion].Name) == 0) {
            gKernelInfo->Version = PTR_OFFSET (Data, PatchLocation, CHAR8 *);
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kVersionMajor].Name) == 0) {
            gKernelInfo->VersionMajor = *(PTR_OFFSET (Data, PatchLocation, UINT32 *));
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kVersionMinor].Name) == 0) {
            gKernelInfo->VersionMinor = *(PTR_OFFSET (Data, PatchLocation, UINT32 *));
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kRevision].Name) == 0) {
            gKernelInfo->Revision = *(PTR_OFFSET (Data, PatchLocation, UINT32 *));
            CntPatches--;
          }
        } else if (SysTabEntry->n_sect == gKernelInfo->DataIndex) {
          if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kXCPMStart].Name) == 0) {
            gKernelInfo->XCPMStart = PatchLocation;
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kXCPMEnd].Name) == 0) {
            gKernelInfo->XCPMEnd = PatchLocation;
            CntPatches--;
          }/*
        } else if (SysTabEntry->n_sect == gKernelInfo->DataCommonIndex) {
          if (AsciiStrCmp (SymbolName, "_gLoadedKextSummaries") == 0) {
            loadedKextSummaries = (PTR_OFFSET (Data, PatchLocation, OSKextLoadedKextSummaryHeader *));
            CntPatches--;
          }*/
        } else if (SysTabEntry->n_sect == gKernelInfo->KldIndex) {
          if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kStartupExtStart].Name) == 0) {
            gKernelInfo->StartupExtStart = PatchLocation;
            CntPatches--;
          }
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kStartupExtEnd].Name) == 0) {
            gKernelInfo->StartupExtEnd = PatchLocation;
            //gKernelInfo->PrelinkedStart = PatchLocation;
            CntPatches--;
          }/*
          //else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kPrelinkedStart].Name) == 0) {
          //  gKernelInfo->PrelinkedStart = PatchLocation;
          //  CntPatches--;
          //}
          else if (AsciiStrCmp (SymbolName, KernelPatchSymbolLookup[kPrelinkedEnd].Name) == 0) {
            gKernelInfo->PrelinkedEnd = PatchLocation;
            CntPatches--;
          }*/
        }
      }

      Cnt++;
      SymBin += sizeof (struct nlist_64);
    }
  } else {
    DBG ("%a: symbol table not found\n", __FUNCTION__);
  }

  if (gKernelInfo->LoadEXEStart && gKernelInfo->LoadEXEEnd && (gKernelInfo->LoadEXEEnd > gKernelInfo->LoadEXEStart)) {
    gKernelInfo->LoadEXESize = (gKernelInfo->LoadEXEEnd - gKernelInfo->LoadEXEStart);
  }

  if (gKernelInfo->StartupExtStart && gKernelInfo->StartupExtEnd && (gKernelInfo->StartupExtEnd > gKernelInfo->StartupExtStart)) {
    gKernelInfo->StartupExtSize = (gKernelInfo->StartupExtEnd - gKernelInfo->StartupExtStart);
  }

  //if (gKernelInfo->PrelinkedStart && gKernelInfo->PrelinkedEnd && (gKernelInfo->PrelinkedEnd > gKernelInfo->PrelinkedStart)) {
  //  gKernelInfo->PrelinkedSize = (gKernelInfo->PrelinkedEnd - gKernelInfo->PrelinkedStart);
  //}

  if (gKernelInfo->XCPMStart && gKernelInfo->XCPMEnd && (gKernelInfo->XCPMEnd > gKernelInfo->XCPMStart)) {
    gKernelInfo->XCPMSize = (gKernelInfo->XCPMEnd - gKernelInfo->XCPMStart);
  }

  if (gKernelInfo->CPUInfoStart && gKernelInfo->CPUInfoEnd && (gKernelInfo->CPUInfoEnd > gKernelInfo->CPUInfoStart)) {
    gKernelInfo->CPUInfoSize = (gKernelInfo->CPUInfoEnd - gKernelInfo->CPUInfoStart);
  }
}

STATIC
BOOLEAN
FindBootArgs () {
  UINT8     *Ptr, ArchMode = sizeof (UINTN) * 8;
  BOOLEAN   Ret = FALSE;

  // start searching from 0x200000.
  Ptr = (UINT8 *)(UINTN)0x200000;

  while (TRUE) {
    // check bootargs for 10.7 and up
    BootArgs2   *mBootArgs = (BootArgs2 *)Ptr;

    if (
      (mBootArgs->Version == 2) && (mBootArgs->Revision == 0) &&
      // plus additional checks - some values are not inited by boot.efi yet
      (mBootArgs->efiMode == ArchMode) &&
      (mBootArgs->kaddr == 0) && (mBootArgs->ksize == 0) &&
      (mBootArgs->efiSystemTable == 0)
    ) {
      // set vars
      //gDtRoot = (CHAR8 *)(UINTN)mBootArgs->deviceTreeP;
      gKernelInfo->Slide = mBootArgs->kslide;

      // Find kernel Mach-O header:
      // for ML: mBootArgs->kslide + 0x00200000
      // for older versions: just 0x200000
      // for AptioFix booting - it's always at 0x200000
      gKernelInfo->Bin = (VOID *)(UINTN)(gKernelInfo->Slide + 0x00200000);

      /*
      DBG ("Found mBootArgs at 0x%08x, DevTree at %p\n", Ptr, dtRoot);
      //DBG ("mBootArgs->kaddr = 0x%08x and mBootArgs->ksize =  0x%08x\n", mBootArgs->kaddr, mBootArgs->ksize);
      //DBG ("mBootArgs->efiMode = 0x%02x\n", mBootArgs->efiMode);
      DBG ("mBootArgs->CommandLine = %a\n", mBootArgs->CommandLine);
      DBG ("mBootArgs->flags = 0x%x\n", mBootArgs->flags);
      DBG ("mBootArgs->kslide = 0x%x\n", mBootArgs->kslide);
      DBG ("mBootArgs->bootMemStart = 0x%x\n", mBootArgs->bootMemStart);
      */

      Ret = TRUE;

      break;
    }

    Ptr += EFI_PAGE_SIZE;
  }

  return Ret;
}

STATIC
BOOLEAN
KernelAndKextPatcherInit () {
  UINT32  Magic;

  if (gKernelInfo->PatcherInited) {
    goto Finish;
  }

  DBG ("%a: Start\n", __FUNCTION__);

  gKernelInfo->PatcherInited = TRUE;

  // Find bootArgs - we need then for proper detection of kernel Mach-O header
  if (!FindBootArgs ()) {
    DBG ("BootArgs not found - skipping patches!\n");
    goto Finish;
  }

  // check that it is Mach-O header and detect architecture
  Magic = MACH_GET_MAGIC (gKernelInfo->Bin);

  if ((Magic == MH_MAGIC_64) || (Magic == MH_CIGAM_64)) {
    DBG ("Found 64 bit kernel at 0x%p\n", gKernelInfo->Bin);
    gKernelInfo->A64Bit = TRUE;
  } else {
    // not valid Mach-O header - exiting
    DBG ("64Bit Kernel not found at 0x%p - skipping patches!", gKernelInfo->Bin);
    gKernelInfo->Bin = NULL;
    goto Finish;
  }

  InitKernel ();

  if (gKernelInfo->VersionMajor < DARWIN_KERNEL_VER_MAJOR_MINIMUM) {
    DBG ("Unsupported kernel version (%d.%d.%d)\n", gKernelInfo->VersionMajor, gKernelInfo->VersionMinor, gKernelInfo->Revision);
    goto Finish;
  }

  gKernelInfo->Cached = ((gKernelInfo->PrelinkTextSize > 0) && (gKernelInfo->PrelinkInfoSize > 0));
  DBG ("Loaded %a | VersionMajor: %d | VersionMinor: %d | Revision: %d\n",
    gKernelInfo->Version, gKernelInfo->VersionMajor, gKernelInfo->VersionMinor, gKernelInfo->Revision
  );

  DBG ("Cached: %s\n", gKernelInfo->Cached ? L"Yes" : L"No");
  DBG ("%a: End\n", __FUNCTION__);

  Finish:
  DBG_PAUSE (3);

  return (gKernelInfo->A64Bit && (gKernelInfo->Bin != NULL));
}

STATIC
VOID
EFIAPI
OnExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
) {
  //LOADER_ENTRY   *Entry = (LOADER_ENTRY *)Context;

  DBG ("**** ExitBootServices called\n");
  DBG_PAUSE (2);

  gKernelInfo = AllocateZeroPool (sizeof (KERNEL_INFO));

  if (KernelAndKextPatcherInit ()) {
    FilterKernelPatches ();
    FilterKextPatches ();
  }

  //DBG_PAUSE (5);
}

STATIC
EFI_STATUS
EventsInitialize () {
  EFI_STATUS    Status;
  VOID          *Registration = NULL;

  //
  // Register notify for exit boot services
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  OnExitBootServices,
                  NULL,
                  &gExitBootServiceEvent
                );

  if (!EFI_ERROR (Status)) {
    gBS->RegisterProtocolNotify (
                         &gEfiStatusCodeRuntimeProtocolGuid,
                         gExitBootServiceEvent,
                         &Registration
                       );
  }

  return EFI_SUCCESS;
}

STATIC
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
) {
  return ConvertDevicePathToText (DevPath, TRUE, TRUE);
}

/** gBS->StartImage override:
 * Called to start an efi image.
 *
 * If this is boot.efi, then run it with our overrides.
 */
//STATIC
EFI_STATUS
EFIAPI
KPStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData  OPTIONAL
) {
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *Image;
  CHAR16                      *FilePathText = NULL;
  BOOLEAN                     StartFlag;

  DBG ("StartImage (%lx)\n", ImageHandle);

  // find out image name from EfiLoadedImageProtocol
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&Image,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );

  if (Status != EFI_SUCCESS) {
    DBG ("ERROR: KPStartImage: OpenProtocol (gEfiLoadedImageProtocolGuid) = %r\n", Status);
    return EFI_INVALID_PARAMETER;
  }

  FilePathText = DevicePathToStr (Image->FilePath);
  if (FilePathText != NULL) {
    DBG ("FilePath: %s\n", FilePathText);
  }

  DBG ("ImageBase: %p - %lx (%lx)\n", Image->ImageBase, (UINT64)Image->ImageBase + Image->ImageSize, Image->ImageSize);

  Status = gBS->CloseProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, gImageHandle, NULL);
  if (EFI_ERROR (Status)) {
    DBG ("CloseProtocol: %r\n", Status);
  }

  StartFlag = (
                (StriStr (FilePathText, L"boot.efi") != NULL) ||
                (StriStr (FilePathText, L"bootbase.efi") != NULL)
              );

  if (StartFlag) {
    DBG ("**** boot.efi\n");
    DBG_PAUSE (5);
    EventsInitialize ();
  }

  Status = gKPStartImage (ImageHandle, ExitDataSize, ExitData);

  if (FilePathText != NULL) {
    gBS->FreePool (FilePathText);
  }

  return Status;
}


//
// file and dir functions
//


STATIC
EFI_FILE_INFO *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE    FHand
) {
  EFI_STATUS      Status;
  EFI_FILE_INFO   *FileInfo = NULL;
  UINTN           Size = 0;

  Status = FHand->GetInfo (FHand, &gEfiFileInfoGuid, &Size, FileInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // inc size by 2 because some drivers (HFSPlus.efi) do not count 0 at the end of file name
    Size += 2;
    FileInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileInfoGuid, &Size, FileInfo);
  }

  return EFI_ERROR (Status) ? NULL : FileInfo;
}

STATIC
EFI_STATUS
LoadFile (
  IN  EFI_FILE_HANDLE   BaseDir,
  IN  CHAR16            *FileName,
  OUT UINT8             **FileData,
  OUT UINTN             *FileDataLength
) {
  EFI_STATUS        Status;
  EFI_FILE_HANDLE   FileHandle;
  EFI_FILE_INFO     *FileInfo;
  UINT64            ReadSize;
  UINTN             BufferSize;
  UINT8             *Buffer;

  Status = BaseDir->Open (BaseDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileInfo = EfiLibFileInfo (FileHandle);

  if (FileInfo == NULL) {
    FileHandle->Close (FileHandle);
    return EFI_NOT_FOUND;
  }

  ReadSize = FileInfo->FileSize;

  if (ReadSize > MAX_FILE_SIZE) {
    ReadSize = MAX_FILE_SIZE;
  }

  FreePool (FileInfo);

  BufferSize = (UINTN)ReadSize;   // was limited to 1 GB above, so this is safe
  Buffer = (UINT8 *)AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    FileHandle->Close (FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FileHandle->Read (FileHandle, &BufferSize, Buffer);
  FileHandle->Close (FileHandle);

  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  *FileData = Buffer;
  *FileDataLength = BufferSize;

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
FileExists (
  IN EFI_FILE   *Root,
  IN CHAR16     *RelativePath
) {
  EFI_STATUS  Status;
  EFI_FILE    *TestFile;

  Status = Root->Open (Root, &TestFile, RelativePath, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS) {
    TestFile->Close (TestFile);
  }

  return (Status == EFI_SUCCESS);
}

STATIC
EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE   DeviceHandle
) {
  EFI_STATUS                        Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Volume;
  EFI_FILE_HANDLE                   File = NULL;

  //
  // File the file system interface to the device
  //
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&Volume
                );

  //
  // Open the root directory of the volume
  //
  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (Volume, &File);
  }

  return EFI_ERROR (Status) ? NULL : File;
}

STATIC
CHAR16 *
SelfPathToString (
  IN EFI_DEVICE_PATH    *DevicePath
) {
  // find the current directory
  CHAR16  *FilePathAsString = DevicePathToStr (DevicePath);

  if (FilePathAsString != NULL) {
    UINTN   i, Len = StrLen (FilePathAsString);
    //SelfFullDevicePath = FileDevicePath (SelfDeviceHandle, FilePathAsString);
    for (i = Len; i > 0 && FilePathAsString[i] != '\\'; i--);
    if (i > 0) {
      FilePathAsString[i] = 0;
    } else {
      FilePathAsString[0] = L'\\';
      FilePathAsString[1] = 0;
    }
  } else {
    FilePathAsString = AllocateCopyPool (StrSize (L"\\"), L"\\");
  }

  return FilePathAsString;
}


//
// Plist
//


STATIC
BOOLEAN
GetPropertyBool (
  TagPtr    Prop,
  BOOLEAN   Default
) {
  return (
    (Prop == NULL)
      ? Default
      : (
          (Prop->type == kTagTypeTrue) ||
          (
            (Prop->type == kTagTypeString) &&
            (TO_AUPPER (Prop->string[0]) == 'Y')
          )
        )
  );
}

STATIC
INTN
GetPropertyInteger (
  TagPtr  Prop,
  INTN    Default
) {
  if (Prop != NULL) {
    if (Prop->type == kTagTypeInteger) {
      return Prop->integer;
    } else if (Prop->type == kTagTypeString) {
      if ((Prop->string[0] == '0') && (TO_AUPPER (Prop->string[1]) == 'X')) {
        return (INTN)AsciiStrHexToUintn (Prop->string);
      }

      if (Prop->string[0] == '-') {
        return -(INTN)AsciiStrDecimalToUintn (Prop->string + 1);
      }

      return (INTN)AsciiStrDecimalToUintn (Prop->string);
    }
  }

  return Default;
}

/*
STATIC
CHAR8 *
GetPropertyString (
  TagPtr  Prop,
  CHAR8   *Default
) {
  if (
    (Prop != NULL) &&
    (Prop->type == kTagTypeString) &&
    AsciiStrLen (Prop->string)
  ) {
    return Prop->string;
  }

  return Default;
}
*/

//
// returns binary setting in a new allocated buffer and data length in dataLen.
// data can be specified in <data></data> base64 encoded
// or in <string></string> hex encoded
//

STATIC
VOID *
GetDataSetting (
  IN   TagPtr   Dict,
  IN   CHAR8    *PropName,
  OUT  UINTN    *DataLen
) {
  TagPtr    Prop;
  UINT8     *Data = NULL;
  UINTN     Len;

  Prop = GetProperty (Dict, PropName);
  if (Prop != NULL) {
    if ((Prop->type == kTagTypeData) && Prop->data && Prop->size) {
      // data property
      Data = AllocateCopyPool (Prop->size, Prop->data);

      if (Data != NULL) {
        *DataLen = Prop->size;
      }
    } else {
      // assume data in hex encoded string property
      Data = StringDataToHex (Prop->string, &Len);
      *DataLen = Len;
    }
  }

  return Data;
}

STATIC
VOID
ParsePatchesPlist (
  IN TagPtr     Dict
) {
  if (Dict != NULL) {
    TagPtr  Prop, DictPointer = GetProperty (Dict, OZ_PATCHES_PLIST_KEY);
    INTN    i, Count;

    if (DictPointer != NULL) {
      Prop = GetProperty (DictPointer, "KextsToPatch");
      if (Prop != NULL) {
        Count = Prop->size;

        //delete old and create new
        if (KextPatches) {
          NrKexts = 0;
          FreePool (KextPatches);
        }

        if (Count > 0) {
          TagPtr        Prop2 = NULL, Dict = NULL;
          KEXT_PATCH    *newPatches = AllocateZeroPool (Count * sizeof (KEXT_PATCH));

          KextPatches = newPatches;
          DBG ("KextsToPatch: %d requested\n", Count);

          for (i = 0; i < Count; i++) {
            CHAR8     *KextPatchesName, *KextPatchesLabel;
            UINTN     FindLen = 0, ReplaceLen = 0;
            UINT8     *TmpData, *TmpPatch;

            EFI_STATUS Status = GetElement (Prop, i, Count, &Prop2);

            DBG (" - [%02d]:", i);

            if (EFI_ERROR (Status) || (Prop2 == NULL)) {
              DBG (" %r parsing / empty element\n", Status);
              continue;
            }

            Dict = GetProperty (Prop2, "Name");
            if ((Dict == NULL) || (Dict->type != kTagTypeString)) {
              DBG (" patch without Name, skip\n");
              continue;
            }

            KextPatchesName = AllocateCopyPool (255, Dict->string);
            KextPatchesLabel = AllocateCopyPool (255, KextPatchesName);

            Dict = GetProperty (Prop2, "Comment");
            if ((Dict != NULL) && (Dict->type == kTagTypeString)) {
              AsciiStrCatS (KextPatchesLabel, 255, " (");
              AsciiStrCatS (KextPatchesLabel, 255, Dict->string);
              AsciiStrCatS (KextPatchesLabel, 255, ")");
            } else {
              AsciiStrCatS (KextPatchesLabel, 255, " (NoLabel)");
            }

            DBG (" %a", KextPatchesLabel);

            Dict = GetProperty (Prop2, "Disabled");
            if (GetPropertyBool (Dict, FALSE)) {
              DBG (" | patch disabled, skip\n");
              continue;
            }

            TmpData    = GetDataSetting (Prop2, "Find", &FindLen);
            TmpPatch   = GetDataSetting (Prop2, "Replace", &ReplaceLen);

            if (!FindLen || !ReplaceLen || (FindLen != ReplaceLen)) {
              DBG (" - invalid Find/Replace data, skip\n");
              continue;
            }

            KextPatches[NrKexts].Data       = AllocateCopyPool (FindLen, TmpData);
            KextPatches[NrKexts].DataLen    = FindLen;
            KextPatches[NrKexts].Patch      = AllocateCopyPool (ReplaceLen, TmpPatch);
            KextPatches[NrKexts].MatchOS    = NULL;
            KextPatches[NrKexts].MatchBuild = NULL;
            KextPatches[NrKexts].Filename   = NULL;
            KextPatches[NrKexts].Disabled   = FALSE;
            KextPatches[NrKexts].Patched    = FALSE;
            KextPatches[NrKexts].Name       = AllocateCopyPool (AsciiStrnLenS (KextPatchesName, 255) + 1, KextPatchesName);
            KextPatches[NrKexts].Label      = AllocateCopyPool (AsciiStrnLenS (KextPatchesLabel, 255) + 1, KextPatchesLabel);
            KextPatches[NrKexts].Count      = GetPropertyInteger (GetProperty (Prop2, "Count"), 0);
            KextPatches[NrKexts].Wildcard   = 0xFF;

            FreePool (TmpData);
            FreePool (TmpPatch);
            FreePool (KextPatchesName);
            FreePool (KextPatchesLabel);

            // check enable/disabled patch (OS based) by Micky1979
            /*
            Dict = GetProperty (Prop2, "MatchOS");
            if ((Dict != NULL) && (Dict->type == kTagTypeString)) {
              KextPatches[NrKexts].MatchOS = AllocateCopyPool (AsciiStrnLenS (Dict->string, 255) + 1, Dict->string);
              DBG (" | MatchOS: %a", KextPatches[NrKexts].MatchOS);
            }

            Dict = GetProperty (Prop2, "MatchBuild");
            if ((Dict != NULL) && (Dict->type == kTagTypeString)) {
              KextPatches[NrKexts].MatchBuild = AllocateCopyPool (AsciiStrnLenS (Dict->string, 255) + 1, Dict->string);
              DBG (" | MatchBuild: %a", KextPatches[NrKexts].MatchBuild);
            }
            */

            // check if this is Info.plist patch or kext binary patch
            KextPatches[NrKexts].IsPlistPatch = GetPropertyBool (GetProperty (Prop2, "InfoPlistPatch"), FALSE);
            Dict = GetProperty (Prop2, "Wildcard");
            if (Dict != NULL) {
              KextPatches[NrKexts].Wildcard = (KextPatches[NrKexts].IsPlistPatch && (Dict->type == kTagTypeString))
                                                ? (UINT8)*Prop->string
                                                : (UINT8)GetPropertyInteger (Dict, 0xFF);
            }

            DBG (" | %a | len: %d\n",
              KextPatches[NrKexts].IsPlistPatch ? "PlistPatch" : "BinPatch",
              KextPatches[NrKexts].DataLen
            );

            NrKexts++;
          }
        }
      }

      Prop = GetProperty (DictPointer, "KernelToPatch");
      if (Prop != NULL) {
        INTN    i, Count = Prop->size;

        //delete old and create new
        if (KernelPatches) {
          NrKernels = 0;
          FreePool (KernelPatches);
        }

        if (Count > 0) {
          TagPtr          Prop2 = NULL, Dict = NULL;
          KERNEL_PATCH    *newPatches = AllocateZeroPool (Count * sizeof (KERNEL_PATCH));

          KernelPatches = newPatches;
          DBG ("KernelToPatch: %d requested\n", Count);

          for (i = 0; i < Count; i++) {
            CHAR8         *KernelPatchesLabel;
            UINTN         FindLen = 0, ReplaceLen = 0;
            UINT8         *TmpData, *TmpPatch;
            EFI_STATUS    Status = GetElement (Prop, i, Count, &Prop2);

            DBG (" - [%02d]:", i);

            if (EFI_ERROR (Status) || (Prop2 == NULL)) {
              DBG (" %r parsing / empty element\n", Status);
              continue;
            }

            Dict = GetProperty (Prop2, "Comment");
            KernelPatchesLabel = ((Dict != NULL) && (Dict->type == kTagTypeString))
                                    ? AllocateCopyPool (AsciiStrSize (Dict->string), Dict->string)
                                    : AllocateCopyPool (8, "NoLabel");

            DBG (" %a", KernelPatchesLabel);

            if (GetPropertyBool (GetProperty (Prop2, "Disabled"), FALSE)) {
              DBG (" | patch disabled, skip\n");
              continue;
            }

            TmpData   = GetDataSetting (Prop2, "Find", &FindLen);
            TmpPatch  = GetDataSetting (Prop2, "Replace", &ReplaceLen);

            if (!FindLen || !ReplaceLen || (FindLen != ReplaceLen)) {
              DBG (" | invalid Find/Replace data, skip\n");
              continue;
            }

            KernelPatches[NrKernels].Data       = AllocateCopyPool (FindLen, TmpData);
            KernelPatches[NrKernels].DataLen    = FindLen;
            KernelPatches[NrKernels].Patch      = AllocateCopyPool (ReplaceLen, TmpPatch);
            KernelPatches[NrKernels].MatchOS    = NULL;
            KernelPatches[NrKernels].MatchBuild = NULL;
            KernelPatches[NrKernels].Disabled   = FALSE;
            KernelPatches[NrKernels].Label      = AllocateCopyPool (AsciiStrSize (KernelPatchesLabel), KernelPatchesLabel);
            KernelPatches[NrKernels].Count      = GetPropertyInteger (GetProperty (Prop2, "Count"), 0);
            KernelPatches[NrKernels].Wildcard   = (UINT8)GetPropertyInteger (GetProperty (Prop2, "Wildcard"), 0xFF);

            FreePool (TmpData);
            FreePool (TmpPatch);
            FreePool (KernelPatchesLabel);

            // check enable/disabled patch (OS based) by Micky1979
            /*
            Dict = GetProperty (Prop2, "MatchOS");
            if ((Dict != NULL) && (Dict->type == kTagTypeString)) {
              KernelPatches[NrKernels].MatchOS = AllocateCopyPool (AsciiStrSize (Dict->string), Dict->string);
              DBG (" | MatchOS: %a", KernelPatches[NrKernels].MatchOS);
            }

            Dict = GetProperty (Prop2, "MatchBuild");
            if ((Dict != NULL) && (Dict->type == kTagTypeString)) {
              KernelPatches[NrKernels].MatchBuild = AllocateCopyPool (AsciiStrSize (Dict->string), Dict->string);
              DBG (" | MatchBuild: %a", KernelPatches[NrKernels].MatchBuild);
            }
            */

            DBG (" | len: %d\n", KernelPatches[NrKernels].DataLen);

            NrKernels++;
          }
        }
      }
    }
  }
}

STATIC
EFI_STATUS
LoadUserSettings (
  IN EFI_FILE   *RootDir,
  TagPtr        *Dict
) {
  EFI_STATUS    Status = EFI_NOT_FOUND;
  UINTN         Size = 0;
  CHAR8         *gConfigPtr = NULL;

  //DbgHeader ("LoadUserSettings");

  if (FileExists (gSelfRootDir, OZ_PLIST)) {
    Status = LoadFile (gSelfRootDir, OZ_PLIST, (UINT8 **)&gConfigPtr, &Size);
    DBG ("Load plist: '%s' ... %r\n", OZ_PLIST, Status);

    if (!EFI_ERROR (Status) && (gConfigPtr != NULL)) {
      Status = ParseXML (gConfigPtr, (UINT32)Size, Dict);
      DBG ("Parsing plist: ... %r\n", Status);
    }
  }

  return Status;
}

STATIC
EFI_STATUS
FinishInitRefitLib (
  IN EFI_HANDLE   DeviceHandle
) {
  EFI_STATUS        Status;

  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;
  EFI_DEVICE_PATH             *DevicePath;
  UINTN                       DevicePathSize;
  TagPtr                      gConfigDict;

  CHAR16    *SelfDirPath = AllocateZeroPool (SVALUE_MAX_SIZE);
  EFI_FILE  *SelfDir;

  if (gSelfRootDir != NULL) {
    gSelfRootDir->Close (gSelfRootDir);
    gSelfRootDir = NULL;
  }

  TmpDevicePath = DevicePathFromHandle (DeviceHandle);
  DevicePathSize = GetDevicePathSize (TmpDevicePath);
  DevicePath = AllocateAlignedPages (EFI_SIZE_TO_PAGES (DevicePathSize), 64);
  CopyMem (DevicePath, TmpDevicePath, DevicePathSize);

  gSelfRootDir = EfiLibOpenRoot (DeviceHandle);
  if (gSelfRootDir == NULL) {
    return EFI_LOAD_ERROR;
  }

  SelfDirPath = SelfPathToString (DevicePath);

  Status = gSelfRootDir->Open (gSelfRootDir, &SelfDir, SelfDirPath, EFI_FILE_MODE_READ, 0);

  if (EFI_ERROR (Status)) {
    DBG ("Error while opening self directory\n");
  } else {
    Status = LoadUserSettings (gSelfRootDir, &gConfigDict);

    if (!EFI_ERROR (Status)) {
      DBG ("Found '%s' : Root = '%s', DevicePath = '%s'\n", OZ_PLIST, SelfDirPath, DevicePathToStr (DevicePath));
      DBG_PAUSE (3);
      ParsePatchesPlist (gConfigDict);
      DBG_PAUSE (3);
    }
  }

  FreePool (SelfDirPath);

  return Status;
}

STATIC
EFI_STATUS
ScanVolumes () {
  EFI_STATUS    Status;
  EFI_HANDLE    *Handles = NULL, DeviceHandle;
  UINTN         HandleCount = 0, HandleIndex;

  //DBG ("Scanning volumes...\n");

  // get all BlockIo handles
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_LOAD_ERROR;

  // first pass: collect information about all handles
  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    VOID    *Instance;

    DeviceHandle = Handles[HandleIndex];

    if (!EFI_ERROR (gBS->HandleProtocol (
                          DeviceHandle,
                          &gEfiPartTypeSystemPartGuid,
                          &Instance)
                   )
    ) {
      if (!EFI_ERROR (FinishInitRefitLib (DeviceHandle))) {
        Status = EFI_SUCCESS;
        break;
      }
    }
  }

  return Status;
}

STATIC
EFI_STATUS
InitRefitLib (
  IN EFI_HANDLE     ImageHandle
) {
  EFI_STATUS          Status;
  EFI_LOADED_IMAGE    *SelfLoadedImage;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&SelfLoadedImage
                );

  if (EFI_ERROR (Status)) {
    DBG ("Error getting LoadedImageProtocol handle\n");
    return Status;
  }

  return FinishInitRefitLib (SelfLoadedImage->DeviceHandle);
}

/** Entry point. Installs our StartImage override.
 * All other stuff will be installed from there when boot.efi is started.
 */
EFI_STATUS
EFIAPI
KernextPatcherEntrypoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
) {
  EFI_STATUS              Status;
  //KEXTPATCHER_PROTOCOL       *KextPatcher;
  //EFI_HANDLE              KextPatcherIHandle;

  Status = InitRefitLib (ImageHandle);

  if (EFI_ERROR (Status)) {
    Status = ScanVolumes ();
  }

  if (!EFI_ERROR (Status)) {
    // install StartImage override
    // all other overrides will be started when boot.efi is started
    gKPStartImage = gBS->StartImage;
    gBS->StartImage = KPStartImage;
    gBS->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
  }

  /*
  // install KEXTPATCHER_PROTOCOL to new handle
  KextPatcher = AllocateZeroPool (sizeof (KEXTPATCHER_PROTOCOL));

  if (KextPatcher == NULL)   {
    DBG ("%a: Can not allocate memory for KEXTPATCHER_PROTOCOL\n", __FUNCTION__);
    return EFI_OUT_OF_RESOURCES;
  }

  KextPatcher->Signature = KEXTPATCHER_SIGNATURE;
  KextPatcherIHandle = NULL; // install to new handle
  Status = gBS->InstallMultipleProtocolInterfaces (&KextPatcherIHandle, &gKextPatcherProtocolGuid, KextPatcher, NULL);

  if (EFI_ERROR (Status)) {
    DBG ("%a: error installing KEXTPATCHER_PROTOCOL, Status = %r\n", __FUNCTION__, Status);
    return Status;
  }
  */

  return EFI_SUCCESS;
}
