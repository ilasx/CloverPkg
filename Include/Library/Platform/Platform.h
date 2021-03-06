/*
Headers collection for procedures
*/

#ifndef __REFIT_PLATFORM_H__
#define __REFIT_PLATFORM_H__

#include "Version.h"

#include <PiDxe.h>
#include <FrameworkDxe.h>

#include <Guid/Acpi.h>

#include <Pi/PiBootMode.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/GenericBdsLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/Common/CommonLib.h>
#include <Library/Common/MemLogLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DataHub.h>
#include <Protocol/DiskIo.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidOverride.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/Smbios.h>
#include <Protocol/UnicodeCollation.h>

#include <Library/UI/UI.h>
#include <Library/UI/PngLib.h>

#include <Guid/DataHubRecords.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Pci.h>

#include <Library/Common/Lib.h>
#include <Library/Common/PlistLib.h>

#include <Protocol/AptioFixProtocol.h>
#include <Protocol/EfiConsoleControl.h>
#include <Protocol/FSInjectProtocol.h>
#include <Protocol/MsgLog.h>

#include <Library/Platform/AppleSmBios.h>
#include <Library/Platform/DeviceInject.h>
#include <Library/Platform/KextInject.h>

#define ROUND_PAGE(x)                   ((((unsigned)(x)) + EFI_PAGE_SIZE - 1) & ~(EFI_PAGE_SIZE - 1))

#define bit(n)                          (1UL << (n))
#define _Bit(n)                         (1ULL << (n))
#define _HBit(n)                        (1ULL << ((n) + 32))

#define bitmask(h,l)                    ((bit(h)|(bit(h) - 1)) & ~(bit(l) - 1))
#define bitfield(x,h,l)                 RShiftU64(((x) & bitmask((h),(l))), (l))
#define quad(hi,lo)                     ((LShiftU64((hi), 32) | (lo)))

#define PCIADDR(bus, dev, func)         ((1 << 31) | ((bus) << 16) | ((dev) << 11) | ((func) << 8))
#define REG8(base, reg)                 ((volatile UINT8 *)(UINTN)(base))[(reg)]
#define REG16(base, reg)                ((volatile UINT16 *)(UINTN)(base))[(reg) >> 1]
#define REG32(base, reg)                ((volatile UINT32 *)(UINTN)(base))[(reg) >> 2]
#define WRITEREG32(base, reg, value)    REG32 ((base), (reg)) = value

#define MEDIA_VALID(kind, type) (\
          ((kind == DISK_KIND_OPTICAL)  && BIT_ISSET (type, VOLTYPE_OPTICAL)) ||\
          ((kind == DISK_KIND_EXTERNAL) && BIT_ISSET (type, VOLTYPE_EXTERNAL)) ||\
          ((kind == DISK_KIND_INTERNAL) && BIT_ISSET (type, VOLTYPE_INTERNAL)) ||\
          ((kind == DISK_KIND_FIREWIRE) && BIT_ISSET (type, VOLTYPE_FIREWIRE))\
        )

#define MEDIA_INVALID(kind, type) (\
          ((kind == DISK_KIND_OPTICAL)  && BIT_ISUNSET (type, VOLTYPE_OPTICAL)) ||\
          ((kind == DISK_KIND_EXTERNAL) && BIT_ISUNSET (type, VOLTYPE_EXTERNAL)) ||\
          ((kind == DISK_KIND_INTERNAL) && BIT_ISUNSET (type, VOLTYPE_INTERNAL)) ||\
          ((kind == DISK_KIND_FIREWIRE) && BIT_ISUNSET (type, VOLTYPE_FIREWIRE))\
        )

//UINT64 AsciiStrVersionToUint64 (CONST CHAR8 *Version, UINT8 MaxDigitByPart, UINT8 MaxParts);
/* Macro to use the AsciiStrVersionToUint64 for OSX Version strings */
#define AsciiOSVersionToUint64(version) AsciiStrVersionToUint64 (version, 2, 3)

#define OSX_EQ(OSVersion, CurrVer) (OSVersion && CurrVer && (AsciiOSVersionToUint64 (OSVersion) == AsciiOSVersionToUint64 (CurrVer)))
#define OSX_LT(OSVersion, CurrVer) (OSVersion && CurrVer && (AsciiOSVersionToUint64 (OSVersion) < AsciiOSVersionToUint64 (CurrVer)))
#define OSX_LE(OSVersion, CurrVer) (OSVersion && CurrVer && (AsciiOSVersionToUint64 (OSVersion) <= AsciiOSVersionToUint64 (CurrVer)))
#define OSX_GT(OSVersion, CurrVer) (OSVersion && CurrVer && (AsciiOSVersionToUint64 (OSVersion) > AsciiOSVersionToUint64 (CurrVer)))
#define OSX_GE(OSVersion, CurrVer) (OSVersion && CurrVer && (AsciiOSVersionToUint64 (OSVersion) >= AsciiOSVersionToUint64 (CurrVer)))


#define MAX_NUM_DEVICES                 64
#define MAX_NUM_GFX                     5

/* Decimal powers: */
#define kilo                            (1000ULL)
#define Mega                            (kilo * kilo)
#define Giga                            (kilo * Mega)
#define Tera                            (kilo * Giga)
#define Peta                            (kilo * Tera)

#define EBDA_BASE_ADDRESS               0x40E
#define EFI_SYSTEM_TABLE_MAX_ADDRESS    0xFFFFFFFF

#define CONFIG_FILENAME                 L"config"

#define CONFIG_THEME_FILENAME           L"theme"
#define CONFIG_THEME_RANDOM             L"random"
#define CONFIG_THEME_EMBEDDED           L"embedded"

#define DIR_CLOVER                      L"\\EFI\\CLOVER"

#define DEV_MARK                        DIR_CLOVER L"\\.dev"

#define DIR_DRIVERS                     DIR_CLOVER L"\\Driver"
#define DIR_MISC                        DIR_CLOVER L"\\Misc"
//#define DIR_OEM                         DIR_CLOVER L"\\Oem"
#define DIR_ROM                         DIR_CLOVER L"\\Rom"
#define DIR_THEMES                      DIR_CLOVER L"\\Theme"
#define DIR_TOOLS                       DIR_CLOVER L"\\Tool"
#define DIR_FONTS                       DIR_CLOVER L"\\Font"

#define DIR_ACPI                        DIR_CLOVER L"\\Acpi"
#define DIR_ACPI_PATCHED                DIR_ACPI L"\\Patch"
#define DIR_ACPI_ORIGIN                 DIR_ACPI L"\\Origin"

#define DSDT_ORIGIN_NAME                DIR_ACPI_ORIGIN L"\\DSDT-or.aml"

#define DIR_KEXTS                       DIR_CLOVER L"\\Kext"
#define DIR_KEXTS_COMMON                DIR_KEXTS "\\Common"
//#define DIR_KEXTS_COMMON_SLAVE           DIR_KEXTS_COMMON "\\Slave"

#define VBIOS_BIN                       DIR_MISC L"\\c0000.bin"

#define PREBOOT_LOG                     DIR_MISC L"\\preboot.log"
#define DEBUG_LOG                       DIR_MISC L"\\debug.log"

#define DATAHUB_LOG                     "boot-log"

#define OSX_PATH_SLE                    L"\\System\\Library\\Extensions"

#define DSDT_CLOVER_PREFIX              L"DSDT-Clover"
#define SSDT_CLOVER_PREFIX              L"SSDT-Clover"

#define DSDT_NAME                       L"DSDT.aml"
#define DSDT_PATCHED_NAME               DSDT_CLOVER_PREFIX L"%x.aml"
#define DSDT_DUMP_LOG                   L"DSDTDump.log"

