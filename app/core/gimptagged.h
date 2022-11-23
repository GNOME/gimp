/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmatagged.h
 * Copyright (C) 2008  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TAGGED_H__
#define __LIGMA_TAGGED_H__


#define LIGMA_TYPE_TAGGED (ligma_tagged_get_type ())
G_DECLARE_INTERFACE (LigmaTagged, ligma_tagged, LIGMA, TAGGED, GObject)


struct _LigmaTaggedInterface
{
  GTypeInterface base_iface;

  /*  signals            */
  void       (* tag_added)      (LigmaTagged *tagged,
                                 LigmaTag    *tag);
  void       (* tag_removed)    (LigmaTagged *tagged,
                                 LigmaTag    *tag);

  /*  virtual functions  */
  gboolean   (* add_tag)        (LigmaTagged *tagged,
                                 LigmaTag    *tag);
  gboolean   (* remove_tag)     (LigmaTagged *tagged,
                                 LigmaTag    *tag);
  GList    * (* get_tags)       (LigmaTagged *tagged);
  gchar    * (* get_identifier) (LigmaTagged *tagged);
  gchar    * (* get_checksum)   (LigmaTagged *tagged);
};


void       ligma_tagged_add_tag        (LigmaTagged *tagged,
                                       LigmaTag    *tag);
void       ligma_tagged_remove_tag     (LigmaTagged *tagged,
                                       LigmaTag    *tag);

void       ligma_tagged_set_tags       (LigmaTagged *tagged,
                                       GList      *tags);
GList    * ligma_tagged_get_tags       (LigmaTagged *tagged);

gchar    * ligma_tagged_get_identifier (LigmaTagged *tagged);
gchar    * ligma_tagged_get_checksum   (LigmaTagged *tagged);

gboolean   ligma_tagged_has_tag        (LigmaTagged *tagged,
                                       LigmaTag    *tag);


#endif  /* __LIGMA_TAGGED_H__ */
