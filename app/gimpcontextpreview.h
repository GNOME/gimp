/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpContextPreview Widget
 * Copyright (C) 1999 Sven Neumann
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

#ifndef __GIMP_CONTEXT_PREVIEW_H__
#define __GIMP_CONTEXT_PREVIEW_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_CONTEXT_PREVIEW            (gimp_context_preview_get_type ())
#define GIMP_CONTEXT_PREVIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CONTEXT_PREVIEW, GimpContextPreview))
#define GIMP_CONTEXT_PREVIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTEXT_PREVIEW, GimpContextPreviewClass))
#define GIMP_IS_CONTEXT_PREVIEW(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_CONTEXT_PREVIEW))
#define GIMP_IS_CONTEXT_PREVIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTEXT_PREVIEW))

typedef struct _GimpContextPreview       GimpContextPreview;
typedef struct _GimpContextPreviewClass  GimpContextPreviewClass;

typedef enum
{
  GCP_BRUSH = 0,
  GCP_PATTERN,
  GCP_LAST
} GimpContextPreviewType;

struct _GimpContextPreview
{
  GtkPreview               preview;

  gpointer                 data;
  GimpContextPreviewType   type; 
  gint                     width;
  gint                     height;
  gboolean                 show_tooltips;
};

struct _GimpContextPreviewClass
{
  GtkPreviewClass parent_class;

  void (* clicked)  (GimpContextPreview *gbp);
};

GtkType     gimp_context_preview_get_type  (void);
GtkWidget*  gimp_context_preview_new       (GimpContextPreviewType type,
					    gint       width,
					    gint       height,
					    gboolean   show_tooltips);
void        gimp_context_preview_update    (GimpContextPreview *gcp,
					    gpointer   data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_CONTEXT_PREVIEW_H__ */
