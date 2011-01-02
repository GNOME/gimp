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
 * <http://www.gnu.org/licenses/>.
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


#define GIMP_TYPE_COLOR_SELECTOR            (gimp_color_selector_get_type ())
#define GIMP_COLOR_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_SELECTOR, GimpColorSelector))
#define GIMP_COLOR_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SELECTOR, GimpColorSelectorClass))
#define GIMP_IS_COLOR_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_SELECTOR))
#define GIMP_IS_COLOR_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SELECTOR))
#define GIMP_COLOR_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SELECTOR, GimpColorSelectorClass))


typedef struct _GimpColorSelectorClass GimpColorSelectorClass;

struct _GimpColorSelector
{
  GtkBox                    parent_instance;

  gboolean                  toggles_visible;
  gboolean                  toggles_sensitive;
  gboolean                  show_alpha;

  GimpRGB                   rgb;
  GimpHSV                   hsv;

  GimpColorSelectorChannel  channel;
};

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
                                  const GimpRGB            *rgb,
                                  const GimpHSV            *hsv);
  void (* set_channel)           (GimpColorSelector        *selector,
                                  GimpColorSelectorChannel  channel);

  /*  signals  */
  void (* color_changed)         (GimpColorSelector        *selector,
                                  const GimpRGB            *rgb,
                                  const GimpHSV            *hsv);
  void (* channel_changed)       (GimpColorSelector        *selector,
                                  GimpColorSelectorChannel  channel);

  /*  another virtual function  */
  void (* set_config)            (GimpColorSelector        *selector,
                                  GimpColorConfig          *config);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_color_selector_get_type         (void) G_GNUC_CONST;
GtkWidget * gimp_color_selector_new              (GType              selector_type,
                                                  const GimpRGB     *rgb,
                                                  const GimpHSV     *hsv,
                                                  GimpColorSelectorChannel  channel);

void   gimp_color_selector_set_toggles_visible   (GimpColorSelector *selector,
                                                  gboolean           visible);
void   gimp_color_selector_set_toggles_sensitive (GimpColorSelector *selector,
                                                  gboolean           sensitive);
void   gimp_color_selector_set_show_alpha        (GimpColorSelector *selector,
                                                  gboolean           show_alpha);
void   gimp_color_selector_set_color             (GimpColorSelector *selector,
                                                  const GimpRGB     *rgb,
                                                  const GimpHSV     *hsv);
void   gimp_color_selector_set_channel           (GimpColorSelector *selector,
                                                  GimpColorSelectorChannel  channel);

void   gimp_color_selector_color_changed         (GimpColorSelector *selector);
void   gimp_color_selector_channel_changed       (GimpColorSelector *selector);

void   gimp_color_selector_set_config            (GimpColorSelector *selector,
                                                  GimpColorConfig   *config);


G_END_DECLS

#endif /* __GIMP_COLOR_SELECTOR_H__ */