#define SSDT_PSTATES_NAME               SSDT_CLOVER_PREFIX L"PStates.aml"
#define SSDT_CSTATES_NAME               SSDT_CLOVER_PREFIX L"CStates.aml"

#ifdef DEBUG_ALL
#define MsgLog(...)  MemLog (TRUE, DEBUG_ALL, __VA_ARGS__)
#else
#define MsgLog(...)  MemLog (TRUE, 1, __VA_ARGS__)
#endif

#define DebugLog(Mode, ...) do { if ((Mode > 0) && (Mode < 3)) MemLog (TRUE, Mode, __VA_ARGS__); } while (0)

#ifndef CLOVER_VERSION
  #define CLOVER_VERSION "2.3k"
#endif

#ifndef CLOVER_REVISION
  #define CLOVER_REVISION "0000"
#endif

#define CLOVER_REVISION_STR "Clover " CLOVER_VERSION " (rev " CLOVER_REVISION ")"
#define CLOVER_BASED_INFO "Based on official rev 3884"

#ifndef CLOVER_BUILDDATE
  #define CLOVER_BUILDDATE "Unknown"
#endif

#ifndef EDK2_REVISION
  #define EDK2_REVISION "0000"
#endif

#ifndef CLOVER_BUILDINFOS_STR
  #define CLOVER_BUILDINFOS_STR "Unknown"
#endif


#define DARWIN_SYSTEM_VENDOR            "Apple Inc."
#define DARWIN_SYSTEM_VERSION           "1.0"
#define DARWIN_FIRMWARE_VENDOR          L"Apple"
#define DARWIN_FIRMWARE_REVISION        0x00010000
#define DARWIN_BOARD_LOCATION           "MLB"

#define DEF_NOSIP_CSR_ACTIVE_CONFIG     ( \
                                          CSR_ALLOW_UNTRUSTED_KEXTS | \
                                          CSR_ALLOW_UNRESTRICTED_FS | \
                                          CSR_ALLOW_TASK_FOR_PID | \
                                          CSR_ALLOW_APPLE_INTERNAL | \
                                          CSR_ALLOW_UNRESTRICTED_DTRACE | \
                                          CSR_ALLOW_UNRESTRICTED_NVRAM \
                                        ) // 0x77, 0x67 without CSR_ALLOW_APPLE_INTERNAL

#define DEF_NOSIP_BOOTER_CONFIG         (kBootArgsFlagCSRActiveConfig /* + kBootArgsFlagCSRConfigMode */ + kBootArgsFlagCSRBoot)

#define DEF_DISK_TEMPLATE               L"$label $platform on $path"
#define DEF_DARWIN_DISK_TEMPLATE        L"$platform $label $version ($build) on $path"
#define DEF_DARWIN_RECOVERY_TEMPLATE    DEF_DARWIN_DISK_TEMPLATE // $major $minor $revision $build
#define DEF_DARWIN_INSTALLER_TEMPLATE   DEF_DARWIN_DISK_TEMPLATE
#define DEF_LINUX_TEMPLATE              DEF_DISK_TEMPLATE
//#define DEF_ANDROID_TEMPLATE            DEF_DISK_TEMPLATE
#define DEF_WINDOWS_TEMPLATE            DEF_DISK_TEMPLATE

// NVRAM

#define NVRAM_ATTR_BS                   EFI_VARIABLE_BOOTSERVICE_ACCESS
#define NVRAM_ATTR_RT_BS                (NVRAM_ATTR_BS | EFI_VARIABLE_RUNTIME_ACCESS)
#define NVRAM_ATTR_RT_BS_NV             (NVRAM_ATTR_RT_BS | EFI_VARIABLE_NON_VOLATILE)

typedef enum {
  kNvSystemID,
  kNvMLB,
  kNvHWMLB,
  kNvROM,
  kNvHWROM,
  kNvFirmwareFeatures,
  kNvFirmwareFeaturesMask,
  kNvSBoardID,
  kNvSystemSerialNumber,
  //kNvBlackMode,
  //kNvUIScale,
  kNvBootArgs,
  kNvPlatformUUID,
  kNvBacklightLevel,
  kNvCsrActiveConfig,
  kNvBootercfg,
  kNvBootercfgOnce,
  kNvBootSwitchVar,
  kNvRecoveryBootMode,
  kNvCloverConfig,
  kNvCloverTheme,
  kNvCloverNoEarlyProgress
} NVRAM_KEY;

typedef struct NVRAM_DATA {
  NVRAM_KEY   Key;
  CHAR16      *VariableName;
  EFI_GUID    *Guid;
  UINT32      Attribute;
  UINT8       Reset;
} NVRAM_DATA;

#define CPUID_MODEL_SANDYBRIDGE       0x2A
//#define CPUID_MODEL_JAKETOWN          0x2D

#define CPUID_MODEL_IVYBRIDGE         0x3A
#define CPUID_MODEL_IVYBRIDGE_EP      0x3E

#define CPUID_MODEL_CRYSTALWELL       0x46
#define CPUID_MODEL_HASWELL           0x3C
#define CPUID_MODEL_HASWELL_EP        0x3F
#define CPUID_MODEL_HASWELL_ULT       0x45

#define CPUID_MODEL_BROADWELL         0x3D
//#define CPUID_MODEL_BROADWELL_ULX     0x3D
//#define CPUID_MODEL_BROADWELL_ULT     0x3D
#define CPUID_MODEL_BRYSTALWELL       0x47

#define CPUID_MODEL_SKYLAKE           0x4E
//#define CPUID_MODEL_SKYLAKE_ULT       0x4E
//#define CPUID_MODEL_SKYLAKE_ULX       0x4E
#define CPUID_MODEL_SKYLAKE_DT        0x5E

#define CPUID_MODEL_KABYLAKE          0x8E
#define CPUID_MODEL_KABYLAKE_DT       0x9E

#define BRIDGETYPE_SANDY_BRIDGE       2
#define BRIDGETYPE_IVY_BRIDGE         4
#define BRIDGETYPE_HASWELL            8
#define BRIDGETYPE_BROADWELL          16
#define BRIDGETYPE_SKYLAKE            32
#define BRIDGETYPE_KABYLAKE           64

#define CPU_VENDOR_INTEL              0x756E6547
//#define CPU_VENDOR_AMD              0x68747541
/* Unknown CPU */
#define CPU_STRING_UNKNOWN            "Unknown CPU Type"

/*
 * The CPUID_FEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 1:
 */
