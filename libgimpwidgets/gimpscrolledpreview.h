/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscrolledpreview.h
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

#ifndef __GIMP_SCROLLED_PREVIEW_H__
#define __GIMP_SCROLLED_PREVIEW_H__

#include "gimppreview.h"

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_SCROLLED_PREVIEW (gimp_scrolled_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpScrolledPreview, gimp_scrolled_preview, GIMP, SCROLLED_PREVIEW, GimpPreview)

struct _GimpScrolledPreviewClass
{
  GimpPreviewClass  parent_class;

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


void   gimp_scrolled_preview_set_position    (GimpScrolledPreview  *preview,
                                              gint                  x,
                                              gint                  y);
void   gimp_scrolled_preview_set_policy      (GimpScrolledPreview  *preview,
                                              GtkPolicyType         hscrollbar_policy,
                                              GtkPolicyType         vscrollbar_policy);

void   gimp_scrolled_preview_get_adjustments (GimpScrolledPreview  *preview,
                                              GtkAdjustment       **hadj,
                                              GtkAdjustment       **vadj);

/*  only for use from derived widgets  */
void   gimp_scrolled_preview_freeze          (GimpScrolledPreview  *preview);
void   gimp_scrolled_preview_thaw            (GimpScrolledPreview  *preview);

/*  utility function for scrolled-window like gimp widgets like the canvas  */
void   gimp_scroll_adjustment_values         (GdkEventScroll       *sevent,
                                              GtkAdjustment        *hadj,
                                              GtkAdjustment        *vadj,
                                              gdouble              *hvalue,
                                              gdouble              *vvalue);


G_END_DECLS

#endif /* __GIMP_SCROLLED_PREVIEW_H__ */
