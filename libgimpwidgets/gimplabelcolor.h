/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmalabelcolor.h
 * Copyright (C) 2022 Jehan
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

#ifndef __LIGMA_LABEL_COLOR_H__
#define __LIGMA_LABEL_COLOR_H__

#include <libligmawidgets/ligmalabeled.h>

G_BEGIN_DECLS

#define LIGMA_TYPE_LABEL_COLOR (ligma_label_color_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaLabelColor, ligma_label_color, LIGMA, LABEL_COLOR, LigmaLabeled)

struct _LigmaLabelColorClass
{
  LigmaLabeledClass   parent_class;

  /*  Signals */
  void (* value_changed)   (LigmaLabelColor *color);

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

GtkWidget * ligma_label_color_new              (const gchar       *label,
                                               const LigmaRGB     *color,
                                               gboolean           editable);

/* TODO: it would be interesting for such a widget to have an API to
 * customize the label being also above or below, left or right. I could
 * imagine wanting to pretty-list several colors with specific layouts.
 */

void        ligma_label_color_set_value        (LigmaLabelColor     *color,
                                               const LigmaRGB      *value);
void        ligma_label_color_get_value        (LigmaLabelColor     *color,
                                               LigmaRGB            *value);

void        ligma_label_color_set_editable     (LigmaLabelColor     *color,
                                               gboolean            editable);
gboolean    ligma_label_color_is_editable      (LigmaLabelColor     *color);

GtkWidget * ligma_label_color_get_color_widget (LigmaLabelColor     *color);

G_END_DECLS

#endif /* __LIGMA_LABEL_COLOR_H__ */
