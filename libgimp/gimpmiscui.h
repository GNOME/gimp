/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmiscui.h
 * Contains all kinds of miscellaneous routines factored out from different
 * plug-ins. They stay here until their API has crystalized a bit and we can
 * put them into the file where they belong (Maurits Rijk
 * <lpeek.mrijk@consunet.nl> if you want to blame someone for this mess)
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

#ifndef __GIMP_MISCUI_H__
#define __GIMP_MISCUI_H__

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
  gint	     width;
  gint	     height;
  gint       rowstride;
  gint       bpp;		/* bpp of the drawable */
  guchar    *cmap;
  gint       ncolors;
  gdouble    scale_x;
  gdouble    scale_y;
  gboolean   is_gray;
} GimpFixMePreview;

typedef void (*GimpFixeMePreviewFunc)  (const guchar *src,
                                        guchar       *dest,
                                        gint          bpp,
                                        gpointer      data);

GimpFixMePreview * gimp_fixme_preview_new    (GimpDrawable     *drawable,
                                              gboolean          has_frame);
GimpFixMePreview * gimp_fixme_preview_new2   (GimpImageType     drawable_type,
                                              gboolean          has_frame);
void               gimp_fixme_preview_free   (GimpFixMePreview *preview);

void               gimp_fixme_preview_update (GimpFixMePreview *preview,
                                              GimpFixeMePreviewFunc func,
                                              gpointer          data);

void      gimp_fixme_preview_fill_with_thumb (GimpFixMePreview *preview,
                                              gint32            drawable_ID);
void      gimp_fixme_preview_fill            (GimpFixMePreview *preview,
                                              GimpDrawable     *drawable);
void      gimp_fixme_preview_fill_scaled     (GimpFixMePreview *preview,
                                              GimpDrawable     *drawable);

void      gimp_fixme_preview_do_row          (GimpFixMePreview *preview,
                                              gint              row,
                                              gint              width,
                                              const guchar     *src);

void      gimp_fixme_preview_put_pixel       (GimpFixMePreview *preview,
                                              gint              x,
                                              gint              y,
                                              const guchar     *pixel);
void      gimp_fixme_preview_get_pixel       (GimpFixMePreview *preview,
                                              gint              x,
                                              gint              y,
                                              guchar           *pixel);

gchar   * gimp_plug_in_get_path              (const gchar *path_name,
                                              const gchar *dir_name);

G_END_DECLS

#endif /* __GIMP_MISCUI_H__ */