#define CPUID_FEATURE_FPU             _Bit (0)   /* Floating point unit on-chip */
#define CPUID_FEATURE_VME             _Bit (1)   /* Virtual Mode Extension */
#define CPUID_FEATURE_DE              _Bit (2)   /* Debugging Extension */
#define CPUID_FEATURE_PSE             _Bit (3)   /* Page Size Extension */
#define CPUID_FEATURE_TSC             _Bit (4)   /* Time Stamp Counter */
#define CPUID_FEATURE_MSR             _Bit (5)   /* Model Specific Registers */
#define CPUID_FEATURE_PAE             _Bit (6)   /* Physical Address Extension */
#define CPUID_FEATURE_MCE             _Bit (7)   /* Machine Check Exception */
#define CPUID_FEATURE_CX8             _Bit (8)   /* CMPXCHG8B */
#define CPUID_FEATURE_APIC            _Bit (9)   /* On-chip APIC */
#define CPUID_FEATURE_SEP             _Bit (11)  /* Fast System Call */
#define CPUID_FEATURE_MTRR            _Bit (12)  /* Memory Type Range Register */
#define CPUID_FEATURE_PGE             _Bit (13)  /* Page Global Enable */
#define CPUID_FEATURE_MCA             _Bit (14)  /* Machine Check Architecture */
#define CPUID_FEATURE_CMOV            _Bit (15)  /* Conditional Move Instruction */
#define CPUID_FEATURE_PAT             _Bit (16)  /* Page Attribute Table */
#define CPUID_FEATURE_PSE36           _Bit (17)  /* 36-bit Page Size Extension */
#define CPUID_FEATURE_PSN             _Bit (18)  /* Processor Serial Number */
#define CPUID_FEATURE_CLFSH           _Bit (19)  /* CLFLUSH Instruction supported */
#define CPUID_FEATURE_DS              _Bit (21)  /* Debug Store */
#define CPUID_FEATURE_ACPI            _Bit (22)  /* Thermal monitor and Clock Ctrl */
#define CPUID_FEATURE_MMX             _Bit (23)  /* MMX supported */
#define CPUID_FEATURE_FXSR            _Bit (24)  /* Fast floating pt save/restore */
#define CPUID_FEATURE_SSE             _Bit (25)  /* Streaming SIMD extensions */
#define CPUID_FEATURE_SSE2            _Bit (26)  /* Streaming SIMD extensions 2 */
#define CPUID_FEATURE_SS              _Bit (27)  /* Self-Snoop */
#define CPUID_FEATURE_HTT             _Bit (28)  /* Hyper-Threading Technology */
#define CPUID_FEATURE_TM              _Bit (29)  /* Thermal Monitor (TM1) */
#define CPUID_FEATURE_PBE             _Bit (31)  /* Pend Break Enable */

#define CPUID_FEATURE_SSE3            _HBit (0)  /* Streaming SIMD extensions 3 */
#define CPUID_FEATURE_PCLMULQDQ       _HBit (1)  /* PCLMULQDQ Instruction */
#define CPUID_FEATURE_DTES64          _HBit (2)  /* 64-bit DS layout */
#define CPUID_FEATURE_MONITOR         _HBit (3)  /* Monitor/mwait */
#define CPUID_FEATURE_DSCPL           _HBit (4)  /* Debug Store CPL */
#define CPUID_FEATURE_VMX             _HBit (5)  /* VMX */
#define CPUID_FEATURE_SMX             _HBit (6)  /* SMX */
#define CPUID_FEATURE_EST             _HBit (7)  /* Enhanced SpeedsTep (GV3) */
#define CPUID_FEATURE_TM2             _HBit (8)  /* Thermal Monitor 2 */
#define CPUID_FEATURE_SSSE3           _HBit (9)  /* Supplemental SSE3 instructions */
#define CPUID_FEATURE_CID             _HBit (10) /* L1 Context ID */
#define CPUID_FEATURE_SEGLIM64        _HBit (11) /* 64-bit segment limit checking */
#define CPUID_FEATURE_CX16            _HBit (13) /* CmpXchg16b instruction */
#define CPUID_FEATURE_xTPR            _HBit (14) /* Send Task PRiority msgs */
#define CPUID_FEATURE_PDCM            _HBit (15) /* Perf/Debug Capability MSR */

#define CPUID_FEATURE_PCID            _HBit (17) /* ASID-PCID support */
#define CPUID_FEATURE_DCA             _HBit (18) /* Direct Cache Access */
#define CPUID_FEATURE_SSE4_1          _HBit (19) /* Streaming SIMD extensions 4.1 */
#define CPUID_FEATURE_SSE4_2          _HBit (20) /* Streaming SIMD extensions 4.2 */
#define CPUID_FEATURE_xAPIC           _HBit (21) /* Extended APIC Mode */
#define CPUID_FEATURE_MOVBE           _HBit (22) /* MOVBE instruction */
#define CPUID_FEATURE_POPCNT          _HBit (23) /* POPCNT instruction */
#define CPUID_FEATURE_TSCTMR          _HBit (24) /* TSC deadline timer */
#define CPUID_FEATURE_AES             _HBit (25) /* AES instructions */
#define CPUID_FEATURE_XSAVE           _HBit (26) /* XSAVE instructions */
#define CPUID_FEATURE_OSXSAVE         _HBit (27) /* XGETBV/XSETBV instructions */
#define CPUID_FEATURE_AVX1_0          _HBit (28) /* AVX 1.0 instructions */
#define CPUID_FEATURE_RDRAND          _HBit (29) /* RDRAND instruction */
#define CPUID_FEATURE_F16C            _HBit (30) /* Float16 convert instructions */
#define CPUID_FEATURE_VMM             _HBit (31) /* VMM (Hypervisor) present */

/*
 * Leaf 7, subleaf 0 additional features.
 * Bits returned in %ebx to a CPUID request with {%eax,%ecx} of (0x7,0x0}:
 */
#define CPUID_LEAF7_FEATURE_RDWRFSGS  _Bit (0)   /* FS/GS base read/write */
#define CPUID_LEAF7_FEATURE_SMEP      _Bit (7)   /* Supervisor Mode Execute Protect */
#define CPUID_LEAF7_FEATURE_ENFSTRG   _Bit (9)   /* ENhanced Fast STRinG copy */

/*
 * The CPUID_EXTFEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 0x80000001:
 */
#define CPUID_EXTFEATURE_SYSCALL      _Bit (11)  /* SYSCALL/sysret */
#define CPUID_EXTFEATURE_XD           _Bit (20)  /* eXecute Disable */
#define CPUID_EXTFEATURE_1GBPAGE      _Bit (26)  /* 1G-Byte Page support */
#define CPUID_EXTFEATURE_RDTSCP       _Bit (27)  /* RDTSCP */
#define CPUID_EXTFEATURE_EM64T        _Bit (29)  /* Extended Mem 64 Technology */

//#define CPUID_EXTFEATURE_LAHF       _HBit (20) /* LAFH/SAHF instructions */
// New definition with Snow kernel
#define CPUID_EXTFEATURE_LAHF         _HBit (0)  /* LAHF/SAHF instructions */
/*
 * The CPUID_EXTFEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 0x80000007:
 */
#define CPUID_EXTFEATURE_TSCI         _Bit (8)   /* TSC Invariant */

#define CPUID_CACHE_SIZE              16        /* Number of descriptor values */

#define CPUID_MWAIT_EXTENSION         _Bit (0)   /* enumeration of WMAIT extensions */
#define CPUID_MWAIT_BREAK             _Bit (1)   /* interrupts are break events     */

/* Known MSR registers */
#define MSR_IA32_PLATFORM_ID          0x0017
#define IA32_APIC_BASE                0x001B    /* used also for AMD */
#define MSR_CORE_THREAD_COUNT         0x0035    /* limited use - not for Penryn or older */
#define IA32_TSC_ADJUST               0x003B
#define MSR_IA32_BIOS_SIGN_ID         0x008B    /* microcode version */
#define MSR_FSB_FREQ                  0x00CD    /* limited use - not for i7 */
/*
• 101B: 100 MHz (FSB 400)
• 001B: 133 MHz (FSB 533)
• 011B: 167 MHz (FSB 667)
• 010B: 200 MHz (FSB 800)
• 000B: 267 MHz (FSB 1067)
• 100B: 333 MHz (FSB 1333)
• 110B: 400 MHz (FSB 1600)
 */
// T8300 -> 0x01A2 => 200MHz
#define MSR_PLATFORM_INFO             0x00CE    /* limited use - MinRatio for i7 but Max for Yonah */
                                                /* turbo for penryn */
