/* xmp-model.h - treeview model for XMP metadata
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

#ifndef XMP_MODEL_H
#define XMP_MODEL_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XMPModel XMPModel;

/* columns used in the GtkTreeStore model holding the XMP metadata */
typedef enum
{
  COL_XMP_NAME     = 0, /* G_TYPE_STRING   - name */
  COL_XMP_VALUE,        /* G_TYPE_STRING   - value as string (for viewing) */
  COL_XMP_VALUE_RAW,    /* G_TYPE_POINTER  - value as array (from parser) */
  COL_XMP_TYPE_XREF,    /* G_TYPE_POINTER  - XMPProperty or XMPSchema */
  COL_XMP_WIDGET_XREF,  /* G_TYPE_POINTER  - GtkWidget cross-reference */
  COL_XMP_EDITABLE,     /* G_TYPE_INT      - editable? */
  COL_XMP_EDIT_ICON,    /* GDK_TYPE_PIXBUF - edit icon */
  COL_XMP_VISIBLE,      /* G_TYPE_BOOLEAN  - visible? */
  COL_XMP_WEIGHT,       /* G_TYPE_INT      - font weight */
  COL_XMP_WEIGHT_SET,   /* G_TYPE_BOOLEAN  - font weight set? */
  XMP_MODEL_NUM_COLUMNS
} XMPModelColumns;

/* special value for the COL_XMP_EDITABLE column.  not strictly boolean... */
#define XMP_AUTO_UPDATE 2

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
