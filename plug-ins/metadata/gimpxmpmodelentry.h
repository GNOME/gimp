/* GimpXmpModelEntry.h - custom entry widget linked to the xmp model
 *
 * Copyright (C) 2009, Róman Joost <romanofski@gimp.org>
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

#ifndef __GIMP_XMP_MODEL_ENTRY_H__
#define __GIMP_XMP_MODEL_ENTRY_H__

G_BEGIN_DECLS

#define GIMP_TYPE_XMP_MODEL_ENTRY               (gimp_xmp_model_entry_get_type ())
#define GIMP_XMP_MODEL_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_XMP_MODEL_ENTRY, GimpXmpModelEntry))
#define GIMP_XMP_MODEL_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_XMP_MODEL_ENTRY, XMPModelClass))
#define GIMP_IS_XMP_MODEL_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_XMP_MODEL_ENTRY))
#define GIMP_IS_XMP_MODEL_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_XMP_MODEL_ENTRY))
#define GIMP_XMP_MODEL_ENTRY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_XMP_MODEL_ENTRY, XMPModelClass))


typedef struct _GimpXmpModelEntry       GimpXmpModelEntry;
typedef struct _GimpXmpModelEntryClass  GimpXmpModelEntryClass;


struct _GimpXmpModelEntryClass
{
  GtkEntryClass parent_class;
};

struct _GimpXmpModelEntry
{
  GtkEntry   parent_instance;
  gpointer   priv;
};


GType       gimp_xmp_model_entry_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_xmp_model_entry_new      (const gchar *schema_uri,
                                           const gchar *property,
                                           XMPModel    *xmp_model);


G_END_DECLS

#endif /* __GIMP_XMP_MODEL_ENTRY_H__ */
