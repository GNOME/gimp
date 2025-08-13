/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorProfileView
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_COLOR_PROFILE_VIEW_H__
#define __GIMP_COLOR_PROFILE_VIEW_H__

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_PROFILE_VIEW (gimp_color_profile_view_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorProfileView, gimp_color_profile_view, GIMP, COLOR_PROFILE_VIEW, GtkTextView)


GtkWidget * gimp_color_profile_view_new         (void);

void        gimp_color_profile_view_set_profile (GimpColorProfileView *view,
                                                 GimpColorProfile     *profile);
void        gimp_color_profile_view_set_error   (GimpColorProfileView *view,
                                                 const gchar          *message);

G_END_DECLS

#endif /* __GIMP_COLOR_PROFILE_VIEW_H__ */
