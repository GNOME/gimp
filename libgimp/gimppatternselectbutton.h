/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapatternselectbutton.h
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

#ifndef __LIGMA_PATTERN_SELECT_BUTTON_H__
#define __LIGMA_PATTERN_SELECT_BUTTON_H__

#include <libligma/ligmaselectbutton.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PATTERN_SELECT_BUTTON            (ligma_pattern_select_button_get_type ())
#define LIGMA_PATTERN_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PATTERN_SELECT_BUTTON, LigmaPatternSelectButton))
#define LIGMA_PATTERN_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PATTERN_SELECT_BUTTON, LigmaPatternSelectButtonClass))
#define LIGMA_IS_PATTERN_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PATTERN_SELECT_BUTTON))
#define LIGMA_IS_PATTERN_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PATTERN_SELECT_BUTTON))
#define LIGMA_PATTERN_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PATTERN_SELECT_BUTTON, LigmaPatternSelectButtonClass))


typedef struct _LigmaPatternSelectButtonPrivate LigmaPatternSelectButtonPrivate;
typedef struct _LigmaPatternSelectButtonClass   LigmaPatternSelectButtonClass;

struct _LigmaPatternSelectButton
{
  LigmaSelectButton                parent_instance;

  LigmaPatternSelectButtonPrivate *priv;
};

struct _LigmaPatternSelectButtonClass
{
  LigmaSelectButtonClass  parent_class;

  /* pattern_set signal is emitted when pattern is chosen */
  void (* pattern_set) (LigmaPatternSelectButton *button,
                        const gchar             *pattern_name,
                        gint                     width,
                        gint                     height,
                        gint                     bpp,
                        const guchar            *mask_data,
                        gboolean                 dialog_closing);

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


GType         ligma_pattern_select_button_get_type    (void) G_GNUC_CONST;

GtkWidget   * ligma_pattern_select_button_new         (const gchar *title,
                                                      const gchar *pattern_name);

const gchar * ligma_pattern_select_button_get_pattern (LigmaPatternSelectButton *button);
void          ligma_pattern_select_button_set_pattern (LigmaPatternSelectButton *button,
                                                      const gchar             *pattern_name);


G_END_DECLS

#endif /* __LIGMA_PATTERN_SELECT_BUTTON_H__ */
