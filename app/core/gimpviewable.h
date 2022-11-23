/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaviewable.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEWABLE_H__
#define __LIGMA_VIEWABLE_H__


#include "ligmaobject.h"


#define LIGMA_VIEWABLE_MAX_PREVIEW_SIZE 2048
#define LIGMA_VIEWABLE_MAX_POPUP_SIZE    256
#define LIGMA_VIEWABLE_MAX_BUTTON_SIZE    64
#define LIGMA_VIEWABLE_MAX_MENU_SIZE      48


#define LIGMA_TYPE_VIEWABLE            (ligma_viewable_get_type ())
#define LIGMA_VIEWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEWABLE, LigmaViewable))
#define LIGMA_VIEWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEWABLE, LigmaViewableClass))
#define LIGMA_IS_VIEWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VIEWABLE))
#define LIGMA_IS_VIEWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEWABLE))
#define LIGMA_VIEWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEWABLE, LigmaViewableClass))


typedef struct _LigmaViewableClass LigmaViewableClass;

struct _LigmaViewable
{
  LigmaObject  parent_instance;
};

struct _LigmaViewableClass
{
  LigmaObjectClass  parent_class;

  const gchar     *default_icon_name;
  const gchar     *name_changed_signal;
  gboolean         name_editable;

  /*  signals  */
  void            (* invalidate_preview) (LigmaViewable  *viewable);
  void            (* size_changed)       (LigmaViewable  *viewable);
  void            (* expanded_changed)   (LigmaViewable  *viewable);
  void            (* ancestry_changed)   (LigmaViewable  *viewable);

  /*  virtual functions  */
  gboolean        (* get_size)           (LigmaViewable  *viewable,
                                          gint          *width,
                                          gint          *height);
  void            (* get_preview_size)   (LigmaViewable  *viewable,
                                          gint           size,
                                          gboolean       is_popup,
                                          gboolean       dot_for_dot,
                                          gint          *width,
                                          gint          *height);
  gboolean        (* get_popup_size)     (LigmaViewable  *viewable,
                                          gint           width,
                                          gint           height,
                                          gboolean       dot_for_dot,
                                          gint          *popup_width,
                                          gint          *popup_height);
  LigmaTempBuf   * (* get_preview)        (LigmaViewable  *viewable,
                                          LigmaContext   *context,
                                          gint           width,
                                          gint           height);
  LigmaTempBuf   * (* get_new_preview)    (LigmaViewable  *viewable,
                                          LigmaContext   *context,
                                          gint           width,
                                          gint           height);
  GdkPixbuf     * (* get_pixbuf)         (LigmaViewable  *viewable,
                                          LigmaContext   *context,
                                          gint           width,
                                          gint           height);
  GdkPixbuf     * (* get_new_pixbuf)     (LigmaViewable  *viewable,
                                          LigmaContext   *context,
                                          gint           width,
                                          gint           height);
  gchar         * (* get_description)    (LigmaViewable  *viewable,
                                          gchar        **tooltip);

  gboolean        (* is_name_editable)   (LigmaViewable  *viewable);

  void            (* preview_freeze)     (LigmaViewable  *viewable);
  void            (* preview_thaw)       (LigmaViewable  *viewable);

  LigmaContainer * (* get_children)       (LigmaViewable  *viewable);

  void            (* set_expanded)       (LigmaViewable  *viewable,
                                          gboolean       expand);
  gboolean        (* get_expanded)       (LigmaViewable  *viewable);
};


GType           ligma_viewable_get_type           (void) G_GNUC_CONST;

void            ligma_viewable_invalidate_preview (LigmaViewable  *viewable);
void            ligma_viewable_size_changed       (LigmaViewable  *viewable);
void            ligma_viewable_expanded_changed   (LigmaViewable  *viewable);

void            ligma_viewable_calc_preview_size  (gint           aspect_width,
                                                  gint           aspect_height,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       dot_for_dot,
                                                  gdouble        xresolution,
                                                  gdouble        yresolution,
                                                  gint          *return_width,
                                                  gint          *return_height,
                                                  gboolean      *scaling_up);

gboolean        ligma_viewable_get_size           (LigmaViewable  *viewable,
                                                  gint          *width,
                                                  gint          *height);
void            ligma_viewable_get_preview_size   (LigmaViewable  *viewable,
                                                  gint           size,
                                                  gboolean       popup,
                                                  gboolean       dot_for_dot,
                                                  gint          *width,
                                                  gint          *height);
gboolean        ligma_viewable_get_popup_size     (LigmaViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       dot_for_dot,
                                                  gint          *popup_width,
                                                  gint          *popup_height);

LigmaTempBuf   * ligma_viewable_get_preview        (LigmaViewable  *viewable,
                                                  LigmaContext   *context,
                                                  gint           width,
                                                  gint           height);
LigmaTempBuf   * ligma_viewable_get_new_preview    (LigmaViewable  *viewable,
                                                  LigmaContext   *context,
                                                  gint           width,
                                                  gint           height);

LigmaTempBuf   * ligma_viewable_get_dummy_preview  (LigmaViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  const Babl    *format);

GdkPixbuf     * ligma_viewable_get_pixbuf         (LigmaViewable  *viewable,
                                                  LigmaContext   *context,
                                                  gint           width,
                                                  gint           height);
GdkPixbuf     * ligma_viewable_get_new_pixbuf     (LigmaViewable  *viewable,
                                                  LigmaContext   *context,
                                                  gint           width,
                                                  gint           height);

GdkPixbuf     * ligma_viewable_get_dummy_pixbuf   (LigmaViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       with_alpha);

gchar         * ligma_viewable_get_description    (LigmaViewable  *viewable,
                                                  gchar        **tooltip);

gboolean        ligma_viewable_is_name_editable   (LigmaViewable  *viewable);

const gchar   * ligma_viewable_get_icon_name      (LigmaViewable  *viewable);
void            ligma_viewable_set_icon_name      (LigmaViewable  *viewable,
                                                  const gchar   *icon_name);

void            ligma_viewable_preview_freeze     (LigmaViewable  *viewable);
void            ligma_viewable_preview_thaw       (LigmaViewable  *viewable);
gboolean        ligma_viewable_preview_is_frozen  (LigmaViewable  *viewable);

LigmaViewable  * ligma_viewable_get_parent         (LigmaViewable  *viewable);
void            ligma_viewable_set_parent         (LigmaViewable  *viewable,
                                                  LigmaViewable  *parent);

gint            ligma_viewable_get_depth          (LigmaViewable  *viewable);

LigmaContainer * ligma_viewable_get_children       (LigmaViewable  *viewable);

gboolean        ligma_viewable_get_expanded       (LigmaViewable  *viewable);
void            ligma_viewable_set_expanded       (LigmaViewable  *viewable,
                                                  gboolean       expanded);

gboolean        ligma_viewable_is_ancestor        (LigmaViewable  *ancestor,
                                                  LigmaViewable  *descendant);


#endif  /* __LIGMA_VIEWABLE_H__ */
