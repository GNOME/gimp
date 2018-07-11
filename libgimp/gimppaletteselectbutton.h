/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppaletteselectbutton.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_PALETTE_SELECT_BUTTON_H__
#define __GIMP_PALETTE_SELECT_BUTTON_H__

#include <libgimp/gimpselectbutton.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PALETTE_SELECT_BUTTON            (gimp_palette_select_button_get_type ())
#define GIMP_PALETTE_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PALETTE_SELECT_BUTTON, GimpPaletteSelectButton))
#define GIMP_PALETTE_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE_SELECT_BUTTON, GimpPaletteSelectButtonClass))
#define GIMP_IS_PALETTE_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PALETTE_SELECT_BUTTON))
#define GIMP_IS_PALETTE_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE_SELECT_BUTTON))
#define GIMP_PALETTE_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PALETTE_SELECT_BUTTON, GimpPaletteSelectButtonClass))


typedef struct _GimpPaletteSelectButtonClass  GimpPaletteSelectButtonClass;

struct _GimpPaletteSelectButton
{
  GimpSelectButton  parent_instance;
};

struct _GimpPaletteSelectButtonClass
{
  GimpSelectButtonClass  parent_class;

  /* palette_set signal is emitted when palette is chosen */
  void (* palette_set) (GimpPaletteSelectButton *button,
                        const gchar             *palette_name,
                        gboolean                 dialog_closing);

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
};


GType         gimp_palette_select_button_get_type    (void) G_GNUC_CONST;

GtkWidget   * gimp_palette_select_button_new         (const gchar *title,
                                                      const gchar *palette_name);

const gchar * gimp_palette_select_button_get_palette (GimpPaletteSelectButton *button);
void          gimp_palette_select_button_set_palette (GimpPaletteSelectButton *button,
                                                      const gchar             *palette_name);


G_END_DECLS

#endif /* __GIMP_PALETTE_SELECT_BUTTON_H__ */
