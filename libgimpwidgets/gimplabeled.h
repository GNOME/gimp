/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmalabeled.h
 * Copyright (C) 2020 Jehan
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

#ifndef __LIGMA_LABELED_H__
#define __LIGMA_LABELED_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_LABELED (ligma_labeled_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaLabeled, ligma_labeled, LIGMA, LABELED, GtkGrid)

struct _LigmaLabeledClass
{
  GtkGridClass       parent_class;

  /*  Signals        */

  void (* mnemonic_widget_changed) (LigmaLabeled *labeled,
                                    GtkWidget   *widget);

  /*  Class methods  */

  /**
   * LigmaLabelledClass::populate:
   *
   * Fill the #GtkGrid with any necessary widget and sets the
   * coordinates and dimensions the #GtkLabel should be attached to.
   * By default, @x, @y, @width and @height will be pre-filled with 0,
   * 0, 1 and 1 respectively, i.e. the top-left of the grid. There is no
   * need to edit these output variables unless your subclass wants the
   * label to be placed elsewhere.
   *
   * Returns: (transfer none): the #GtkWidget which the label must be
   *                           set as mnemonic to.
   **/
  GtkWidget     * (* populate)     (LigmaLabeled *labeled,
                                    gint        *x,
                                    gint        *y,
                                    gint        *width,
                                    gint        *height);


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

GtkWidget     * ligma_labeled_get_label (LigmaLabeled *labeled);

const gchar   * ligma_labeled_get_text  (LigmaLabeled *labeled);
void            ligma_labeled_set_text  (LigmaLabeled *labeled,
                                        const gchar *text);


G_END_DECLS

#endif /* __LIGMA_LABELED_H__ */
