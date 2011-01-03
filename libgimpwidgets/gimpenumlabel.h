/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumlabel.h
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_ENUM__LABEL_H__
#define __GIMP_ENUM__LABEL_H__

G_BEGIN_DECLS


#define GIMP_TYPE_ENUM_LABEL            (gimp_enum_label_get_type ())
#define GIMP_ENUM_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ENUM_LABEL, GimpEnumLabel))
#define GIMP_ENUM_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ENUM_LABEL, GimpEnumLabelClass))
#define GIMP_IS_ENUM_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ENUM_LABEL))
#define GIMP_IS_ENUM_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ENUM_LABEL))
#define GIMP_ENUM_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ENUM_LABEL, GimpEnumLabelClass))


typedef struct _GimpEnumLabelClass  GimpEnumLabelClass;

struct _GimpEnumLabel
{
  GtkLabel  parent_instance;
};

struct _GimpEnumLabelClass
{
  GtkLabelClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_enum_label_get_type         (void) G_GNUC_CONST;

GtkWidget * gimp_enum_label_new              (GType          enum_type,
                                              gint           value);
void        gimp_enum_label_set_value        (GimpEnumLabel *label,
                                              gint           value);


G_END_DECLS

#endif  /* __GIMP_ENUM_LABEL_H__ */
