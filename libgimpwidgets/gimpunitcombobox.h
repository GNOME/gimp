/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimpunitcombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_UNIT_COMBO_BOX_H__
#define __GIMP_UNIT_COMBO_BOX_H__

G_BEGIN_DECLS


#define GIMP_TYPE_UNIT_COMBO_BOX            (gimp_unit_combo_box_get_type ())
#define GIMP_UNIT_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_COMBO_BOX, GimpUnitComboBox))
#define GIMP_UNIT_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_COMBO_BOX, GimpUnitComboBoxClass))
#define GIMP_IS_UNIT_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UNIT_COMBO_BOX))
#define GIMP_IS_UNIT_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_COMBO_BOX))
#define GIMP_UNIT_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_COMBO_BOX, GimpUnitComboBoxClass))


typedef struct _GimpUnitComboBoxPrivate GimpUnitComboBoxPrivate;
typedef struct _GimpUnitComboBoxClass   GimpUnitComboBoxClass;

struct _GimpUnitComboBox
{
  GtkComboBox              parent_instance;

  GimpUnitComboBoxPrivate *priv;
};

struct _GimpUnitComboBoxClass
{
  GtkComboBoxClass  parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
};


GType       gimp_unit_combo_box_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_unit_combo_box_new            (void);
GtkWidget * gimp_unit_combo_box_new_with_model (GimpUnitStore    *model);

GimpUnit    gimp_unit_combo_box_get_active     (GimpUnitComboBox *combo);
void        gimp_unit_combo_box_set_active     (GimpUnitComboBox *combo,
                                                GimpUnit          unit);


G_END_DECLS

#endif  /* __GIMP_UNIT_COMBO_BOX_H__ */
