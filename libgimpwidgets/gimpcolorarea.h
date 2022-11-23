/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorarea.h
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

/* This provides a color preview area. The preview
 * can handle transparency by showing the checkerboard and
 * handles drag'n'drop.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_AREA_H__
#define __LIGMA_COLOR_AREA_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_AREA            (ligma_color_area_get_type ())
#define LIGMA_COLOR_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_AREA, LigmaColorArea))
#define LIGMA_COLOR_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_AREA, LigmaColorAreaClass))
#define LIGMA_IS_COLOR_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_AREA))
#define LIGMA_IS_COLOR_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_AREA))
#define LIGMA_COLOR_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_AREA, LigmaColorAreaClass))


typedef struct _LigmaColorAreaPrivate LigmaColorAreaPrivate;
typedef struct _LigmaColorAreaClass   LigmaColorAreaClass;

struct _LigmaColorArea
{
  GtkDrawingArea        parent_instance;

  LigmaColorAreaPrivate *priv;
};

struct _LigmaColorAreaClass
{
  GtkDrawingAreaClass  parent_class;

  void (* color_changed) (LigmaColorArea *area);

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


GType       ligma_color_area_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_color_area_new              (const LigmaRGB     *color,
                                              LigmaColorAreaType  type,
                                              GdkModifierType    drag_mask);

void        ligma_color_area_set_color        (LigmaColorArea     *area,
                                              const LigmaRGB     *color);
void        ligma_color_area_get_color        (LigmaColorArea     *area,
                                              LigmaRGB           *color);

gboolean    ligma_color_area_has_alpha        (LigmaColorArea     *area);
void        ligma_color_area_set_type         (LigmaColorArea     *area,
                                              LigmaColorAreaType  type);
void        ligma_color_area_enable_drag      (LigmaColorArea     *area,
                                              GdkModifierType    drag_mask);
void        ligma_color_area_set_draw_border  (LigmaColorArea     *area,
                                              gboolean           draw_border);
void        ligma_color_area_set_out_of_gamut (LigmaColorArea     *area,
                                              gboolean           out_of_gamut);

void        ligma_color_area_set_color_config (LigmaColorArea     *area,
                                              LigmaColorConfig   *config);


G_END_DECLS

#endif /* __LIGMA_COLOR_AREA_H__ */
