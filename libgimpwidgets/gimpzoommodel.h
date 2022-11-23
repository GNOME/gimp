/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmazoommodel.h
 * Copyright (C) 2005  David Odin <dindinx@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_ZOOM_MODEL_H__
#define __LIGMA_ZOOM_MODEL_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_ZOOM_MODEL            (ligma_zoom_model_get_type ())
#define LIGMA_ZOOM_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ZOOM_MODEL, LigmaZoomModel))
#define LIGMA_ZOOM_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ZOOM_MODEL, LigmaZoomModelClass))
#define LIGMA_IS_ZOOM_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ZOOM_MODEL))
#define LIGMA_IS_ZOOM_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ZOOM_MODEL))
#define LIGMA_ZOOM_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ZOOM_MODEL, LigmaZoomModel))


typedef struct _LigmaZoomModelPrivate LigmaZoomModelPrivate;
typedef struct _LigmaZoomModelClass   LigmaZoomModelClass;

struct _LigmaZoomModel
{
  GObject               parent_instance;

  LigmaZoomModelPrivate *priv;
};

struct _LigmaZoomModelClass
{
  GObjectClass  parent_class;

  void (* zoomed) (LigmaZoomModel *model,
                   gdouble        old_factor,
                   gdouble        new_factor);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType           ligma_zoom_model_get_type     (void) G_GNUC_CONST;

LigmaZoomModel * ligma_zoom_model_new          (void);
void            ligma_zoom_model_set_range    (LigmaZoomModel      *model,
                                              gdouble             min,
                                              gdouble             max);
void            ligma_zoom_model_zoom         (LigmaZoomModel      *model,
                                              LigmaZoomType        zoom_type,
                                              gdouble             scale);
gdouble         ligma_zoom_model_get_factor   (LigmaZoomModel      *model);
void            ligma_zoom_model_get_fraction (LigmaZoomModel      *model,
                                              gint               *numerator,
                                              gint               *denominator);

GtkWidget     * ligma_zoom_button_new         (LigmaZoomModel      *model,
                                              LigmaZoomType        zoom_type,
                                              GtkIconSize         icon_size);

gdouble         ligma_zoom_model_zoom_step    (LigmaZoomType        zoom_type,
                                              gdouble             scale,
                                              gdouble             delta);

G_END_DECLS

#endif /* __LIGMA_ZOOM_MODEL_H__ */
