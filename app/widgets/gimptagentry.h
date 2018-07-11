/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagentry.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#ifndef __GIMP_TAG_ENTRY_H__
#define __GIMP_TAG_ENTRY_H__


#define GIMP_TYPE_TAG_ENTRY            (gimp_tag_entry_get_type ())
#define GIMP_TAG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAG_ENTRY, GimpTagEntry))
#define GIMP_TAG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TAG_ENTRY, GimpTagEntryClass))
#define GIMP_IS_TAG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAG_ENTRY))
#define GIMP_IS_TAG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TAG_ENTRY))
#define GIMP_TAG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TAG_ENTRY, GimpTagEntryClass))


typedef struct _GimpTagEntryClass  GimpTagEntryClass;

struct _GimpTagEntry
{
  GtkEntry             parent_instance;

  GimpTaggedContainer *container;

  /* mask describes the meaning of each char in GimpTagEntry.
   * It is maintained automatically on insert-text and delete-text
   * events. If manual mask modification is desired, then
   * suppress_mask_update must be increased before calling any
   * function changing entry contents.
   * Meaning of mask chars:
   * u - undefined / unknown (just typed unparsed text)
   * t - tag
   * s - separator
   * w - whitespace.
   */
  GString             *mask;
  GList               *selected_items;
  GList               *common_tags;
  GList               *recent_list;
  gint                 tab_completion_index;
  gint                 internal_operation;
  gint                 suppress_mask_update;
  gint                 suppress_tag_query;
  GimpTagEntryMode     mode;
  gboolean             description_shown;
  gboolean             has_invalid_tags;
  guint                tag_query_idle_id;
};

struct _GimpTagEntryClass
{
  GtkEntryClass   parent_class;
};


GType          gimp_tag_entry_get_type           (void) G_GNUC_CONST;

GtkWidget    * gimp_tag_entry_new                (GimpTaggedContainer *container,
                                                  GimpTagEntryMode     mode);

void           gimp_tag_entry_set_selected_items (GimpTagEntry        *entry,
                                                  GList               *items);
gchar       ** gimp_tag_entry_parse_tags         (GimpTagEntry        *entry);
void           gimp_tag_entry_set_tag_string     (GimpTagEntry        *entry,
                                                  const gchar         *tag_string);

const gchar  * gimp_tag_entry_get_separator      (void);


#endif  /*  __GIMP_TAG_ENTRY_H__  */
