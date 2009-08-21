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

G_BEGIN_DECLS

#include <glib-object.h>

#define GIMP_TYPE_XMP_MODEL             (xmp_model_get_type ())
#define XMP_MODEL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_XMP_MODEL, XMPModel))
#define XMP_MODEL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_XMP_MODEL, XMPModelClass))
#define GIMP_IS_XMP_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_XMP_MODEL))
#define GIMP_IS_XMP_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_XMP_MODEL))
#define GIMP_XMP_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_XMP_MODEL, XMPModelClass))


typedef struct _XMPModel        XMPModel;
typedef struct _XMPModelClass   XMPModelClass;

/* The main part of the XMPModel structure is the GtkTreeStore in
 * which all references to XMP properties are stored.  In the tree,
 * the elements at the root level are the schemas (namespaces) and the
 * children of the schemas are the XMP properties.
 *
 * If the XMP file contains a schema that is not part of the XMP
 * specification or a known extension (e.g., IPTC Core), it will be
 * included in the custom_schemas list and the corresponding element
 * in the tree will get a reference to that list element instead of a
 * reference to one of the static schema definitions found in
 * xmp-schemas.c.  Same for custom properties inside a known or custom
 * schema.
 */
struct _XMPModel
{
  GtkTreeStore  parent_instance;

  GSList       *custom_schemas;
  GSList       *custom_properties;

  XMPSchema    *cached_schema;
  GtkTreeIter   cached_schema_iter;
};

struct _XMPModelClass
{
  GtkTreeStoreClass  parent_class;

  void (* property_changed) (XMPModel       *xmp_model,
                             XMPSchema      *schema,
                             GtkTreeIter    *iter);
};

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

GType         xmp_model_get_type            (void) G_GNUC_CONST;

XMPModel     *xmp_model_new                 (void);

void          xmp_model_free                (XMPModel     *xmp_model);

gboolean      xmp_model_is_empty            (XMPModel     *xmp_model);

gboolean      xmp_model_parse_buffer        (XMPModel     *xmp_model,
                                             const gchar  *buffer,
                                             gssize        buffer_length,
                                             gboolean      skip_other_data,
                                             GError      **error);

gboolean      xmp_model_parse_file          (XMPModel     *xmp_model,
                                             const gchar  *filename,
                                             GError      **error);

GtkTreeModel *xmp_model_get_tree_model      (XMPModel     *xmp_model);

const gchar  *xmp_model_get_scalar_property (XMPModel    *xmp_model,
                                             const gchar *schema_name,
                                             const gchar *property_name);

gboolean      xmp_model_set_scalar_property (XMPModel    *xmp_model,
                                             const gchar *schema_name,
                                             const gchar *property_name,
                                             const gchar *property_value);

/* Signals */
void          xmp_model_property_changed    (XMPModel     *xmp_model,
                                             XMPSchema    *schema,
                                             GtkTreeIter  *iter);

G_END_DECLS

#endif /* XMP_MODEL_H */
