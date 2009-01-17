/* xmp-schemas.h - standard schemas defined in the XMP specifications
 *
 * Copyright (C) 2004, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef XMP_SCHEMAS_H
#define XMP_SCHEMAS_H

G_BEGIN_DECLS

/* known data types for XMP properties, as found in the XMP specification */
typedef enum
{
  XMP_TYPE_BOOLEAN,              /* TEXT */
  XMP_TYPE_DATE,                 /* TEXT */
  XMP_TYPE_DIMENSIONS,           /* STRUCTURE */
  XMP_TYPE_INTEGER,              /* TEXT */
  XMP_TYPE_INTEGER_SEQ,          /* ORDERED_LIST */
  XMP_TYPE_LANG_ALT,             /* ALT_LANG */
  XMP_TYPE_LOCALE_BAG,           /* UNORDERED_LIST */
  XMP_TYPE_REAL,                 /* TEXT */
  XMP_TYPE_MIME_TYPE,            /* TEXT */
  XMP_TYPE_TEXT,                 /* TEXT */
  XMP_TYPE_TEXT_BAG,             /* UNORDERED_LIST */
  XMP_TYPE_TEXT_SEQ,             /* ORDERED_LIST */
  XMP_TYPE_THUMBNAIL_ALT,        /* ALT_THUMBS */
  XMP_TYPE_URI,                  /* TEXT or RESOURCE (?) */
  XMP_TYPE_XPATH_BAG,            /* UNORDERED_LIST */
  XMP_TYPE_RESOURCE_EVENT_SEQ,   /* ORDERED_LIST */
  XMP_TYPE_RESOURCE_REF,         /* TEXT */
  XMP_TYPE_JOB_BAG,              /* UNORDERED_LIST */
  XMP_TYPE_RATIONAL,             /* TEXT */
  XMP_TYPE_RATIONAL_SEQ,         /* ORDERED_LIST */
  XMP_TYPE_GPS_COORDINATE,       /* (?) */
  XMP_TYPE_FLASH,                /* STRUCTURE */
  XMP_TYPE_OECF_SFR,             /* (?) */
  XMP_TYPE_CFA_PATTERN,          /* (?) */
  XMP_TYPE_DEVICE_SETTINGS,      /* (?) */
  XMP_TYPE_CONTACT_INFO,         /* STRUCTURE */
  XMP_TYPE_GENERIC_STRUCTURE,    /* STRUCTURE */
  XMP_TYPE_UNKNOWN
} XMPType;

/* XMP properties referenced in the tree via COL_XMP_TYPE_XREF (depth 2) */
typedef struct
{
  const gchar *name;
  XMPType      type;
  gboolean     editable;
} XMPProperty;

/* XMP schemas referenced in the tree via COL_XMP_TYPE_XREF (depth 1) */
typedef struct
{
  const gchar *uri;
  const gchar *prefix;
  const gchar *name;
  XMPProperty *properties;
} XMPSchema;

/* URIs of standard XMP schemas (as of January 2004) */
#define XMP_SCHEMA_DUBLIN_CORE "http://purl.org/dc/elements/1.1/"
#define XMP_SCHEMA_XMP_BASIC   "http://ns.adobe.com/xap/1.0/"
#define XMP_SCHEMA_XMP_RIGHTS  "http://ns.adobe.com/xap/1.0/rights/"
#define XMP_SCHEMA_XMP_MM      "http://ns.adobe.com/xap/1.0/mm/"
#define XMP_SCHEMA_XMP_BJ      "http://ns.adobe.com/xap/1.0/bj/"
#define XMP_SCHEMA_XMP_TPG     "http://ns.adobe.com/xap/1.0/t/pg/"
#define XMP_SCHEMA_PDF         "http://ns.adobe.com/pdf/1.3/"
#define XMP_SCHEMA_PHOTOSHOP   "http://ns.adobe.com/photoshop/1.0/"
#define XMP_SCHEMA_TIFF        "http://ns.adobe.com/tiff/1.0/"
#define XMP_SCHEMA_EXIF        "http://ns.adobe.com/exif/1.0/"
/* Additional schemas published in March 2005 */
#define XMP_SCHEMA_XMP_PLUS    "http://ns.adobe.com/xap/1.0/PLUS/"
#define XMP_SCHEMA_IPTC_CORE   "http://iptc.org/std/Iptc4xmpCore/1.0/xmlns/"

/* recommended prefixes for the schemas listed above */
#define XMP_PREFIX_DUBLIN_CORE "dc"
#define XMP_PREFIX_XMP_BASIC   "xmp"
#define XMP_PREFIX_XMP_RIGHTS  "xmpRights"
#define XMP_PREFIX_XMP_MM      "xmpMM"
#define XMP_PREFIX_XMP_BJ      "xmpBJ"
#define XMP_PREFIX_XMP_TPG     "xmpTPg"
#define XMP_PREFIX_PDF         "pdf"
#define XMP_PREFIX_PHOTOSHOP   "photoshop"
#define XMP_PREFIX_TIFF        "tiff"
#define XMP_PREFIX_EXIF        "exif"
#define XMP_PREFIX_XMP_PLUS    "xmpPLUS"
#define XMP_PREFIX_IPTC_CORE   "Iptc4xmpCore"

/* List of known XMP schemas and their properties */
extern XMPSchema * const xmp_schemas;

G_END_DECLS

#endif /* XMP_SCHEMAS_H */
