/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __GIMP_PATTERN_PREVIEW_H__
#define __GIMP_PATTERN_PREVIEW_H_

#include <gtk/gtk.h>
#include "patterns.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_PATTERN_PREVIEW            (gimp_pattern_preview_get_type ())
#define GIMP_PATTERN_PREVIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PATTERN_PREVIEW, GimpPatternPreview))
#define GIMP_PATTERN_PREVIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATTERN_PREVIEW, GimpPatternPreviewClass))
#define GIMP_IS_PATTERN_PREVIEW(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_PATTERN_PREVIEW))
#define GIMP_IS_PATTERN_PREVIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATTERN_PREVIEW))

typedef struct _GimpPatternPreview       GimpPatternPreview;
typedef struct _GimpPatternPreviewClass  GimpPatternPreviewClass;


struct _GimpPatternPreview
{
  GtkPreview   preview;

  GPattern    *pattern;
  gint         width;
  gint         height;
  
  GtkTooltips *tooltips;

  GtkWidget   *popup;
  GtkWidget   *popup_preview;
};

struct _GimpPatternPreviewClass
{
  GtkPreviewClass parent_class;

  void (* clicked)  (GimpPatternPreview *gpp);
};

GtkType     gimp_pattern_preview_get_type     (void);
GtkWidget*  gimp_pattern_preview_new          (gint width,
					       gint height);
void        gimp_pattern_preview_update       (GimpPatternPreview *gpp,
					       GPattern           *pattern);
void        gimp_pattern_preview_set_tooltips (GimpPatternPreview *gpp,
					       GtkTooltips        *tooltips);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PATTERN_PREVIEW_H__ */
