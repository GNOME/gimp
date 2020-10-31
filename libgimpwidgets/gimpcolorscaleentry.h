/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscaleentry.h
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_SCALE_ENTRY_H__
#define __GIMP_COLOR_SCALE_ENTRY_H__

#include <libgimpwidgets/gimpscaleentry.h>

G_BEGIN_DECLS

#define GIMP_TYPE_COLOR_SCALE_ENTRY            (gimp_color_scale_entry_get_type ())
#define GIMP_COLOR_SCALE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_SCALE_ENTRY, GimpColorScaleEntry))
#define GIMP_COLOR_SCALE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SCALE_ENTRY, GimpColorScaleEntryClass))
#define GIMP_IS_COLOR_SCALE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_COLOR_SCALE_ENTRY))
#define GIMP_IS_COLOR_SCALE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SCALE_ENTRY))
#define GIMP_COLOR_SCALE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SCALE_ENTRY, GimpColorScaleEntryClass))


typedef struct _GimpColorScaleEntryPrivate GimpColorScaleEntryPrivate;
typedef struct _GimpColorScaleEntryClass   GimpColorScaleEntryClass;

struct _GimpColorScaleEntry
{
  GimpScaleEntry              parent_instance;
};

struct _GimpColorScaleEntryClass
{
  GimpScaleEntryClass  parent_class;

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

GType           gimp_color_scale_entry_get_type   (void) G_GNUC_CONST;

GtkWidget     * gimp_color_scale_entry_new2       (const gchar *text,
                                                   gdouble      value,
                                                   gdouble      lower,
                                                   gdouble      upper,
                                                   guint        digits);


G_END_DECLS

#endif /* __GIMP_COLOR_SCALE_ENTRY_H__ */
