/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPreview Widget
 * Copyright (C) 2001 Michael Natterer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PREVIEW_H__
#define __GIMP_PREVIEW_H__


#include <gtk/gtkpreview.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_PREVIEW            (gimp_preview_get_type ())
#define GIMP_PREVIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PREVIEW, GimpPreview))
#define GIMP_PREVIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PREVIEW, GimpPreviewClass))
#define GIMP_IS_PREVIEW(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_PREVIEW))
#define GIMP_IS_PREVIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PREVIEW))


typedef struct _GimpPreviewClass  GimpPreviewClass;

struct _GimpPreview
{
  GtkPreview    parent_instance;

  GimpViewable *viewable;

  gint          width;
  gint          height;
  gint          border_width;

  GimpRGB       border_color;

  gboolean      is_popup;
  gboolean      clickable;
  gboolean      show_popup;

  /*< private >*/
  gboolean      in_button;
  guint         idle_id;
  guint         popup_id;
  gint          popup_x;
  gint          popup_y;
};

struct _GimpPreviewClass
{
  GtkPreviewClass  parent_class;

  void        (* clicked)        (GimpPreview *preview);
  void        (* double_clicked) (GimpPreview *preview);
  void        (* render)         (GimpPreview *preview);
  void        (* get_size)       (GimpPreview *preview,
				  gint         size,
				  gint        *width,
				  gint        *height);
  gboolean    (* needs_popup)    (GimpPreview *preview);
  GtkWidget * (* create_popup)   (GimpPreview *preview);
};


GtkType      gimp_preview_get_type         (void);

GtkWidget *  gimp_preview_new              (GimpViewable  *viewable,
					    gint           size,
					    gint           border_width,
					    gboolean       is_popup);
GtkWidget *  gimp_preview_new_full         (GimpViewable  *viewable,
					    gint           width,
					    gint           height,
					    gint           border_width,
					    gboolean       is_popup,
					    gboolean       clickable,
					    gboolean       show_popup);

void         gimp_preview_set_viewable     (GimpPreview   *preview,
					    GimpViewable  *viewable);

void         gimp_preview_set_size         (GimpPreview   *preview,
					    gint           size,
					    gint           border_width);
void         gimp_preview_set_size_full    (GimpPreview   *preview,
					    gint           width,
					    gint           height,
					    gint           border_width);

void         gimp_preview_set_border_color (GimpPreview   *preview,
					    const GimpRGB *border_color);

void         gimp_preview_render           (GimpPreview   *preview);


/*  protected  */

void         gimp_preview_render_and_flush (GimpPreview   *preview,
					    TempBuf       *temp_buf,
					    gint           channel);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PREVIEW_H__ */
