/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_UTILS_H__
#define __GIMP_UTILS_H__

G_BEGIN_DECLS


gchar         * gimp_utf8_strtrim            (const gchar  *str,
                                              gint          max_chars) G_GNUC_MALLOC;
gchar         * gimp_any_to_utf8             (const gchar  *str,
                                              gssize        len,
                                              const gchar  *warning_format,
                                              ...) G_GNUC_PRINTF (3, 4) G_GNUC_MALLOC;
const gchar   * gimp_filename_to_utf8        (const gchar  *filename);

const gchar   * gimp_file_get_utf8_name      (GFile        *file);
gboolean        gimp_file_has_extension      (GFile        *file,
                                              const gchar  *extension);

gchar         * gimp_strip_uline             (const gchar  *str) G_GNUC_MALLOC;
gchar         * gimp_escape_uline            (const gchar  *str) G_GNUC_MALLOC;

gchar         * gimp_canonicalize_identifier (const gchar  *identifier) G_GNUC_MALLOC;

GimpEnumDesc  * gimp_enum_get_desc           (GEnumClass   *enum_class,
                                              gint          value);
gboolean        gimp_enum_get_value          (GType         enum_type,
                                              gint          value,
                                              const gchar **value_name,
                                              const gchar **value_nick,
                                              const gchar **value_desc,
                                              const gchar **value_help);
const gchar   * gimp_enum_value_get_desc     (GEnumClass   *enum_class,
                                              GEnumValue   *enum_value);
const gchar   * gimp_enum_value_get_help     (GEnumClass   *enum_class,
                                              GEnumValue   *enum_value);

GimpFlagsDesc * gimp_flags_get_first_desc    (GFlagsClass  *flags_class,
                                              guint         value);
gboolean        gimp_flags_get_first_value   (GType         flags_type,
                                              guint         value,
                                              const gchar **value_name,
                                              const gchar **value_nick,
                                              const gchar **value_desc,
                                              const gchar **value_help);
const gchar   * gimp_flags_value_get_desc    (GFlagsClass  *flags_class,
                                              GFlagsValue  *flags_value);
const gchar   * gimp_flags_value_get_help    (GFlagsClass  *flags_class,
                                              GFlagsValue  *flags_value);


G_END_DECLS

#endif  /* __GIMP_UTILS_H__ */