//haswell
//Low Frequency Mode. LFM is Pn in the P-state table. It can be read at MSR CEh [47:40].
//Minimum Frequency Mode. MFM is the minimum ratio supported by the processor and can be read from MSR CEh [55:48].
#define MSR_PKG_CST_CONFIG_CONTROL    0x00E2    /* sandy and up */
#define MSR_PMG_IO_CAPTURE_BASE       0x00E4    /* sandy and up */
#define IA32_MPERF                    0x00E7    /* TSC in C0 only */
#define IA32_APERF                    0x00E8    /* actual clocks in C0 */
#define MSR_IA32_EXT_CONFIG           0x00EE    /* limited use - not for i7 */
#define MSR_FLEX_RATIO                0x0194    /* limited use - not for Penryn or older */
                                                //see no value on most CPUs
#define MSR_IA32_PERF_STATUS          0x0198
#define MSR_IA32_PERF_CONTROL         0x0199
#define MSR_IA32_CLOCK_MODULATION     0x019A
#define MSR_THERMAL_STATUS            0x019C
#define MSR_IA32_MISC_ENABLE          0x01A0
#define MSR_THERMAL_TARGET            0x01A2    /* TjMax limited use - not for Penryn or older */
#define MSR_TURBO_RATIO_LIMIT         0x01AD    /* limited use - not for Penryn or older */

//#define IA32_ENERGY_PERF_BIAS       0x01B0
////MSR 000001B0                                      0000-0000-0000-0005
//#define MSR_PACKAGE_THERM_STATUS    0x01B1
////MSR 000001B1                                      0000-0000-8838-0000
//#define IA32_PLATFORM_DCA_CAP       0x01F8
////MSR 000001FC                                      0000-0000-0004-005F

// Sandy Bridge & JakeTown specific 'Running Average Power Limit' MSR's.
#define MSR_RAPL_POWER_UNIT           0x606     /* R/O */
//MSR 00000606                                      0000-0000-000A-1003
#define MSR_PKGC3_IRTL                0x60A     /* RW time limit to go C3 */
          // bit 15 = 1 -- the value valid for C-state PM
#define MSR_PKGC6_IRTL                0x60B     /* RW time limit to go C6 */
//MSR 0000060B                                      0000-0000-0000-8854
  //Valid + 010=1024ns + 0x54=84mks
#define MSR_PKGC7_IRTL                0x60C     /* RW time limit to go C7 */
//MSR 0000060C                                      0000-0000-0000-8854
#define MSR_PKG_C2_RESIDENCY          0x60D     /* same as TSC but in C2 only */

#define MSR_PKG_RAPL_POWER_LIMIT      0x610
//MSR 00000610                                      0000-A580-0000-8960
#define MSR_PKG_ENERGY_STATUS         0x611
//MSR 00000611                                      0000-0000-3212-A857
#define MSR_PKG_POWER_INFO            0x614
//MSR 00000614                                      0000-0000-01E0-02F8
// Sandy Bridge IA (Core) domain MSR's.
#define MSR_PP0_POWER_LIMIT           0x638
#define MSR_PP0_ENERGY_STATUS         0x639
#define MSR_PP0_POLICY                0x63A
#define MSR_PP0_PERF_STATUS           0x63B

// Sandy Bridge Uncore (IGPU) domain MSR's (Not on JakeTown).
#define MSR_PP1_POWER_LIMIT           0x640
#define MSR_PP1_ENERGY_STATUS         0x641
//MSR 00000641                                      0000-0000-0000-0000
#define MSR_PP1_POLICY                0x642

// JakeTown only Memory MSR's.
#define MSR_PKG_PERF_STATUS           0x613
#define MSR_DRAM_POWER_LIMIT          0x618
#define MSR_DRAM_ENERGY_STATUS        0x619
#define MSR_DRAM_PERF_STATUS          0x61B
#define MSR_DRAM_POWER_INFO           0x61C

//IVY_BRIDGE
#define MSR_CONFIG_TDP_NOMINAL        0x648
#define MSR_CONFIG_TDP_LEVEL1         0x649
#define MSR_CONFIG_TDP_LEVEL2         0x64A
#define MSR_CONFIG_TDP_CONTROL        0x64B  /* write once to lock */
#define MSR_TURBO_ACTIVATION_RATIO    0x64C

//Skylake
#define BASE_ART_CLOCK_SOURCE         24000000ULL /* 24Mhz */

#define DEFAULT_FSB                   100000          /* for now, hardcoding 100MHz for old CPUs */

/* CPUID Index */
#define CPUID_0                       (0)
#define CPUID_1                       (1)
#define CPUID_2                       (2)
#define CPUID_3                       (3)
#define CPUID_4                       (4)
#define CPUID_5                       (5)
#define CPUID_6                       (6)
#define CPUID_80                      (7)
#define CPUID_81                      (8)
#define CPUID_87                      (9)
#define CPUID_88                      (10)
#define CPUID_15                      (15)
#define CPUID_MAX                     (16)

/* CPU Cache */
#define MAX_CACHE_COUNT               4
#define CPU_CACHE_LEVEL               3

typedef struct {
 //values from CPUID
  UINT32    CPUID[CPUID_MAX][4];
  UINT32    Vendor;
  UINT32    Signature;
  UINT32    Family;
  UINT32    Model;
  UINT32    Stepping;
  UINT32    Type;
  UINT32    Extmodel;
  UINT32    Extfamily;
  UINT64    Features;
  UINT64    ExtFeatures;
  UINT32    CoresPerPackage;
  UINT32    LogicalPerPackage;
  CHAR8     BrandString[48];

  //values from BIOS
  UINT64    ExternalClock; //keep this values as kHz
  UINT32    MaxSpeed;       //MHz
  UINT32    CurrentSpeed;   //MHz

  //calculated from MSR
  UINT64    MicroCode;
  UINT64    ProcessorFlag;
  UINT32    MaxRatio;
  UINT32    SubDivider;
  UINT32    MinRatio;
  UINT32    DynFSB;
  UINT64    ProcessorInterconnectSpeed; //MHz
  UINT64    FSBFrequency; //Hz
  UINT64    CPUFrequency;
  UINT64    TSCFrequency;
  UINT8     Cores;
  UINT8     EnabledCores;
  UINT8     Threads;
  UINT8     Mobile;  //not for i3-i7
  BOOLEAN   Turbo;

  /* Core i7,5,3 */
  UINT16    Turbo1; //1 Core
  UINT16    Turbo2; //2 Core
  UINT16    Turbo3; //3 Core
  UINT16    Turbo4; //4 Core

  UINT64    TSCCalibr;
  UINT64    ARTFrequency;
} CPU_STRUCTURE;

/* PCI */
#define PCI_BASE_ADDRESS_0            0x10    /* 32 bits */
#define PCI_BASE_ADDRESS_1            0x14    /* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2            0x18    /* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3            0x1c    /* 32 bits */
#define PCI_BASE_ADDRESS_4            0x20    /* 32 bits */
#define PCI_BASE_ADDRESS_5            0x24    /* 32 bits */

#define PCI_CLASS_MEDIA_HDA           0x03

