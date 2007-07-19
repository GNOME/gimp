/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_PRINT_PREVIEW_H__
#define __GIMP_PRINT_PREVIEW_H__

G_BEGIN_DECLS


#define GIMP_TYPE_PRINT_PREVIEW            (gimp_print_preview_get_type ())
#define GIMP_PRINT_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PRINT_PREVIEW, GimpPrintPreview))
#define GIMP_PRINT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PRINT_PREVIEW, GimpPrintPreviewClass))
#define GIMP_IS_PRINT_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PRINT_PREVIEW))
#define GIMP_IS_PRINT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PRINT_PREVIEW))
#define GIMP_PRINT_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PRINT_PREVIEW, GimpPrintPreviewClass))

typedef struct _GimpPrintPreview       GimpPrintPreview;
typedef struct _GimpPrintPreviewClass  GimpPrintPreviewClass;

struct _GimpPrintPreview
{
  GtkAspectFrame  parent_instance;

  GtkWidget      *area;
  GtkPageSetup   *page;
  GdkPixbuf      *pixbuf;

  gint32          drawable_id;

  gdouble         image_offset_x;
  gdouble         image_offset_y;
  gdouble         image_offset_x_max;
  gdouble         image_offset_y_max;
  gdouble         image_xres;
  gdouble         image_yres;

  gboolean        use_full_page;
};

struct _GimpPrintPreviewClass
{
  GtkAspectFrameClass  parent_class;

  void (* offsets_changed)  (GimpPrintPreview *print_preview,
                             gint              offset_x,
                             gint              offset_y);
};


GType       gimp_print_preview_get_type          (void) G_GNUC_CONST;

GtkWidget * gimp_print_preview_new               (GtkPageSetup     *page,
                                                  gint32            drawable_id);

void        gimp_print_preview_set_image_dpi     (GimpPrintPreview *preview,
                                                  gdouble           xres,
                                                  gdouble           yres);

void        gimp_print_preview_set_page_setup    (GimpPrintPreview *preview,
                                                  GtkPageSetup     *page);

void        gimp_print_preview_set_image_offsets (GimpPrintPreview *preview,
                                                  gdouble           offset_x,
                                                  gdouble           offset_y);

void        gimp_print_preview_set_image_offsets_max (GimpPrintPreview *preview,
                                                      gdouble           offset_x_max,
                                                      gdouble           offset_y_max);

void        gimp_print_preview_set_use_full_page (GimpPrintPreview *preview,
                                                  gboolean          full_page);

G_END_DECLS

#endif /* __GIMP_PRINT_PREVIEW_H__ */
