/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpchoice.h
 * Copyright (C) 2023 Jehan
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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif


#ifndef __GIMP_CHOICE_H__
#define __GIMP_CHOICE_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_CHOICE (gimp_choice_get_type ())
G_DECLARE_FINAL_TYPE (GimpChoice, gimp_choice, GIMP, CHOICE, GObject)


GimpChoice    * gimp_choice_new               (void);
GimpChoice    * gimp_choice_new_with_values   (const gchar *nick,
                                               gint         id,
                                               const gchar *label,
                                               const gchar *help,
                                               ...) G_GNUC_NULL_TERMINATED;
void            gimp_choice_add               (GimpChoice  *choice,
                                               const gchar *nick,
                                               gint         id,
                                               const gchar *label,
                                               const gchar *help);

gboolean        gimp_choice_is_valid          (GimpChoice   *choice,
                                               const gchar  *nick);
GList         * gimp_choice_list_nicks        (GimpChoice   *choice);
gint            gimp_choice_get_id            (GimpChoice   *choice,
                                               const gchar  *nick);
const gchar   * gimp_choice_get_label         (GimpChoice   *choice,
                                               const gchar  *nick);
const gchar   * gimp_choice_get_help          (GimpChoice   *choice,
                                               const gchar  *nick);
gboolean        gimp_choice_get_documentation (GimpChoice   *choice,
                                               const gchar  *nick,
                                               const gchar **label,
                                               const gchar **help);

void            gimp_choice_set_sensitive     (GimpChoice   *choice,
                                               const gchar  *nick,
                                               gboolean      sensitive);


/*
 * GIMP_TYPE_PARAM_CHOICE
 */

#define GIMP_TYPE_PARAM_CHOICE           (gimp_param_choice_get_type ())
#define GIMP_IS_PARAM_SPEC_CHOICE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CHOICE))


GType        gimp_param_choice_get_type         (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_choice             (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 GimpChoice   *choice,
                                                 const gchar  *default_value,
                                                 GParamFlags   flags);

GimpChoice  * gimp_param_spec_choice_get_choice  (GParamSpec *pspec);
const gchar * gimp_param_spec_choice_get_default (GParamSpec *pspec);


G_END_DECLS

#endif /* __GIMP_CHOICE_H__ */
