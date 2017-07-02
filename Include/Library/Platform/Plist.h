/*
Headers collection for procedures
*/

#ifndef __REFIT_PLIST_H__
#define __REFIT_PLIST_H__

/* XML Tags */
typedef enum {
  kTagTypeNone,
  kTagTypeDict,
  kTagTypeKey,
  kTagTypeString,
  kTagTypeInteger,
  kTagTypeData,
  kTagTypeDate,
  kTagTypeFalse,
  kTagTypeTrue,
  kTagTypeArray
} TAG_TYPE;

#define kXMLTagPList      "plist"
#define kXMLTagDict       "dict"
#define kXMLTagKey        "key"
#define kXMLTagString     "string"
#define kXMLTagInteger    "integer"
#define kXMLTagData       "data"
#define kXMLTagDate       "date"
#define kXMLTagFalse      "false"
#define kXMLTagTrue       "true"
#define kXMLTagArray      "array"
#define kXMLTagReference  "reference"
#define kXMLTagID         "ID="
#define kXMLTagIDREF      "IDREF="
#define kXMLTagSIZE       "size="

typedef struct Symbol {
          UINTN   refCount;
          CHAR8   *string;
  struct  Symbol  *next;
} Symbol , *SymbolPtr;

typedef struct sREF {
          CHAR8   *string;
          INT32   id;
          INTN    integer;
          INTN    size;
  struct  sREF    *next;
} sREF;

typedef struct {
  UINTN   type;
  CHAR8   *string;
  INTN    integer;
  UINT8   *data;
  UINTN   size;
  VOID    *tag;
  VOID    *tagNext;
  UINTN   offset;
  UINTN   taglen;
  INT32   ref;
  INT32   id;
  sREF    *ref_strings;
  sREF    *ref_integer;
} TagStruct, *TagPtr;

CHAR8 *
XMLDecode (
  CHAR8   *src
);

EFI_STATUS
ParseXML (
  CHAR8   *Buffer,
  UINT32  BufSize,
  TagPtr  *Dict
);

TagPtr
GetProperty (
  TagPtr  Dict,
  CHAR8   *Key
);

EFI_STATUS
GetRefInteger (
   IN TagPtr  Tag,
   IN INT32   Id,
  OUT CHAR8   **Val,
  OUT INTN    *DecVal,
  OUT INTN    *Size
);

EFI_STATUS
GetRefString (
  IN  TagPtr  Tag,
  IN  INT32   Id,
  OUT CHAR8   **Val,
  OUT INTN    *Size
);

VOID
DumpTag (
  TagPtr  Tag,
  INT32   Depth
);

VOID
FreeTag (
  TagPtr Tag
);

INTN
GetTagCount (
  TagPtr Dict
);

EFI_STATUS
GetElement (
  TagPtr    Dict,
  INTN      Id,
  INTN      Count,
  TagPtr    *Dict1
);

#endif