#define EFI_HANDLE_TYPE_UNKNOWN                         0x000
#define EFI_HANDLE_TYPE_IMAGE_HANDLE                    0x001
#define EFI_HANDLE_TYPE_DRIVER_BINDING_HANDLE           0x002
#define EFI_HANDLE_TYPE_DEVICE_DRIVER                   0x004
#define EFI_HANDLE_TYPE_BUS_DRIVER                      0x008
#define EFI_HANDLE_TYPE_DRIVER_CONFIGURATION_HANDLE     0x010
#define EFI_HANDLE_TYPE_DRIVER_DIAGNOSTICS_HANDLE       0x020
#define EFI_HANDLE_TYPE_COMPONENT_NAME_HANDLE           0x040
#define EFI_HANDLE_TYPE_DEVICE_HANDLE                   0x080
#define EFI_HANDLE_TYPE_PARENT_HANDLE                   0x100
#define EFI_HANDLE_TYPE_CONTROLLER_HANDLE               0x200
#define EFI_HANDLE_TYPE_CHILD_HANDLE                    0x400

// ACPI/PATCHED/AML

#define AML_CHUNK_NONE          0xff
#define AML_CHUNK_ZERO          0x00
#define AML_CHUNK_ONE           0x01
#define AML_CHUNK_ALIAS         0x06
#define AML_CHUNK_NAME          0x08
#define AML_CHUNK_BYTE          0x0A
#define AML_CHUNK_WORD          0x0B
#define AML_CHUNK_DWORD         0x0C
#define AML_CHUNK_STRING        0x0D
#define AML_CHUNK_QWORD         0x0E
#define AML_CHUNK_SCOPE         0x10
#define AML_CHUNK_PACKAGE       0x12
#define AML_CHUNK_METHOD        0x14
#define AML_CHUNK_RETURN        0xA4
#define AML_LOCAL0              0x60
#define AML_STORE_OP            0x70
//-----------------------------------
// defines added by pcj
#define AML_CHUNK_BUFFER        0x11
#define AML_CHUNK_STRING_BUFFER 0x15
#define AML_CHUNK_OP            0x5B
#define AML_CHUNK_REFOF         0x71
#define AML_CHUNK_DEVICE        0x82
#define AML_CHUNK_LOCAL0        0x60
#define AML_CHUNK_LOCAL1        0x61
#define AML_CHUNK_LOCAL2        0x62

#define AML_CHUNK_ARG0          0x68
#define AML_CHUNK_ARG1          0x69
#define AML_CHUNK_ARG2          0x6A
#define AML_CHUNK_ARG3          0x6B

#define FIX_MCHC                BIT0
#define FIX_DISPLAY             BIT1
#define FIX_LAN                 BIT2
#define FIX_WIFI                BIT3
#define FIX_HDA                 BIT4
#define FIX_INTELGFX            BIT5
#define FIX_PNLF                BIT6
#define FIX_HDMI                BIT7
#define FIX_IMEI                BIT8
#define FIX_HEADER              BIT9

#define ACPI_NAME_SIZE          4
#define ACPI_OEM_ID_SIZE        6
#define ACPI_OEM_TABLE_ID_SIZE  8
#define ACPI_RSDP_SIG_SIZE      8

typedef struct ACPI_DROP_TABLE {
          UINT32            Signature;
          UINT32            Length;
          UINT64            TableId;
          INPUT_ITEM        MenuItem;
  struct  ACPI_DROP_TABLE   *Next;
} ACPI_DROP_TABLE;

typedef struct ACPI_USER_LOAD {
          UINT8             OSType;
          CHAR16            *FileName;
          CHAR16            *Description;
          INPUT_ITEM        MenuItem;
  struct  ACPI_USER_LOAD    *Next;
} ACPI_USER_LOAD;

typedef struct PATCH_DSDT {
          UINT8       *Find;
          UINT32      LenToFind;
          UINT8       *Replace;
          UINT32      LenToReplace;
          BOOLEAN     Disabled;
          CHAR8       *Comment;
          UINT8       Wildcard;
  struct  PATCH_DSDT  *Next;
} PATCH_DSDT;

typedef struct AML_CHUNK {
          UINT8       Type;
          UINT16      Length;
          CHAR8       *Buffer;
          UINT16      Size;
  struct  AML_CHUNK   *Next;
  struct  AML_CHUNK   *First;
  struct  AML_CHUNK   *Last;
} AML_CHUNK;

struct p_state_vid_fid {
  UINT8 VID;  // Voltage ID
  UINT8 FID;  // Frequency ID
};

typedef struct P_STATE {
  union {
            UINT16            Control;
    struct  p_state_vid_fid   VID_FID;
  } Control;

  UINT32 CID;   // Compare ID
  UINT32 Frequency;
} P_STATE;

typedef struct OPER_REGION {
          CHAR8         Name[8];
          UINT32        Address;
  struct  OPER_REGION   *next;
} OPER_REGION;

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER   Header;
  UINT32                        Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER   Header;
  UINT64                        Entry;
} XSDT_TABLE;

#pragma pack()

//devices
#define DEV_ATI         bit (0)
#define DEV_NVIDIA      bit (1)
#define DEV_INTEL       bit (2)
#define DEV_HDA         bit (3)
#define DEV_HDMI        bit (4)
#define DEV_LAN         bit (5)
#define DEV_WIFI        bit (6)
#define DEV_SATA        bit (7)
#define DEV_IDE         bit (8)
#define DEV_LPC         bit (9)
#define DEV_SMBUS       bit (10)
#define DEV_USB         bit (11)
#define DEV_FIREWIRE    bit (12)
#define DEV_MCHC        bit (13)
#define DEV_BY_PCI      bit (31)

//#define DEV_INDEX_ATI       (0)
//#define DEV_INDEX_NVIDIA    (1)
//#define DEV_INDEX_INTELGFX  (2)
//#define DEV_INDEX_HDMI      (4)
//#define DEV_INDEX_LAN       (5)
//#define DEV_INDEX_WIFI      (6)
//#define DEV_INDEX_FIREWIRE  (12)
//#define DEV_INDEX_USB       (11)
//#define DEV_INDEX_NVME      (13)
#define DEV_INDEX_MAX       (32)

typedef struct {
  UINT16            SegmentGroupNum;
  UINT8             BusNum;
  UINT8             DevFuncNum;
  BOOLEAN           Valid;
  UINT8             SlotID;
  UINT8             SlotType;
  CHAR8             SlotName[31];
  BOOLEAN           ForceInject;
  PCI_DT            PCIDevice;
  UINT8             Type;
} SLOT_DEVICE;

typedef struct DEV_PROPERTY {
          UINTN         Device;
          CHAR8         *Key;
          CHAR8         *Value;
          UINTN         ValueLen;
  struct  DEV_PROPERTY  *Next;
} DEV_PROPERTY;

// GFX

typedef struct {
  BOOLEAN     Intel;
  BOOLEAN     Nvidia;
  BOOLEAN     Ati;
} S_HAS_GRAPHICS;

typedef enum {
  GfxUnknown,
  GfxAti,
  GfxIntel,
  GfxNvidia
} GFX_MANUFACTURER;

typedef struct {
  GFX_MANUFACTURER  Vendor;
  UINT8             Ports;
  UINT16            DeviceID;
  UINT16            Family;
  CHAR8             Model[64];
  CHAR8             Config[64];
  BOOLEAN           LoadVBios;
  UINTN             Segment;
  UINTN             Bus;
  UINTN             Device;
  UINTN             Function;
  EFI_HANDLE        Handle;
  UINT8             *Mmio;
} GFX_PROPERTIES;

//typedef struct {
//  UINT32            Signature;
//  LIST_ENTRY        Link;
//  CHAR8             Model[64];
//  UINT32            Id;
//  UINT32            SubId;
//  UINT64            VideoRam;
//  UINTN             VideoPorts;
//  BOOLEAN           LoadVBios;
//} CARDLIST;

//#define CARDLIST_SIGNATURE SIGNATURE_32('C','A','R','D')

// RAM

