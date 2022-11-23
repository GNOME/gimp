/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaview.h
 * Copyright (C) 2001-2006 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEW_H__
#define __LIGMA_VIEW_H__


#define LIGMA_TYPE_VIEW            (ligma_view_get_type ())
#define LIGMA_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW, LigmaView))
#define LIGMA_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW, LigmaViewClass))
#define LIGMA_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW))
#define LIGMA_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW))
#define LIGMA_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW, LigmaViewClass))


typedef struct _LigmaViewClass  LigmaViewClass;

struct _LigmaView
{
  GtkWidget         parent_instance;

  GdkWindow        *event_window;

  LigmaViewable     *viewable;
  LigmaViewRenderer *renderer;

  guint             clickable : 1;
  guint             eat_button_events : 1;
  guint             show_popup : 1;
  guint             expand : 1;

  /*< private >*/
  guint             in_button : 1;
  guint             has_grab : 1;
  GdkModifierType   press_state;
};

struct _LigmaViewClass
{
  GtkWidgetClass  parent_class;

  /*  signals  */
  void        (* set_viewable)   (LigmaView        *view,
                                  LigmaViewable    *old_viewable,
                                  LigmaViewable    *new_viewable);
  void        (* clicked)        (LigmaView        *view,
                                  GdkModifierType  modifier_state);
  void        (* double_clicked) (LigmaView        *view);
  void        (* context)        (LigmaView        *view);
};


GType          ligma_view_get_type          (void) G_GNUC_CONST;

GtkWidget    * ligma_view_new               (LigmaContext   *context,
                                            LigmaViewable  *viewable,
                                            gint           size,
                                            gint           border_width,
                                            gboolean       is_popup);
GtkWidget    * ligma_view_new_full          (LigmaContext   *context,
                                            LigmaViewable  *viewable,
                                            gint           width,
                                            gint           height,
                                            gint           border_width,
                                            gboolean       is_popup,
                                            gboolean       clickable,
                                            gboolean       show_popup);
GtkWidget    * ligma_view_new_by_types      (LigmaContext   *context,
                                            GType          view_type,
                                            GType          viewable_type,
                                            gint           size,
                                            gint           border_width,
                                            gboolean       is_popup);
GtkWidget    * ligma_view_new_full_by_types (LigmaContext   *context,
                                            GType          view_type,
                                            GType          viewable_type,
                                            gint           width,
                                            gint           height,
                                            gint           border_width,
                                            gboolean       is_popup,
                                            gboolean       clickable,
                                            gboolean       show_popup);

LigmaViewable * ligma_view_get_viewable      (LigmaView      *view);
void           ligma_view_set_viewable      (LigmaView      *view,
                                            LigmaViewable  *viewable);
void           ligma_view_set_expand        (LigmaView      *view,
                                            gboolean       expand);


#endif /* __LIGMA_VIEW_H__ */
