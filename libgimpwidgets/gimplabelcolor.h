/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabelcolor.h
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_LABEL_COLOR_H__
#define __GIMP_LABEL_COLOR_H__

#include <libgimpwidgets/gimplabeled.h>

G_BEGIN_DECLS

#define GIMP_TYPE_LABEL_COLOR (gimp_label_color_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpLabelColor, gimp_label_color, GIMP, LABEL_COLOR, GimpLabeled)

struct _GimpLabelColorClass
{
  GimpLabeledClass   parent_class;

  /*  Signals */
  void (* value_changed)   (GimpLabelColor *color);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};

GtkWidget * gimp_label_color_new              (const gchar       *label,
                                               GeglColor         *color,
                                               gboolean           editable);

/* TODO: it would be interesting for such a widget to have an API to
 * customize the label being also above or below, left or right. I could
 * imagine wanting to pretty-list several colors with specific layouts.
 */

void        gimp_label_color_set_value        (GimpLabelColor     *color,
                                               GeglColor          *value);
GeglColor * gimp_label_color_get_value        (GimpLabelColor     *color);

void        gimp_label_color_set_editable     (GimpLabelColor     *color,
                                               gboolean            editable);
gboolean    gimp_label_color_is_editable      (GimpLabelColor     *color);

GtkWidget * gimp_label_color_get_color_widget (GimpLabelColor     *color);

G_END_DECLS

#endif /* __GIMP_LABEL_COLOR_H__ */
