/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimptagged.h
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TAGGED_H__
#define __GIMP_TAGGED_H__


#define GIMP_TYPE_TAGGED               (gimp_tagged_get_type ())
#define GIMP_IS_TAGGED(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAGGED))
#define GIMP_TAGGED(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAGGED, GimpTagged))
#define GIMP_TAGGED_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_TAGGED, GimpTaggedInterface))


typedef struct _GimpTaggedInterface GimpTaggedInterface;

struct _GimpTaggedInterface
{
  GTypeInterface base_iface;

  /*  signals            */
  void       (* tag_added)      (GimpTagged *tagged,
                                 GimpTag    *tag);
  void       (* tag_removed)    (GimpTagged *tagged,
                                 GimpTag    *tag);

  /*  virtual functions  */
  gboolean   (* add_tag)        (GimpTagged *tagged,
                                 GimpTag    *tag);
  gboolean   (* remove_tag)     (GimpTagged *tagged,
                                 GimpTag    *tag);
  GList    * (* get_tags)       (GimpTagged *tagged);
  gchar    * (* get_identifier) (GimpTagged *tagged);
  gchar    * (* get_checksum)   (GimpTagged *tagged);
};


GType      gimp_tagged_get_type       (void) G_GNUC_CONST;

void       gimp_tagged_add_tag        (GimpTagged *tagged,
                                       GimpTag    *tag);
void       gimp_tagged_remove_tag     (GimpTagged *tagged,
                                       GimpTag    *tag);

void       gimp_tagged_set_tags       (GimpTagged *tagged,
                                       GList      *tags);
GList    * gimp_tagged_get_tags       (GimpTagged *tagged);

gchar    * gimp_tagged_get_identifier (GimpTagged *tagged);
gchar    * gimp_tagged_get_checksum   (GimpTagged *tagged);

gboolean   gimp_tagged_has_tag        (GimpTagged *tagged,
                                       GimpTag    *tag);


#endif  /* __GIMP_TAGGED_H__ */
