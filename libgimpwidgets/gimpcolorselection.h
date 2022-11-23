/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselection.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_SELECTION_H__
#define __LIGMA_COLOR_SELECTION_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define LIGMA_TYPE_COLOR_SELECTION            (ligma_color_selection_get_type ())
#define LIGMA_COLOR_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_SELECTION, LigmaColorSelection))
#define LIGMA_COLOR_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_SELECTION, LigmaColorSelectionClass))
#define LIGMA_IS_COLOR_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_SELECTION))
#define LIGMA_IS_COLOR_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_SELECTION))
#define LIGMA_COLOR_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_SELECTION, LigmaColorSelectionClass))


typedef struct _LigmaColorSelectionPrivate LigmaColorSelectionPrivate;
typedef struct _LigmaColorSelectionClass   LigmaColorSelectionClass;

struct _LigmaColorSelection
{
  GtkBox                     parent_instance;

  LigmaColorSelectionPrivate *priv;
};

struct _LigmaColorSelectionClass
{
  GtkBoxClass  parent_class;

  void (* color_changed) (LigmaColorSelection *selection);

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


GType       ligma_color_selection_get_type       (void) G_GNUC_CONST;

GtkWidget * ligma_color_selection_new            (void);

void        ligma_color_selection_set_show_alpha (LigmaColorSelection *selection,
                                                 gboolean           show_alpha);
gboolean    ligma_color_selection_get_show_alpha (LigmaColorSelection *selection);

void        ligma_color_selection_set_color      (LigmaColorSelection *selection,
                                                 const LigmaRGB      *color);
void        ligma_color_selection_get_color      (LigmaColorSelection *selection,
                                                 LigmaRGB            *color);

void        ligma_color_selection_set_old_color  (LigmaColorSelection *selection,
                                                 const LigmaRGB      *color);
void        ligma_color_selection_get_old_color  (LigmaColorSelection *selection,
                                                 LigmaRGB            *color);

void        ligma_color_selection_reset          (LigmaColorSelection *selection);

void        ligma_color_selection_color_changed  (LigmaColorSelection *selection);

void        ligma_color_selection_set_simulation (LigmaColorSelection *selection,
                                                 LigmaColorProfile   *profile,
                                                 LigmaColorRenderingIntent intent,
                                                 gboolean            bpc);

void        ligma_color_selection_set_config     (LigmaColorSelection *selection,
                                                 LigmaColorConfig    *config);

GtkWidget * ligma_color_selection_get_notebook   (LigmaColorSelection *selection);
GtkWidget * ligma_color_selection_get_right_vbox (LigmaColorSelection *selection);


G_END_DECLS

#endif /* __LIGMA_COLOR_SELECTION_H__ */
