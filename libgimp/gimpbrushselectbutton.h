/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabrushselectbutton.h
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

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_BRUSH_SELECT_BUTTON_H__
#define __LIGMA_BRUSH_SELECT_BUTTON_H__

#include <libligma/ligmaselectbutton.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_BRUSH_SELECT_BUTTON            (ligma_brush_select_button_get_type ())
#define LIGMA_BRUSH_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_SELECT_BUTTON, LigmaBrushSelectButton))
#define LIGMA_BRUSH_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_SELECT_BUTTON, LigmaBrushSelectButtonClass))
#define LIGMA_IS_BRUSH_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_SELECT_BUTTON))
#define LIGMA_IS_BRUSH_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_SELECT_BUTTON))
#define LIGMA_BRUSH_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_SELECT_BUTTON, LigmaBrushSelectButtonClass))


typedef struct _LigmaBrushSelectButtonPrivate LigmaBrushSelectButtonPrivate;
typedef struct _LigmaBrushSelectButtonClass   LigmaBrushSelectButtonClass;

struct _LigmaBrushSelectButton
{
  LigmaSelectButton              parent_instance;

  LigmaBrushSelectButtonPrivate *priv;
};

struct _LigmaBrushSelectButtonClass
{
  LigmaSelectButtonClass  parent_class;

  /* brush_set signal is emitted when brush is chosen */
  void (* brush_set) (LigmaBrushSelectButton *button,
                      const gchar           *brush_name,
                      gdouble                opacity,
                      gint                   spacing,
                      LigmaLayerMode          paint_mode,
                      gint                   width,
                      gint                   height,
                      const guchar          *mask_data,
                      gboolean               dialog_closing);

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType          ligma_brush_select_button_get_type  (void) G_GNUC_CONST;

GtkWidget    * ligma_brush_select_button_new       (const gchar           *title,
                                                   const gchar           *brush_name,
                                                   gdouble                opacity,
                                                   gint                   spacing,
                                                   LigmaLayerMode          paint_mode);

const  gchar * ligma_brush_select_button_get_brush (LigmaBrushSelectButton *button,
                                                   gdouble               *opacity,
                                                   gint                  *spacing,
                                                   LigmaLayerMode         *paint_mode);
void           ligma_brush_select_button_set_brush (LigmaBrushSelectButton *button,
                                                   const gchar           *brush_name,
                                                   gdouble                opacity,
                                                   gint                   spacing,
                                                   LigmaLayerMode          paint_mode);


G_END_DECLS

#endif /* __LIGMA_BRUSH_SELECT_BUTTON_H__ */
