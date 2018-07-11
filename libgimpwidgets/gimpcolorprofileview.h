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


#define GIMP_TYPE_COLOR_PROFILE_VIEW            (gimp_color_profile_view_get_type ())
#define GIMP_COLOR_PROFILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_PROFILE_VIEW, GimpColorProfileView))
#define GIMP_COLOR_PROFILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_PROFILE_VIEW, GimpColorProfileViewClass))
#define GIMP_IS_COLOR_PROFILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_PROFILE_VIEW))
#define GIMP_IS_COLOR_PROFILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_PROFILE_VIEW))
#define GIMP_COLOR_PROFILE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_PROFILE_VIEW, GimpColorProfileViewClass))


typedef struct _GimpColorProfileViewClass   GimpColorProfileViewClass;
typedef struct _GimpColorProfileViewPrivate GimpColorProfileViewPrivate;

struct _GimpColorProfileView
{
  GtkTextView                  parent_instance;

  GimpColorProfileViewPrivate *priv;
};

struct _GimpColorProfileViewClass
{
  GtkTextViewClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_color_profile_view_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_color_profile_view_new         (void);

void        gimp_color_profile_view_set_profile (GimpColorProfileView *view,
                                                 GimpColorProfile     *profile);
void        gimp_color_profile_view_set_error   (GimpColorProfileView *view,
                                                 const gchar          *message);

G_END_DECLS

#endif /* __GIMP_COLOR_PROFILE_VIEW_H__ */
