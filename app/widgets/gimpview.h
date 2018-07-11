/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpview.h
 * Copyright (C) 2001-2006 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEW_H__
#define __GIMP_VIEW_H__


#define GIMP_TYPE_VIEW            (gimp_view_get_type ())
#define GIMP_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW, GimpView))
#define GIMP_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW, GimpViewClass))
#define GIMP_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW))
#define GIMP_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW))
#define GIMP_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW, GimpViewClass))


typedef struct _GimpViewClass  GimpViewClass;

struct _GimpView
{
  GtkWidget         parent_instance;

  GdkWindow        *event_window;

  GimpViewable     *viewable;
  GimpViewRenderer *renderer;

  guint             clickable : 1;
  guint             eat_button_events : 1;
  guint             show_popup : 1;
  guint             expand : 1;

  /*< private >*/
  guint             in_button : 1;
  guint             has_grab : 1;
  GdkModifierType   press_state;
};

struct _GimpViewClass
{
  GtkWidgetClass  parent_class;

  /*  signals  */
  void        (* set_viewable)   (GimpView        *view,
                                  GimpViewable    *old_viewable,
                                  GimpViewable    *new_viewable);
  void        (* clicked)        (GimpView        *view,
                                  GdkModifierType  modifier_state);
  void        (* double_clicked) (GimpView        *view);
  void        (* context)        (GimpView        *view);
};


GType          gimp_view_get_type          (void) G_GNUC_CONST;

GtkWidget    * gimp_view_new               (GimpContext   *context,
                                            GimpViewable  *viewable,
                                            gint           size,
                                            gint           border_width,
                                            gboolean       is_popup);
GtkWidget    * gimp_view_new_full          (GimpContext   *context,
                                            GimpViewable  *viewable,
                                            gint           width,
                                            gint           height,
                                            gint           border_width,
                                            gboolean       is_popup,
                                            gboolean       clickable,
                                            gboolean       show_popup);
GtkWidget    * gimp_view_new_by_types      (GimpContext   *context,
                                            GType          view_type,
                                            GType          viewable_type,
                                            gint           size,
                                            gint           border_width,
                                            gboolean       is_popup);
GtkWidget    * gimp_view_new_full_by_types (GimpContext   *context,
                                            GType          view_type,
                                            GType          viewable_type,
                                            gint           width,
                                            gint           height,
                                            gint           border_width,
                                            gboolean       is_popup,
                                            gboolean       clickable,
                                            gboolean       show_popup);

GimpViewable * gimp_view_get_viewable      (GimpView      *view);
void           gimp_view_set_viewable      (GimpView      *view,
                                            GimpViewable  *viewable);
void           gimp_view_set_expand        (GimpView      *view,
                                            gboolean       expand);


#endif /* __GIMP_VIEW_H__ */