typedef struct {
  BOOLEAN   InUse;
  UINT8     Type;
  UINT32    ModuleSize;
  UINT32    Frequency;
  CHAR8     *Vendor;
  CHAR8     *PartNo;
  CHAR8     *SerialNo;
} RAM_SLOT_INFO;

// The maximum number of RAM slots to detect
// even for 3-channels chipset X58 there are no more then 8 slots
#define MAX_RAM_SLOTS       8 //24
// The maximum sane frequency for a RAM module
#define MAX_RAM_FREQUENCY   5000

typedef struct {
  UINT32    Frequency;
  UINT32    Divider;
  UINT8     TRC;
  UINT8     TRP;
  UINT8     RAS;
  UINT8     Channels;
  UINT8     Slots;
  UINT8     Type;
  UINT8     SPDInUse;
  UINT8     SMBIOSInUse;
  UINT8     UserInUse;
  UINT8     UserChannels;

  RAM_SLOT_INFO SPD[MAX_RAM_SLOTS * 4];
  RAM_SLOT_INFO SMBIOS[MAX_RAM_SLOTS * 4];
  RAM_SLOT_INFO User[MAX_RAM_SLOTS * 4];
} MEM_STRUCTURE;

// CONFIG

typedef enum {
  MinMachineType,

  // Desktop

  MacMini53,
  MacMini62,
  MacMini71,
  iMac162,
  iMac171,
  iMac183,

  MacPro61,

  // Mobile

  MacBookPro83,
  MacBookPro102,
  MacBookPro115,
  MacBookPro121,
  MacBookPro133,
  MacBookPro143,

  MaxMachineType
} MACHINE_TYPES;

typedef struct S_FILES {
          CHAR16    *FileName;
          CHAR16    *Description;
          INTN      Index;
  struct  S_FILES   *Next;
} S_FILES;

//#define NUM_OF_CONFIGS 3

// Kernel scan states
#define KERNEL_SCAN_ALL         (0)
//#define KERNEL_SCAN_NEWEST      (1)
//#define KERNEL_SCAN_OLDEST      (2)
//#define KERNEL_SCAN_FIRST       (3)
//#define KERNEL_SCAN_LAST        (4)
//#define KERNEL_SCAN_MOSTRECENT  (5)
//#define KERNEL_SCAN_EARLIEST    (6)
#define KERNEL_SCAN_NONE        (100)

typedef struct {
  BOOLEAN   AptioFixEmbedded;
  BOOLEAN   FSInjectEmbedded;
  BOOLEAN   AptioFixLoaded;
  INT32     AptioFixVersion;
  BOOLEAN   HFSLoaded;
  BOOLEAN   APFSLoaded;
} DRIVERS_FLAGS;

typedef enum {
  english = 0,  //en
  //djawa, //dj
  //something else? add, please
} LANGUAGES;

typedef struct {
  CHAR8   *Title;
  UINTN   Bit;
} OPT_BITS;

// Settings.c
// Micky1979: Next five functions (+ needed struct) are to split a string like "10.10.5,10.7,10.11.6,10.8.x"
// in their components separated by comma (in this case)
typedef struct MatchOSes {
  UINTN   count;
  CHAR8   *array[100];
} MatchOSes;

typedef struct {
  UINT8     *Mmio[4]; // their MMIO regions
  UINT8     Mac[4][6]; // their MAC addresses
  UINT16    Vendor[4]; // their vendors
  BOOLEAN   Legacy;
  UINTN     Cards; // number of LAN cards
  UINTN     Paths; // number of LAN paths
} S_GLAN;

