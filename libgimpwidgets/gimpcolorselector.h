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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_COLOR_SELECTOR_H__
#define __GIMP_COLOR_SELECTOR_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define GIMP_COLOR_SELECTOR_SIZE     150
#define GIMP_COLOR_SELECTOR_BAR_SIZE 15


typedef enum
{
  GIMP_COLOR_SELECTOR_HUE,
  GIMP_COLOR_SELECTOR_SATURATION,
  GIMP_COLOR_SELECTOR_VALUE,
  GIMP_COLOR_SELECTOR_RED,
  GIMP_COLOR_SELECTOR_GREEN,
  GIMP_COLOR_SELECTOR_BLUE,
  GIMP_COLOR_SELECTOR_ALPHA
} GimpColorSelectorChannel;


#define GIMP_TYPE_COLOR_SELECTOR            (gimp_color_selector_get_type ())
#define GIMP_COLOR_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_SELECTOR, GimpColorSelector))
#define GIMP_COLOR_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SELECTOR, GimpColorSelectorClass))
#define GIMP_IS_COLOR_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_SELECTOR))
#define GIMP_IS_COLOR_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SELECTOR))
#define GIMP_COLOR_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SELECTOR, GimpColorSelectorClass))


typedef struct _GimpColorSelectorClass GimpColorSelectorClass;

struct _GimpColorSelector
{
  GtkVBox  parent_instance;
};

struct _GimpColorSelectorClass
{
  GtkVBoxClass  parent_class;

  const gchar *name;
  const gchar *help_page;

  /*  virtual functions  */
  void (* set_color)     (GimpColorSelector        *selector,
                          const GimpRGB            *rgb,
                          const GimpHSV            *hsv);
  void (* set_channel)   (GimpColorSelector        *selector,
                          GimpColorSelectorChannel  channel);

  /*  signals  */
  void (* color_changed) (GimpColorSelector        *selector,
                          const GimpRGB            *rgb,
                          const GimpHSV            *hsv);
};


GType       gimp_color_selector_get_type (void) G_GNUC_CONST;
GtkWidget * gimp_color_selector_new      (GType          selector_type,
                                          const GimpRGB *rgb,
                                          const GimpHSV *hsv);

void   gimp_color_selector_set_color     (GimpColorSelector        *selector,
                                          const GimpRGB            *rgb,
                                          const GimpHSV            *hsv);
void   gimp_color_selector_set_channel   (GimpColorSelector        *selector,
                                          GimpColorSelectorChannel  channel);

void   gimp_color_selector_color_changed (GimpColorSelector        *selector,
                                          const GimpRGB            *rgb,
                                          const GimpHSV            *hsv);


G_END_DECLS

#endif /* __GIMP_COLOR_SELECTOR_H__ */
