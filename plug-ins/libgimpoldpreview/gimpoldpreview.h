/* LIBGIMPOLDPREVIEW
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpoldpreview.h
 * This file contains the implementation of the gimpoldpreview widget
 * witch is used a a few plug-ins.  This shouldn't be used by any 
 * foreign plug-in, because it uses some deprecated stuff.  We only
 * used it there since we do not a better preview widget for now.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_OLD_PREVIEW_H__
#define __GIMP_OLD_PREVIEW_H__

#include <libgimp/gimptypes.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/* Preview stuff. WARNING: don't use this in new code!!!!!!!
 * It's just here to extract some general preview stuff from plug-ins so
 * that it will be easier to change them when we have a real effect preview
 * widget.
 * Don't say I didn't warn you (Maurits).
 */

typedef struct
{
  GtkWidget *widget;
  GtkWidget *frame;
  guchar    *cache;
  guchar    *even;
  guchar    *odd;
  guchar    *buffer;
  gint       width;
  gint       height;
  gint       rowstride;
  gint       bpp;          /* bpp of the drawable */
  guchar    *cmap;
  gint       ncolors;
  gdouble    scale_x;
  gdouble    scale_y;
  gboolean   is_gray;
} GimpOldPreview;

typedef void (*GimpOldPreviewFunc)  (const guchar *src,
                                     guchar       *dest,
                                     gint          bpp,
                                     gpointer      data);

GimpOldPreview * gimp_old_preview_new    (GimpDrawable       *drawable,
                                          gboolean            has_frame);
GimpOldPreview * gimp_old_preview_new2   (GimpImageType       drawable_type,
                                          gboolean            has_frame);
void             gimp_old_preview_free   (GimpOldPreview     *preview);

void             gimp_old_preview_update (GimpOldPreview     *preview,
                                          GimpOldPreviewFunc  func,
                                          gpointer            data);


void gimp_old_preview_fill_with_thumb (GimpOldPreview   *preview,
                                       gint32            drawable_ID);
void gimp_old_preview_fill            (GimpOldPreview *preview,
                                       GimpDrawable     *drawable);
void gimp_old_preview_fill_scaled     (GimpOldPreview *preview,
                                       GimpDrawable     *drawable);

void gimp_old_preview_do_row          (GimpOldPreview *preview,
                                       gint              row,
                                       gint              width,
                                       const guchar     *src);

void gimp_old_preview_put_pixel       (GimpOldPreview *preview,
                                       gint              x,
                                       gint              y,
                                       const guchar     *pixel);
void gimp_old_preview_get_pixel       (GimpOldPreview *preview,
                                       gint              x,
                                       gint              y,
                                       guchar           *pixel);

G_END_DECLS

#endif /* __GIMP_OLD_PREVIEW_H__ */
