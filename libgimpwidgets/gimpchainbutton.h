/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpchainbutton.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
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

/*
 * This implements a widget derived from GtkTable that visualizes
 * it's state with two different pixmaps showing a closed and a
 * broken chain. It's intended to be used with the GimpSizeEntry
 * widget. The usage is quite similar to the one the GtkToggleButton
 * provides.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_CHAIN_BUTTON_H__
#define __GIMP_CHAIN_BUTTON_H__

G_BEGIN_DECLS


#define GIMP_TYPE_CHAIN_BUTTON            (gimp_chain_button_get_type ())
#define GIMP_CHAIN_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CHAIN_BUTTON, GimpChainButton))
#define GIMP_CHAIN_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHAIN_BUTTON, GimpChainButtonClass))
#define GIMP_IS_CHAIN_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CHAIN_BUTTON))
#define GIMP_IS_CHAIN_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHAIN_BUTTON))
#define GIMP_CHAIN_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CHAIN_BUTTON, GimpChainButtonClass))


typedef struct _GimpChainButtonClass GimpChainButtonClass;

struct _GimpChainButton
{
  GtkTable  parent_instance;
};

struct _GimpChainButtonClass
{
  GtkTableClass  parent_class;

  void (* toggled)  (GimpChainButton *button);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_chain_button_get_type   (void) G_GNUC_CONST;

GtkWidget * gimp_chain_button_new        (GimpChainPosition  position);

void        gimp_chain_button_set_active (GimpChainButton   *button,
                                          gboolean           active);
gboolean    gimp_chain_button_get_active (GimpChainButton   *button);

GtkWidget * gimp_chain_button_get_button (GimpChainButton   *button);


G_END_DECLS

#endif /* __GIMP_CHAIN_BUTTON_H__ */
