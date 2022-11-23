/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatag.h
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

#ifndef __LIGMA_TAG_H__
#define __LIGMA_TAG_H__


#include <glib-object.h>


#define LIGMA_TYPE_TAG            (ligma_tag_get_type ())
#define LIGMA_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TAG, LigmaTag))
#define LIGMA_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TAG, LigmaTagClass))
#define LIGMA_IS_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TAG))
#define LIGMA_IS_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TAG))
#define LIGMA_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TAG, LigmaTagClass))


typedef struct _LigmaTagClass    LigmaTagClass;

struct _LigmaTag
{
  GObject parent_instance;

  GQuark  tag;
  GQuark  collate_key;

  gboolean internal; /* Tags that are not serialized to disk */
};

struct _LigmaTagClass
{
  GObjectClass parent_class;
};

GType         ligma_tag_get_type            (void) G_GNUC_CONST;

LigmaTag     * ligma_tag_new                 (const gchar *tag_string);
LigmaTag     * ligma_tag_try_new             (const gchar *tag_string);

const gchar * ligma_tag_get_name            (LigmaTag     *tag);
guint         ligma_tag_get_hash            (LigmaTag     *tag);

gboolean      ligma_tag_get_internal        (LigmaTag     *tag);
void          ligma_tag_set_internal        (LigmaTag     *tag,
                                            gboolean     internal);

gboolean      ligma_tag_equals              (LigmaTag     *tag,
                                            LigmaTag     *other);
gint          ligma_tag_compare_func        (const void  *p1,
                                            const void  *p2);
gint          ligma_tag_compare_with_string (LigmaTag     *tag,
                                            const gchar *tag_string);
gboolean      ligma_tag_has_prefix          (LigmaTag     *tag,
                                            const gchar *prefix_string);
gchar       * ligma_tag_string_make_valid   (const gchar *tag_string);
gboolean      ligma_tag_is_tag_separator    (gunichar     c);

void          ligma_tag_or_null_ref         (LigmaTag     *tag_or_null);
void          ligma_tag_or_null_unref       (LigmaTag     *tag_or_null);


#endif /* __LIGMA_TAG_H__ */
