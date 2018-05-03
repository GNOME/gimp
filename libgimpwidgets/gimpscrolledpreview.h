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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_SCROLLED_PREVIEW_H__
#define __GIMP_SCROLLED_PREVIEW_H__

#include "gimppreview.h"

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_SCROLLED_PREVIEW            (gimp_scrolled_preview_get_type ())
#define GIMP_SCROLLED_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SCROLLED_PREVIEW, GimpScrolledPreview))
#define GIMP_SCROLLED_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SCROLLED_PREVIEW, GimpScrolledPreviewClass))
#define GIMP_IS_SCROLLED_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SCROLLED_PREVIEW))
#define GIMP_IS_SCROLLED_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SCROLLED_PREVIEW))
#define GIMP_SCROLLED_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SCROLLED_PREVIEW, GimpScrolledPreviewClass))


typedef struct _GimpScrolledPreviewPrivate GimpScrolledPreviewPrivate;
typedef struct _GimpScrolledPreviewClass   GimpScrolledPreviewClass;

struct _GimpScrolledPreview
{
  GimpPreview                 parent_instance;

  GimpScrolledPreviewPrivate *priv;
};

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


GType  gimp_scrolled_preview_get_type        (void) G_GNUC_CONST;

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


G_END_DECLS

#endif /* __GIMP_SCROLLED_PREVIEW_H__ */
