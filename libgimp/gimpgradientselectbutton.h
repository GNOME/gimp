/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientselectbutton.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_GRADIENT_SELECT_BUTTON_H__
#define __GIMP_GRADIENT_SELECT_BUTTON_H__

#include <libgimp/gimpselectbutton.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_GRADIENT_SELECT_BUTTON            (gimp_gradient_select_button_get_type ())
#define GIMP_GRADIENT_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GRADIENT_SELECT_BUTTON, GimpGradientSelectButton))
#define GIMP_GRADIENT_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GRADIENT_SELECT_BUTTON, GimpGradientSelectButtonClass))
#define GIMP_IS_GRADIENT_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GRADIENT_SELECT_BUTTON))
#define GIMP_IS_GRADIENT_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GRADIENT_SELECT_BUTTON))
#define GIMP_GRADIENT_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GRADIENT_SELECT_BUTTON, GimpGradientSelectButtonClass))


typedef struct _GimpGradientSelectButtonPrivate GimpGradientSelectButtonPrivate;
typedef struct _GimpGradientSelectButtonClass   GimpGradientSelectButtonClass;

struct _GimpGradientSelectButton
{
  GimpSelectButton                 parent_instance;

  GimpGradientSelectButtonPrivate *priv;
};

struct _GimpGradientSelectButtonClass
{
  GimpSelectButtonClass  parent_class;

  /* gradient_set signal is emitted when gradient is chosen */
  void (* gradient_set) (GimpGradientSelectButton *button,
                         const gchar              *gradient_name,
                         gint                      width,
                         const gdouble            *gradient_data,
                         gboolean                  dialog_closing);

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


GType         gimp_gradient_select_button_get_type     (void) G_GNUC_CONST;

GtkWidget   * gimp_gradient_select_button_new          (const gchar *title,
                                                        const gchar *gradient_name);

const gchar * gimp_gradient_select_button_get_gradient (GimpGradientSelectButton *button);
void          gimp_gradient_select_button_set_gradient (GimpGradientSelectButton *button,
                                                        const gchar              *gradient_name);


G_END_DECLS

#endif /* __GIMP_GRADIENT_SELECT_BUTTON_H__ */
