/* xmp-model.h - treeview model for XMP metadata
 *
 * Copyright (C) 2004, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef XMP_MODEL_H
#define XMP_MODEL_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XMPModel XMPModel;

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
  XMP_TYPE_UNKNOWN
} XMPType;

/* columns used in the GtkTreeStore model holding the XMP metadata */
typedef enum
{
  COL_XMP_NAME     = 0, /* G_TYPE_STRING */
  COL_XMP_VALUE,        /* G_TYPE_STRING */
  COL_XMP_VALUE_RAW,    /* G_TYPE_POINTER */
  COL_XMP_TYPE_XREF,    /* G_TYPE_POINTER */
  COL_XMP_WIDGET_XREF,  /* G_TYPE_POINTER */
  COL_XMP_EDITABLE,     /* G_TYPE_INT */
  COL_XMP_EDIT_ICON,    /* GDK_TYPE_PIXBUF */
  COL_XMP_VISIBLE,      /* G_TYPE_BOOLEAN */
  COL_XMP_WEIGHT,       /* G_TYPE_INT */
  COL_XMP_WEIGHT_SET,   /* G_TYPE_BOOLEAN */
  XMP_MODEL_NUM_COLUMNS
} XMPModelColumns;

/* special value for the COL_XMP_EDITABLE column.  not strictly boolean... */
#define XMP_AUTO_UPDATE 2

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

XMPModel     *xmp_model_new            (void);

void          xmp_model_free           (XMPModel     *xmp_model);

gboolean      xmp_model_is_empty       (XMPModel     *xmp_model);

gboolean      xmp_model_parse_buffer   (XMPModel     *xmp_model,
                                        const gchar  *buffer,
                                        gssize        buffer_length,
                                        gboolean      skip_other_data,
                                        GError      **error);

gboolean      xmp_model_parse_file     (XMPModel     *xmp_model,
                                        const gchar  *filename,
                                        GError      **error);

GtkTreeModel *xmp_model_get_tree_model (XMPModel     *xmp_model);

const gchar  *xmp_model_get_scalar_property (XMPModel    *xmp_model,
                                             const gchar *schema_name,
                                             const gchar *property_name);

gboolean      xmp_model_set_scalar_property (XMPModel    *xmp_model,
                                             const gchar *schema_name,
                                             const gchar *property_name,
                                             const gchar *property_value);


G_END_DECLS

#endif /* XMP_MODEL_H */