typedef struct {
  // SMBIOS TYPE0
  CHAR8                     VendorName[64];
  CHAR8                     RomVersion[64];
  CHAR8                     ReleaseDate[64];
  // SMBIOS TYPE1
  CHAR8                     ManufactureName[64];
  CHAR8                     ProductName[64];
  CHAR8                     VersionNr[64];
  CHAR8                     SerialNr[64];
  EFI_GUID                  OemSystemID;
  EFI_GUID                  SystemID;
  EFI_GUID                  PlatformUUID;
  CHAR8                     FamilyName[64];
  CHAR8                     OEMProduct[64];
  CHAR8                     OEMVendor[64];
  // SMBIOS TYPE2
  CHAR8                     BoardManufactureName[64];
  CHAR8                     BoardSerialNumber[64];
  CHAR8                     BoardNumber[64]; //Board-ID
  CHAR8                     LocationInChassis[64];
  CHAR8                     BoardVersion[64];
  CHAR8                     OEMBoard[64];
  UINT8                     BoardType;
  // SMBIOS TYPE3
  BOOLEAN                   Mobile;
  BOOLEAN                   CustomMobile;
  UINT8                     ChassisType;
  CHAR8                     ChassisManufacturer[64];
  CHAR8                     ChassisAssetTag[64];
  // SMBIOS TYPE4
  UINT32                    CpuFreqMHz;
  UINT32                    BusSpeed; //in kHz
  BOOLEAN                   Turbo;
  UINT8                     EnabledCores;
  // SMBIOS TYPE17
  CHAR8                     MemoryManufacturer[64];
  CHAR8                     MemorySerialNumber[64];
  CHAR8                     MemoryPartNumber[64];
  CHAR8                     MemorySpeed[64];
  // SMBIOS TYPE131
  UINT16                    CpuType;
  // SMBIOS TYPE132
  UINT16                    QPI;
  BOOLEAN                   TrustSMBIOS;
  BOOLEAN                   InjectMemoryTables;
  INT8                      XMPDetection;
  BOOLEAN                   UseARTFreq;
  // SMBIOS TYPE133
  UINT64                    PlatformFeature;

  // OS parameters
  //CHAR8                     Language[16];
  CHAR8                     BootArgs[256];
  CHAR8                     *BootArgsNVRAM;
  CHAR16                    *DefaultVolume;
  BOOLEAN                   FastBoot;
  BOOLEAN                   NoEarlyProgress;
  BOOLEAN                   TextOnly;
  BOOLEAN                   DebugLog;
  INTN                      Timeout;

  BOOLEAN                   LastBootedVolume;

  CHAR16                    *DefaultLoader;

  UINT16                    BacklightLevel;
  BOOLEAN                   IntelBacklight;
  BOOLEAN                   WithKexts;
  BOOLEAN                   NoCaches;
  BOOLEAN                   FakeSMCOverrides;

  // GUI parameters
  BOOLEAN                   DebugKP;

  //ACPI
  BOOLEAN                   DropSSDT;
  BOOLEAN                   GeneratePStates;
  BOOLEAN                   GenerateCStates;
  BOOLEAN                   DoubleFirstState;
  BOOLEAN                   EnableC2;
  BOOLEAN                   EnableC4;
  BOOLEAN                   EnableC6;
  UINT16                    C3Latency;
  BOOLEAN                   SmartUPS;
  BOOLEAN                   EnableC7;
  UINT8                     MinMultiplier;
  UINT8                     MaxMultiplier;

  CHAR16                    DsdtName[28];
  UINT32                    FixDsdt;
  UINT8                     PluginType;
  //BOOLEAN                   DropMCFG;

  UINT32                    PatchDsdtNum;
  BOOLEAN                   DebugDSDT;
  BOOLEAN                   UseIntelHDMI;

  PATCH_DSDT                *PatchDsdt;
  ACPI_DROP_TABLE           *ACPIDropTables;
  UINTN                     ACPISortedCount;
  CHAR16                    **ACPISorted;
  UINT32                    ACPIDisabledCount;
  CHAR16                    **ACPIDisabled;

  //Injections
  BOOLEAN                   EFIStringInjector;
  BOOLEAN                   NoDefaultProperties;

  BOOLEAN                   ReuseFFFF;

  //PCI devices
  UINT32                    FakeATI;
  UINT32                    FakeNVidia;
  UINT32                    FakeIntel;
  UINT32                    FakeLAN;
  UINT32                    FakeWIFI;
  UINT32                    FakeIMEI;

  //Graphics
  //UINT16                    PCIRootUID;
  BOOLEAN                   InjectIntel;
  BOOLEAN                   InjectATI;
  BOOLEAN                   InjectNVidia;
  BOOLEAN                   LoadVBios;

  BOOLEAN                   InjectEDID;
  UINT16                    DropOEM_DSM;
  UINT8                     *EDID;
  UINT8                     *CustomEDID;

  CHAR16                    FBName[16];
  UINT16                    VideoPorts;
  BOOLEAN                   NvidiaSingle;
  BOOLEAN                   NvidiaNoEFI;
  UINT64                    VRAM;
  //UINT8                     Dcfg[8];
  UINT8                     NVCAP[20];
  INT8                      BootDisplay;
  UINT32                    DualLink;
  UINT32                    IgPlatform;
  S_HAS_GRAPHICS            *HasGraphics;

  S_GLAN                    GLAN;
  UINTN                     NGFX; // number of GFX
  GFX_PROPERTIES            Graphics[5]; // 4 descrete cards max + iGPU
  CPU_STRUCTURE             CPUStructure;
  SLOT_DEVICE               SlotDevices[DEV_INDEX_MAX];
  MEM_STRUCTURE             RAM;

  //Add custom properties
  INTN                      NrAddProperties;
  DEV_PROPERTY              *AddProperties;

  // HDA
  UINT32                    HDALayoutId;

  // Volumes hiding
  CHAR16                    **HVHideStrings;

  INTN                      HVCount;

  // KernelAndKextPatches
  KERNEL_AND_KEXT_PATCHES   KernelAndKextPatches;
  BOOLEAN                   KextPatchesAllowed;
  BOOLEAN                   KernelPatchesAllowed; //From GUI: Only for user patches, not internal Clover
  BOOLEAN                   BooterPatchesAllowed;
  BOOLEAN                   KernelPatchesWholePrelinked;
  BOOLEAN                   KPKernelPm;

  // SysVariables
  CHAR8                     *RtMLB;
  UINT8                     *RtROM;
  UINTN                     RtROMLen;
  UINT32                    CsrActiveConfig;
  UINT16                    BooterConfig;
  UINT32                    FwFeatures;
  UINT32                    FwFeaturesMask;

  // Multi-config
  CHAR16                    *ConfigName;

  // Drivers
  INTN                      BlackListCount;
  CHAR16                    **BlackList;

  // Kexts
  INTN                      BlockKextCachesCount;
  CHAR8                     **BlockKextCaches;

  // BlackListed kexts
  CHAR16                    BlockKexts[64]; // FSInject business

  //SMC keys
  CHAR8                     RPlt[8];
  CHAR8                     RBr[8];
  UINT8                     EPCI[4];
  UINT8                     REV[6];

  // Entries
  BOOLEAN                   DisableEntryScan;
  BOOLEAN                   DisableToolScan;
  BOOLEAN                   ShowHiddenEntries;
  UINT8                     KernelScan;
  BOOLEAN                   LinuxScan;
  BOOLEAN                   AndroidScan;

  CUSTOM_LOADER_ENTRY       *CustomEntries;

  CHAR16                    DarwinDiskTemplate[255];
  CHAR16                    DarwinRecoveryDiskTemplate[255];
  CHAR16                    DarwinInstallerDiskTemplate[255];
  CHAR16                    LinuxDiskTemplate[255];
  //CHAR16                    AndroidDiskTemplate[255];
  CHAR16                    WindowsDiskTemplate[255];

  CUSTOM_TOOL_ENTRY         *CustomTool;

  UINT32                    OptionsBits;
  UINT32                    FlagsBits;
  BOOLEAN                   FakeSMCLoaded;

  DRIVERS_FLAGS             DriversFlags;

  BOOLEAN                   Dev;
} SETTINGS_DATA;


extern REFIT_MENU_ENTRY                 gMenuEntryReturn;
extern REFIT_MENU_SCREEN                gMainMenu;

extern EFI_GUID                         *gEfiBootDeviceGuid;
extern EFI_DEVICE_PATH_PROTOCOL         *gEfiBootDeviceData;
extern SETTINGS_DATA                    gSettings;
extern LANGUAGES                        gLanguage;
extern UINT32                           gmPropSize;
extern UINT8                            *gmProperties;
extern CHAR8                            *gmDeviceProperties;
extern UINT32                           gcPropSize;
extern UINT8                            *gcProperties;
extern CHAR8                            *gcDeviceProperties;
extern CHAR16                           *gInjectKextsDir[2];

extern EFI_EVENT                        ExitBootServiceEvent;

extern UINT32                           gDevicesNumber;
extern INTN                             gOldChosenTheme;
extern INTN                             gOldChosenConfig;
extern INTN                             gOldChosenDSDT;
extern INTN                             gOldChosenTool;

extern UINT8                            gAcpiCPUCount;
extern CHAR8                            *gAcpiCPUName[32];
extern CHAR8                            *gAcpiCPUScore;

// ACPI/PATCHED/AML
extern ACPI_USER_LOAD                   *gACPIUserLoad;

extern S_FILES                          *gConfigFiles;
extern S_FILES                          *gThemeFiles;
extern S_FILES                          *gDSDTFiles;
extern S_FILES                          *gToolFiles;
extern CHAR16                           *gToolPath;

extern UINTN                            gACPIDropTablesNum;
extern UINTN                            gACPIUserLoadNum;

extern OPT_BITS                         gADEVICES[];
extern UINT8                            gOptDevicesBitNum;

extern OPT_BITS                         gAFIXDSDT[];
extern UINT8                            gOptFixDSDTBitNum;

extern OPT_BITS                         gABOOTERCFG[];
extern UINT8                            gOptBooterCfgBitNum;

extern OPT_BITS                         gACSRCFG[];
extern UINT8                            gOptCsrCfgBitNum;

extern CONST CHAR16                     *gOsxPathLCaches[];
extern CONST UINTN                      gOsxPathLCachesCount;

extern CONST NVRAM_DATA                 gNvramData[];
extern CHAR16                           *gSupportedOsType[3];

// loader
VOID ScanLoader ();
VOID AddCustomEntries ();

// tool
VOID ScanTool ();
VOID AddCustomTool ();
VOID ScanTools ();

VOID
StartTool (
  IN LOADER_ENTRY   *Entry
);

VOID
StartToolFromMenu ();

//-----------------------------------

VOID
FixBiosDsdt (
  UINT8     *Temp,
  BOOLEAN   Patched
);

VOID
FindCPU (
  UINT8   *Dsdt
);

INT32
FindBin (
  UINT8   *Bin,
  UINT32  BinLen,
  UINT8   *Pattern,
  UINT32  PatternLen
);

EFI_STATUS
WaitForInputEventPoll (
  REFIT_MENU_SCREEN   *Screen,
  UINTN               TimeoutDefault
);

EFI_STATUS
SetupBooterLog ();

EFI_STATUS
SaveBooterLog (
  IN  EFI_FILE_HANDLE   BaseDir  OPTIONAL,
  IN  CHAR16            *FileName
);

VOID
SetDMISettingsForModel (
  MACHINE_TYPES   Model,
  BOOLEAN         Redefine
);

MACHINE_TYPES
GetModelFromString (
  CHAR8   *ProductName
);

