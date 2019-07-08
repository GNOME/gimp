/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpviewable.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEWABLE_H__
#define __GIMP_VIEWABLE_H__


#include "gimpobject.h"


#define GIMP_VIEWABLE_MAX_PREVIEW_SIZE 2048
#define GIMP_VIEWABLE_MAX_POPUP_SIZE    256
#define GIMP_VIEWABLE_MAX_BUTTON_SIZE    64
#define GIMP_VIEWABLE_MAX_MENU_SIZE      48


#define GIMP_TYPE_VIEWABLE            (gimp_viewable_get_type ())
#define GIMP_VIEWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEWABLE, GimpViewable))
#define GIMP_VIEWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEWABLE, GimpViewableClass))
#define GIMP_IS_VIEWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VIEWABLE))
#define GIMP_IS_VIEWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEWABLE))
#define GIMP_VIEWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEWABLE, GimpViewableClass))


typedef struct _GimpViewableClass GimpViewableClass;

struct _GimpViewable
{
  GimpObject  parent_instance;
};

struct _GimpViewableClass
{
  GimpObjectClass  parent_class;

  const gchar     *default_icon_name;
  const gchar     *name_changed_signal;
  gboolean         name_editable;

  /*  signals  */
  void            (* invalidate_preview) (GimpViewable  *viewable);
  void            (* size_changed)       (GimpViewable  *viewable);
  void            (* expanded_changed)   (GimpViewable  *viewable);
  void            (* ancestry_changed)   (GimpViewable  *viewable);

  /*  virtual functions  */
  gboolean        (* get_size)           (GimpViewable  *viewable,
                                          gint          *width,
                                          gint          *height);
  void            (* get_preview_size)   (GimpViewable  *viewable,
                                          gint           size,
                                          gboolean       is_popup,
                                          gboolean       dot_for_dot,
                                          gint          *width,
                                          gint          *height);
  gboolean        (* get_popup_size)     (GimpViewable  *viewable,
                                          gint           width,
                                          gint           height,
                                          gboolean       dot_for_dot,
                                          gint          *popup_width,
                                          gint          *popup_height);
  GimpTempBuf   * (* get_preview)        (GimpViewable  *viewable,
                                          GimpContext   *context,
                                          gint           width,
                                          gint           height);
  GimpTempBuf   * (* get_new_preview)    (GimpViewable  *viewable,
                                          GimpContext   *context,
                                          gint           width,
                                          gint           height);
  GdkPixbuf     * (* get_pixbuf)         (GimpViewable  *viewable,
                                          GimpContext   *context,
                                          gint           width,
                                          gint           height);
  GdkPixbuf     * (* get_new_pixbuf)     (GimpViewable  *viewable,
                                          GimpContext   *context,
                                          gint           width,
                                          gint           height);
  gchar         * (* get_description)    (GimpViewable  *viewable,
                                          gchar        **tooltip);

  gboolean        (* is_name_editable)   (GimpViewable  *viewable);

  void            (* preview_freeze)     (GimpViewable  *viewable);
  void            (* preview_thaw)       (GimpViewable  *viewable);

  GimpContainer * (* get_children)       (GimpViewable  *viewable);

  void            (* set_expanded)       (GimpViewable  *viewable,
                                          gboolean       expand);
  gboolean        (* get_expanded)       (GimpViewable  *viewable);
};


GType           gimp_viewable_get_type           (void) G_GNUC_CONST;

void            gimp_viewable_invalidate_preview (GimpViewable  *viewable);
void            gimp_viewable_size_changed       (GimpViewable  *viewable);
void            gimp_viewable_expanded_changed   (GimpViewable  *viewable);

void            gimp_viewable_calc_preview_size  (gint           aspect_width,
                                                  gint           aspect_height,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       dot_for_dot,
                                                  gdouble        xresolution,
                                                  gdouble        yresolution,
                                                  gint          *return_width,
                                                  gint          *return_height,
                                                  gboolean      *scaling_up);

gboolean        gimp_viewable_get_size           (GimpViewable  *viewable,
                                                  gint          *width,
                                                  gint          *height);
void            gimp_viewable_get_preview_size   (GimpViewable  *viewable,
                                                  gint           size,
                                                  gboolean       popup,
                                                  gboolean       dot_for_dot,
                                                  gint          *width,
                                                  gint          *height);
gboolean        gimp_viewable_get_popup_size     (GimpViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       dot_for_dot,
                                                  gint          *popup_width,
                                                  gint          *popup_height);

GimpTempBuf   * gimp_viewable_get_preview        (GimpViewable  *viewable,
                                                  GimpContext   *context,
                                                  gint           width,
                                                  gint           height);
GimpTempBuf   * gimp_viewable_get_new_preview    (GimpViewable  *viewable,
                                                  GimpContext   *context,
                                                  gint           width,
                                                  gint           height);

GimpTempBuf   * gimp_viewable_get_dummy_preview  (GimpViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  const Babl    *format);

GdkPixbuf     * gimp_viewable_get_pixbuf         (GimpViewable  *viewable,
                                                  GimpContext   *context,
                                                  gint           width,
                                                  gint           height);
GdkPixbuf     * gimp_viewable_get_new_pixbuf     (GimpViewable  *viewable,
                                                  GimpContext   *context,
                                                  gint           width,
                                                  gint           height);

GdkPixbuf     * gimp_viewable_get_dummy_pixbuf   (GimpViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       with_alpha);

gchar         * gimp_viewable_get_description    (GimpViewable  *viewable,
                                                  gchar        **tooltip);

gboolean        gimp_viewable_is_name_editable   (GimpViewable  *viewable);

const gchar   * gimp_viewable_get_icon_name      (GimpViewable  *viewable);
void            gimp_viewable_set_icon_name      (GimpViewable  *viewable,
                                                  const gchar   *icon_name);

void            gimp_viewable_preview_freeze     (GimpViewable  *viewable);
void            gimp_viewable_preview_thaw       (GimpViewable  *viewable);
gboolean        gimp_viewable_preview_is_frozen  (GimpViewable  *viewable);

GimpViewable  * gimp_viewable_get_parent         (GimpViewable  *viewable);
void            gimp_viewable_set_parent         (GimpViewable  *viewable,
                                                  GimpViewable  *parent);

gint            gimp_viewable_get_depth          (GimpViewable  *viewable);

GimpContainer * gimp_viewable_get_children       (GimpViewable  *viewable);

gboolean        gimp_viewable_get_expanded       (GimpViewable  *viewable);
void            gimp_viewable_set_expanded       (GimpViewable  *viewable,
                                                  gboolean       expanded);

gboolean        gimp_viewable_is_ancestor        (GimpViewable  *ancestor,
                                                  GimpViewable  *descendant);


#endif  /* __GIMP_VIEWABLE_H__ */
