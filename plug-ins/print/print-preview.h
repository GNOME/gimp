/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __PRINT_PREVIEW_H__
#define __PRINT_PREVIEW_H__

G_BEGIN_DECLS


#define PRINT_TYPE_PREVIEW            (print_preview_get_type ())
#define PRINT_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PRINT_TYPE_PREVIEW, PrintPreview))
#define PRINT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PRINT_TYPE_PREVIEW, PrintPreviewClass))
#define PRINT_IS_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PRINT_TYPE_PREVIEW))
#define PRINT_IS_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PRINT_TYPE_PREVIEW))
#define PRINT_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PRINT_TYPE_PREVIEW, PrintPreviewClass))

typedef struct _PrintPreview       PrintPreview;
typedef struct _PrintPreviewClass  PrintPreviewClass;


GType       print_preview_get_type              (void) G_GNUC_CONST;

GtkWidget * print_preview_new                   (GtkPageSetup *page,
                                                 GimpDrawable *drawable);

void        print_preview_set_image_dpi         (PrintPreview *preview,
                                                 gdouble       xres,
                                                 gdouble       yres);

void        print_preview_set_page_setup        (PrintPreview *preview,
                                                 GtkPageSetup *page);

void        print_preview_set_image_offsets     (PrintPreview *preview,
                                                 gdouble       offset_x,
                                                 gdouble       offset_y);

void        print_preview_set_image_offsets_max (PrintPreview *preview,
                                                 gdouble       offset_x_max,
                                                 gdouble       offset_y_max);

void        print_preview_set_use_full_page     (PrintPreview *preview,
                                                 gboolean      full_page);

G_END_DECLS

#endif /* __PRINT_PREVIEW_H__ */
