/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmascaleentry.h
 * Copyright (C) 2000 Michael Natterer <mitch@ligma.org>,
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#include <libligmawidgets/ligmalabelspin.h>

#ifndef __LIGMA_SCALE_ENTRY_H__
#define __LIGMA_SCALE_ENTRY_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_SCALE_ENTRY (ligma_scale_entry_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaScaleEntry, ligma_scale_entry, LIGMA, SCALE_ENTRY, LigmaLabelSpin)

struct _LigmaScaleEntryClass
{
  LigmaLabelSpinClass parent_class;

  /*  Signals        */
  void            (* value_changed)    (LigmaScaleEntry *entry);

  /*  Class methods  */
  GtkWidget     * (* new_range_widget) (GtkAdjustment  *adjustment);

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

GtkWidget     * ligma_scale_entry_new             (const gchar *text,
                                                  gdouble      value,
                                                  gdouble      lower,
                                                  gdouble      upper,
                                                  guint        digits);

GtkWidget     * ligma_scale_entry_get_range       (LigmaScaleEntry *entry);

void            ligma_scale_entry_set_bounds      (LigmaScaleEntry *entry,
                                                  gdouble         lower,
                                                  gdouble         upper,
                                                  gboolean        limit_scale);

void            ligma_scale_entry_set_logarithmic (LigmaScaleEntry *entry,
                                                  gboolean        logarithmic);
gboolean        ligma_scale_entry_get_logarithmic (LigmaScaleEntry *entry);


G_END_DECLS

#endif /* __LIGMA_SCALE_ENTRY_H__ */
