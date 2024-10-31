/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscaleentry.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>,
 *               2008 Bill Skaggs <weskaggs@primate.ucdavis.edu>
 *               2020 Jehan
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

#include <libgimpwidgets/gimplabelspin.h>

#ifndef __GIMP_SCALE_ENTRY_H__
#define __GIMP_SCALE_ENTRY_H__

G_BEGIN_DECLS

#define GIMP_TYPE_SCALE_ENTRY (gimp_scale_entry_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpScaleEntry, gimp_scale_entry, GIMP, SCALE_ENTRY, GimpLabelSpin)

struct _GimpScaleEntryClass
{
  GimpLabelSpinClass parent_class;

  /*  Class methods  */
  GtkWidget     * (* new_range_widget) (GtkAdjustment  *adjustment);

  /* Padding for future expansion */
  void (* _gimp_reserved0) (void);
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
  void (* _gimp_reserved9) (void);
};

GtkWidget     * gimp_scale_entry_new             (const gchar *text,
                                                  gdouble      value,
                                                  gdouble      lower,
                                                  gdouble      upper,
                                                  guint        digits);

GtkWidget     * gimp_scale_entry_get_range       (GimpScaleEntry *entry);

void            gimp_scale_entry_set_bounds      (GimpScaleEntry *entry,
                                                  gdouble         lower,
                                                  gdouble         upper,
                                                  gboolean        limit_scale);

void            gimp_scale_entry_set_logarithmic (GimpScaleEntry *entry,
                                                  gboolean        logarithmic);
gboolean        gimp_scale_entry_get_logarithmic (GimpScaleEntry *entry);


G_END_DECLS

#endif /* __GIMP_SCALE_ENTRY_H__ */
