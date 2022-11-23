/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaColorProfileView
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_PROFILE_VIEW_H__
#define __LIGMA_COLOR_PROFILE_VIEW_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_PROFILE_VIEW            (ligma_color_profile_view_get_type ())
#define LIGMA_COLOR_PROFILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_PROFILE_VIEW, LigmaColorProfileView))
#define LIGMA_COLOR_PROFILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_PROFILE_VIEW, LigmaColorProfileViewClass))
#define LIGMA_IS_COLOR_PROFILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_PROFILE_VIEW))
#define LIGMA_IS_COLOR_PROFILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_PROFILE_VIEW))
#define LIGMA_COLOR_PROFILE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_PROFILE_VIEW, LigmaColorProfileViewClass))


typedef struct _LigmaColorProfileViewClass   LigmaColorProfileViewClass;
typedef struct _LigmaColorProfileViewPrivate LigmaColorProfileViewPrivate;

struct _LigmaColorProfileView
{
  GtkTextView                  parent_instance;

  LigmaColorProfileViewPrivate *priv;
};

struct _LigmaColorProfileViewClass
{
  GtkTextViewClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_color_profile_view_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_color_profile_view_new         (void);

void        ligma_color_profile_view_set_profile (LigmaColorProfileView *view,
                                                 LigmaColorProfile     *profile);
void        ligma_color_profile_view_set_error   (LigmaColorProfileView *view,
                                                 const gchar          *message);

G_END_DECLS

#endif /* __LIGMA_COLOR_PROFILE_VIEW_H__ */
