/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorscale.h
 * Copyright (C) 2002  Sven Neumann <sven@ligma.org>
 *                     Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_SCALE_H__
#define __LIGMA_COLOR_SCALE_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_SCALE            (ligma_color_scale_get_type ())
#define LIGMA_COLOR_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_SCALE, LigmaColorScale))
#define LIGMA_COLOR_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_SCALE, LigmaColorScaleClass))
#define LIGMA_IS_COLOR_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_SCALE))
#define LIGMA_IS_COLOR_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_SCALE))
#define LIGMA_COLOR_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_SCALE, LigmaColorScaleClass))


typedef struct _LigmaColorScalePrivate LigmaColorScalePrivate;
typedef struct _LigmaColorScaleClass   LigmaColorScaleClass;

struct _LigmaColorScale
{
  GtkScale               parent_instance;

  LigmaColorScalePrivate *priv;
};

struct _LigmaColorScaleClass
{
  GtkScaleClass  parent_class;

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


GType       ligma_color_scale_get_type         (void) G_GNUC_CONST;
GtkWidget * ligma_color_scale_new              (GtkOrientation            orientation,
                                               LigmaColorSelectorChannel  channel);

void        ligma_color_scale_set_channel      (LigmaColorScale           *scale,
                                               LigmaColorSelectorChannel  channel);
void        ligma_color_scale_set_color        (LigmaColorScale           *scale,
                                               const LigmaRGB            *rgb,
                                               const LigmaHSV            *hsv);

void        ligma_color_scale_set_color_config (LigmaColorScale           *scale,
                                               LigmaColorConfig          *config);


G_END_DECLS

#endif /* __LIGMA_COLOR_SCALE_H__ */
