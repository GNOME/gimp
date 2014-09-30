/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpattributes.h
 * Copyright (C) 2014  Hartmut Kuhse <hatti@gimp.org>
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


#ifndef __GIMPATTRIBUTES_H__
#define __GIMPATTRIBUTES_H__

G_BEGIN_DECLS

#define GIMP_TYPE_ATTRIBUTES            (gimp_attributes_get_type ())
#define GIMP_ATTRIBUTES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ATTRIBUTES, GimpAttributes))
#define GIMP_ATTRIBUTES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ATTRIBUTES, GimpAttributesClass))
#define GIMP_IS_ATTRIBUTES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ATTRIBUTES))
#define GIMP_IS_ATTRIBUTES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ATTRIBUTES))
#define GIMP_ATTRIBUTES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ATTRIBUTES, GimpAttributesClass))
#define GIMP_ATTRIBUTES_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GIMP_TYPE_ATTRIBUTES, GimpAttributesPrivate))


GType                                     gimp_attributes_get_type                                   (void) G_GNUC_CONST;
GimpAttributes *                          gimp_attributes_new                                        (void);
gint                                      gimp_attributes_size                                       (GimpAttributes      *attributes);
GimpAttributes *                          gimp_attributes_duplicate                                  (GimpAttributes      *attributes);
GHashTable *                              gimp_attributes_get_table                                  (GimpAttributes      *attributes);
void                                      gimp_attributes_add_attribute                              (GimpAttributes      *attributes,
                                                                                                      GimpAttribute       *attribute);
GimpAttribute *                           gimp_attributes_get_attribute                              (GimpAttributes      *attributes,
                                                                                                      const gchar         *name);
gboolean                                  gimp_attributes_remove_attribute                           (GimpAttributes      *attributes,
                                                                                                      const gchar         *name);
gboolean                                  gimp_attributes_has_attribute                              (GimpAttributes      *attributes,
                                                                                                      const gchar         *name);
gboolean                                  gimp_attributes_new_attribute                              (GimpAttributes      *attributes,
                                                                                                      const gchar         *name,
                                                                                                      gchar               *value,
                                                                                                      GimpAttributeValueType
                                                                                                                           type);

GimpAttributes *                          gimp_attributes_from_metadata                              (GimpAttributes      *attributes,
                                                                                                      GimpMetadata        *metadata);
void                                      gimp_attributes_to_metadata                                (GimpAttributes      *attributes,
                                                                                                      GimpMetadata        *metadata,
                                                                                                      const gchar         *mime_type);
const gchar *                             gimp_attributes_to_xmp_packet                              (GimpAttributes      *attributes,
                                                                                                      const gchar         *mime_type);
gchar *                                   gimp_attributes_serialize                                  (GimpAttributes      *attributes);
GimpAttributes *                          gimp_attributes_deserialize                                (const gchar         *xml);
gboolean                                  gimp_attributes_has_tag_type                               (GimpAttributes      *attributes,
                                                                                                      GimpAttributeTagType tag_type);
void                                      gimp_attributes_print                                      (GimpAttributes      *attributes);




G_END_DECLS

#endif
