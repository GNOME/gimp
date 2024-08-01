/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpzoommodel.h
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_ZOOM_MODEL_H__
#define __GIMP_ZOOM_MODEL_H__

G_BEGIN_DECLS


#define GIMP_TYPE_ZOOM_MODEL (gimp_zoom_model_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpZoomModel, gimp_zoom_model, GIMP, ZOOM_MODEL, GtkGrid)

struct _GimpZoomModelClass
{
  GObjectClass  parent_class;

  void (* zoomed) (GimpZoomModel *model,
                   gdouble        old_factor,
                   gdouble        new_factor);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GimpZoomModel * gimp_zoom_model_new          (void);
void            gimp_zoom_model_set_range    (GimpZoomModel      *model,
                                              gdouble             min,
                                              gdouble             max);
void            gimp_zoom_model_zoom         (GimpZoomModel      *model,
                                              GimpZoomType        zoom_type,
                                              gdouble             scale);
gdouble         gimp_zoom_model_get_factor   (GimpZoomModel      *model);
void            gimp_zoom_model_get_fraction (GimpZoomModel      *model,
                                              gint               *numerator,
                                              gint               *denominator);

GtkWidget     * gimp_zoom_button_new         (GimpZoomModel      *model,
                                              GimpZoomType        zoom_type,
                                              GtkIconSize         icon_size);

gdouble         gimp_zoom_model_zoom_step    (GimpZoomType        zoom_type,
                                              gdouble             scale,
                                              gdouble             delta);

G_END_DECLS

#endif /* __GIMP_ZOOM_MODEL_H__ */
