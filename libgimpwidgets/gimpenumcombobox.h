/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumcombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_ENUM_COMBO_BOX_H__
#define __LIGMA_ENUM_COMBO_BOX_H__

#include <libligmawidgets/ligmaintcombobox.h>

G_BEGIN_DECLS

#define LIGMA_TYPE_ENUM_COMBO_BOX            (ligma_enum_combo_box_get_type ())
#define LIGMA_ENUM_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ENUM_COMBO_BOX, LigmaEnumComboBox))
#define LIGMA_ENUM_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ENUM_COMBO_BOX, LigmaEnumComboBoxClass))
#define LIGMA_IS_ENUM_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ENUM_COMBO_BOX))
#define LIGMA_IS_ENUM_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ENUM_COMBO_BOX))
#define LIGMA_ENUM_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ENUM_COMBO_BOX, LigmaEnumComboBoxClass))


typedef struct _LigmaEnumComboBoxPrivate LigmaEnumComboBoxPrivate;
typedef struct _LigmaEnumComboBoxClass   LigmaEnumComboBoxClass;

struct _LigmaEnumComboBox
{
  LigmaIntComboBox          parent_instance;

  LigmaEnumComboBoxPrivate *priv;
};

struct _LigmaEnumComboBoxClass
{
  LigmaIntComboBoxClass  parent_class;

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


GType       ligma_enum_combo_box_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_enum_combo_box_new              (GType             enum_type);
GtkWidget * ligma_enum_combo_box_new_with_model   (LigmaEnumStore    *enum_store);

void        ligma_enum_combo_box_set_icon_prefix  (LigmaEnumComboBox *combo_box,
                                                  const gchar      *icon_prefix);

G_END_DECLS

#endif  /* __LIGMA_ENUM_COMBO_BOX_H__ */
