/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmascrolledpreview.h
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

#ifndef __LIGMA_SCROLLED_PREVIEW_H__
#define __LIGMA_SCROLLED_PREVIEW_H__

#include "ligmapreview.h"

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_SCROLLED_PREVIEW            (ligma_scrolled_preview_get_type ())
#define LIGMA_SCROLLED_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SCROLLED_PREVIEW, LigmaScrolledPreview))
#define LIGMA_SCROLLED_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SCROLLED_PREVIEW, LigmaScrolledPreviewClass))
#define LIGMA_IS_SCROLLED_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SCROLLED_PREVIEW))
#define LIGMA_IS_SCROLLED_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SCROLLED_PREVIEW))
#define LIGMA_SCROLLED_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SCROLLED_PREVIEW, LigmaScrolledPreviewClass))


typedef struct _LigmaScrolledPreviewPrivate LigmaScrolledPreviewPrivate;
typedef struct _LigmaScrolledPreviewClass   LigmaScrolledPreviewClass;

struct _LigmaScrolledPreview
{
  LigmaPreview                 parent_instance;
};

struct _LigmaScrolledPreviewClass
{
  LigmaPreviewClass  parent_class;

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


GType  ligma_scrolled_preview_get_type        (void) G_GNUC_CONST;

void   ligma_scrolled_preview_set_position    (LigmaScrolledPreview  *preview,
                                              gint                  x,
                                              gint                  y);
void   ligma_scrolled_preview_set_policy      (LigmaScrolledPreview  *preview,
                                              GtkPolicyType         hscrollbar_policy,
                                              GtkPolicyType         vscrollbar_policy);

void   ligma_scrolled_preview_get_adjustments (LigmaScrolledPreview  *preview,
                                              GtkAdjustment       **hadj,
                                              GtkAdjustment       **vadj);

/*  only for use from derived widgets  */
void   ligma_scrolled_preview_freeze          (LigmaScrolledPreview  *preview);
void   ligma_scrolled_preview_thaw            (LigmaScrolledPreview  *preview);

/*  utility function for scrolled-window like ligma widgets like the canvas  */
void   ligma_scroll_adjustment_values         (GdkEventScroll       *sevent,
                                              GtkAdjustment        *hadj,
                                              GtkAdjustment        *vadj,
                                              gdouble              *hvalue,
                                              gdouble              *vvalue);


G_END_DECLS

#endif /* __LIGMA_SCROLLED_PREVIEW_H__ */