CHAR8 *
GetAppleReleaseDate (
  IN CHAR8  *Version
);

VOID
FillInputs (
  BOOLEAN   New
);

VOID
ApplyInputs ();

EFI_GUID *
SwapGuid (
  IN EFI_GUID   Guid
);

BOOLEAN
IsValidGuidAsciiString (
  IN CHAR8  *Str
);

EFI_STATUS
InitializeEdidOverride ();

EFI_STATUS
GetEdidDiscovered ();

//Settings.c

VOID
InitializeSettings ();

VOID
SyncDefaultSettings ();

VOID
GetCPUProperties ();

VOID
SyncCPUProperties ();

VOID
SetCPUProperties ();

VOID
GetDevices ();

MACHINE_TYPES
GetDefaultModel ();

UINT16
GetAdvancedCpuType ();

CHAR16 *
GetOSIconName (
  IN  SVersion  *SDarwinVersion
);

EFI_STATUS
GetRootUUID (
  IN  REFIT_VOLUME  *Volume
);

EFI_STATUS
GetUserSettings (
  IN TagPtr     Dict
);

EFI_STATUS
InitTheme (
  BOOLEAN   UseThemeDefinedInNVRam,
  EFI_TIME  *Time
);

EFI_STATUS
SetFSInjection (
  IN LOADER_ENTRY   *Entry
);

VOID
SetupBooterCsrCfg (
  IN LOADER_ENTRY   *Entry
);

//
// Nvram.c
//

VOID *
GetNvramVariable (
  IN   CHAR16     *VariableName,
  IN   EFI_GUID   *VendorGuid,
  OUT  UINT32     *Attributes    OPTIONAL,
  OUT  UINTN      *DataSize      OPTIONAL
);

EFI_STATUS
AddNvramVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
);

EFI_STATUS
SetNvramVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
);

EFI_STATUS
SetOrDeleteNvramVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data,
  IN  BOOLEAN   State
);

EFI_STATUS
DeleteNvramVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid
);

EFI_STATUS
ResetNvram ();

VOID
ResetClover ();

VOID
DumpBitsConfig (
  CHAR8       *Label,
  UINT32      Bits,
  OPT_BITS    *BitsArray,
  UINT8       NumArray
);

VOID
SyncBootArgsFromNvram ();

EFI_STATUS
GetEfiBootDeviceFromNvram ();

EFI_GUID *
FindGPTPartitionGuidInDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
);

VOID
GetSmcKeys ();

VOID
GetMacAddress ();

INTN
FindStartupDiskVolume (
  REFIT_MENU_SCREEN   *MenuScreen
);

EFI_STATUS
SetStartupDiskVolume (
  IN  REFIT_VOLUME  *Volume,
  IN  CHAR16        *LoaderPath
);

VOID
RemoveStartupDiskVolume ();

EFI_STATUS
EFIAPI
LogDataHub (
  IN  EFI_GUID  *TypeGuid,
  IN  CHAR16    *Name,
  IN  VOID      *Data,
  IN  UINT32    DataSize
);

VOID
EFIAPI
SaveDarwinLog ();

EFI_STATUS
EFIAPI
SetVariablesForOSX ();

VOID
EFIAPI
SetupDataForOSX ();

EFI_STATUS
SetPrivateVarProto ();

VOID
SyncDevices ();

VOID
SetDevices (
  LOADER_ENTRY  *Entry
);

VOID
ScanSPD ();

BOOLEAN
SetupAtiDevprop (
  LOADER_ENTRY  *Entry,
  PCI_DT        *Dev
);

BOOLEAN
SetupGmaDevprop (
  PCI_DT    *Dev
);

BOOLEAN
SetupEthernetDevprop (
  PCI_DT    *Dev
);

BOOLEAN
SetupHdaDevprop (
  PCI_DT    *Dev
);

BOOLEAN
SetupNvidiaDevprop (
  PCI_DT    *Dev
);

//VOID
//FillCardList (
//  TagPtr  CfgDict
//);

//CARDLIST *
//FindCardWithIds (
//  UINT32  Id,
//  UINT32  SubId
//);

UINT32
PatchBinACPI (
  UINT8   *Ptr,
  UINT32  Len
);

EFI_STATUS
PatchACPI (
  UINT8   OSType
);

UINT8
Checksum8 (
  VOID      *StartPtr,
  UINT32    Len
);

VOID
SaveOemDsdt (
  BOOLEAN   FullPatch,
  UINT8     OSType
);

VOID
SaveOemTables ();

UINT32
FixAny (
  UINT8     *Dsdt,
  UINT32    Len,
  UINT8     *ToFind,
  UINT32    LenTF,
  UINT8     *ToReplace,
  UINT32    LenTR
);

VOID
GetAcpiTablesList ();

EFI_STATUS
EventsInitialize (
  IN LOADER_ENTRY   *Entry
);

VOID
EFIAPI
ClosingEventAndLog (
  IN LOADER_ENTRY   *Entry
);

VOID
ReInitializeSettings ();

EFI_STATUS
SaveSettings ();

EFI_STATUS
PrePatchSmbios ();

VOID
PatchSmbios ();

VOID
FinalizeSmbios ();

EFI_STATUS
FixOwnership ();

EFI_STATUS
EFIAPI
LzvnDecode (
        UINT8   **Dst,
        UINTN   *DstSize,
  CONST UINT8   *Src,
        UINTN   SrcSize
);

EFI_STATUS
EFIAPI
LzvnEncode (
        UINT8   **Dst,
        UINTN   *DstSize,
  CONST UINT8   *Src,
        UINTN   SrcSize
);

UINT64
TimeDiff (
  UINT64  t0,
  UINT64  t1
);

BOOLEAN
IsPatchEnabled (
  CHAR8   *MatchOSEntry,
  CHAR8   *CurrOS
);

CHAR16  *
AddLoadOption (
  IN CHAR16   *LoadOptions,
  IN CHAR16   *LoadOption
);

CHAR16  *
RemoveLoadOption (
  IN CHAR16   *LoadOptions,
  IN CHAR16   *LoadOption
);

CHAR16  *
ToggleLoadOptions (
  IN  BOOLEAN   State,
  IN  CHAR16    *LoadOptions,
  IN  CHAR16    *LoadOption
);

BOOLEAN
IsHDMIAudio (
  EFI_HANDLE  PciDevHandle
);

//
// PlatformDriverOverride.c
//
/** Registers given PriorityDrivers (NULL terminated) to highest priority during connecting controllers.
 *  Does this by installing our EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL
 *  or by overriding existing EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL.GetDriver.
 */
VOID
RegisterDriversToHighestPriority (
  IN EFI_HANDLE   *PriorityDrivers
);

EFI_STATUS
LoadUserSettings (
  IN EFI_FILE   *RootDir,
  IN CHAR16     *ConfName,
  IN TagPtr     *Dict
);

VOID
CopyKernelAndKextPatches (
  IN OUT KERNEL_AND_KEXT_PATCHES   *Dst,
  IN     KERNEL_AND_KEXT_PATCHES   *Src
);

UINT8 GetOSTypeFromPath (
  IN  CHAR16  *Path
);

VOID  ScanThemes ();
VOID  ScanAcpi ();
VOID  ScanConfigs ();

MatchOSes *
GetStrArraySeparatedByChar (
  CHAR8   *Str,
  CHAR8   Sep
);

VOID
DeallocMatchOSes (
  MatchOSes   *MOS
);

VOID
EFIAPI
hehe ();

VOID
EFIAPI
hehe2 ();

VOID
LoadDrivers ();

#endif
