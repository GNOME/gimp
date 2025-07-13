/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptag.h
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

#pragma once


#define GIMP_TYPE_TAG            (gimp_tag_get_type ())
#define GIMP_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAG, GimpTag))
#define GIMP_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TAG, GimpTagClass))
#define GIMP_IS_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAG))
#define GIMP_IS_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TAG))
#define GIMP_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TAG, GimpTagClass))


typedef struct _GimpTagClass    GimpTagClass;

struct _GimpTag
{
  GObject parent_instance;

  GQuark  tag;
  GQuark  collate_key;

  gboolean internal; /* Tags that are not serialized to disk */
};

struct _GimpTagClass
{
  GObjectClass parent_class;
};

GType         gimp_tag_get_type            (void) G_GNUC_CONST;

GimpTag     * gimp_tag_new                 (const gchar *tag_string);
GimpTag     * gimp_tag_try_new             (const gchar *tag_string);

const gchar * gimp_tag_get_name            (GimpTag     *tag);
guint         gimp_tag_get_hash            (GimpTag     *tag);

gboolean      gimp_tag_get_internal        (GimpTag     *tag);
void          gimp_tag_set_internal        (GimpTag     *tag,
                                            gboolean     internal);

gboolean      gimp_tag_equals              (GimpTag     *tag,
                                            GimpTag     *other);
gint          gimp_tag_compare_func        (const void  *p1,
                                            const void  *p2);
gint          gimp_tag_compare_with_string (GimpTag     *tag,
                                            const gchar *tag_string);
gboolean      gimp_tag_has_prefix          (GimpTag     *tag,
                                            const gchar *prefix_string);
gchar       * gimp_tag_string_make_valid   (const gchar *tag_string);
gboolean      gimp_tag_is_tag_separator    (gunichar     c);

void          gimp_tag_or_null_ref         (GimpTag     *tag_or_null);
void          gimp_tag_or_null_unref       (GimpTag     *tag_or_null);
