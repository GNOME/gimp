/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselector.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on:
 * Colour selector module
 * Copyright (C) 1999 Austin Donnelly <austin@greenend.org.uk>
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

#ifndef __GIMP_COLOR_SELECTOR_H__
#define __GIMP_COLOR_SELECTOR_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


/**
 * GIMP_COLOR_SELECTOR_SIZE:
 *
 * The suggested size for a color area in a #GimpColorSelector
 * implementation.
 **/
#define GIMP_COLOR_SELECTOR_SIZE     150

/**
 * GIMP_COLOR_SELECTOR_BAR_SIZE:
 *
 * The suggested width for a color bar in a #GimpColorSelector
 * implementation.
 **/
#define GIMP_COLOR_SELECTOR_BAR_SIZE 15


#define GIMP_TYPE_COLOR_SELECTOR (gimp_color_selector_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpColorSelector, gimp_color_selector, GIMP, COLOR_SELECTOR, GtkBox)

struct _GimpColorSelectorClass
{
  GtkBoxClass  parent_class;

  const gchar *name;
  const gchar *help_id;
  const gchar *icon_name;

  /*  virtual functions  */
  void (* set_toggles_visible)   (GimpColorSelector        *selector,
                                  gboolean                  visible);
  void (* set_toggles_sensitive) (GimpColorSelector        *selector,
                                  gboolean                  sensitive);
  void (* set_show_alpha)        (GimpColorSelector        *selector,
                                  gboolean                  show_alpha);
  void (* set_color)             (GimpColorSelector        *selector,
                                  GeglColor                *color);
  void (* set_channel)           (GimpColorSelector        *selector,
                                  GimpColorSelectorChannel  channel);
  void (* set_model_visible)     (GimpColorSelector        *selector,
                                  GimpColorSelectorModel    model,
                                  gboolean                  visible);
  void (* set_config)            (GimpColorSelector        *selector,
                                  GimpColorConfig          *config);

  void (* set_format)            (GimpColorSelector        *selector,
                                  const Babl               *format);
  void (* set_simulation)        (GimpColorSelector        *selector,
                                  GimpColorProfile         *profile,
                                  GimpColorRenderingIntent  intent,
                                  gboolean                  bpc);

  /*  signals  */
  void (* color_changed)         (GimpColorSelector        *selector,
                                  GeglColor                *color);
  void (* channel_changed)       (GimpColorSelector        *selector,
                                  GimpColorSelectorChannel  channel);
  void (* model_visible_changed) (GimpColorSelector        *selector,
                                  GimpColorSelectorModel    model,
                                  gboolean                  visible);
  void (* simulation)            (GimpColorSelector        *selector,
                                  gboolean                  enabled);

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


GtkWidget * gimp_color_selector_new                   (GType              selector_type,
                                                       GeglColor         *color,
                                                       GimpColorSelectorChannel  channel);

void        gimp_color_selector_set_toggles_visible   (GimpColorSelector *selector,
                                                       gboolean           visible);
gboolean    gimp_color_selector_get_toggles_visible   (GimpColorSelector *selector);

void        gimp_color_selector_set_toggles_sensitive (GimpColorSelector *selector,
                                                       gboolean           sensitive);
gboolean    gimp_color_selector_get_toggles_sensitive (GimpColorSelector *selector);

void        gimp_color_selector_set_show_alpha        (GimpColorSelector *selector,
                                                       gboolean           show_alpha);
gboolean    gimp_color_selector_get_show_alpha        (GimpColorSelector *selector);

void        gimp_color_selector_set_color             (GimpColorSelector *selector,
                                                       GeglColor         *color);
GeglColor * gimp_color_selector_get_color             (GimpColorSelector *selector);

void        gimp_color_selector_set_channel           (GimpColorSelector *selector,
                                                       GimpColorSelectorChannel  channel);
GimpColorSelectorChannel
            gimp_color_selector_get_channel           (GimpColorSelector *selector);

void        gimp_color_selector_set_model_visible     (GimpColorSelector *selector,
                                                       GimpColorSelectorModel model,
                                                       gboolean           visible);
gboolean    gimp_color_selector_get_model_visible     (GimpColorSelector *selector,
                                                       GimpColorSelectorModel model);

void        gimp_color_selector_set_config            (GimpColorSelector *selector,
                                                       GimpColorConfig   *config);

void        gimp_color_selector_set_format            (GimpColorSelector         *selector,
                                                       const Babl                *format);
void        gimp_color_selector_set_simulation        (GimpColorSelector         *selector,
                                                       GimpColorProfile          *profile,
                                                       GimpColorRenderingIntent  intent,
                                                       gboolean                   bpc);
gboolean    gimp_color_selector_get_simulation        (GimpColorSelector         *selector,
                                                       GimpColorProfile         **profile,
                                                       GimpColorRenderingIntent *intent,
                                                       gboolean                  *bpc);
gboolean    gimp_color_selector_enable_simulation     (GimpColorSelector         *selector,
                                                       gboolean                   enabled);


G_END_DECLS

#endif /* __GIMP_COLOR_SELECTOR_H__ */
