/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Colour selector module (C) 1999 Austin Donnelly <austin@greenend.org.uk>
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

#ifndef __COLOR_SELECTOR_H__
#define __COLOR_SELECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
} GimpColorSelectorChannelType;


typedef void        (* GimpColorSelectorCallback) (gpointer       data,
						   const GimpHSV *hsv,
						   const GimpRGB *rgb);

typedef GtkWidget *  (* GimpColorSelectorNewFunc) (const GimpHSV *hsv,
						   const GimpRGB *rgb,
						   gboolean       show_alpha,
						   GimpColorSelectorCallback cb,
						   gpointer       data,
						   gpointer      *selector_data);

typedef void        (* GimpColorSelectorFreeFunc) (gpointer       selector_data);


typedef void    (* GimpColorSelectorSetColorFunc) (gpointer       selector_data,
						   const GimpHSV *hsv,
						   const GimpRGB *rgb);

typedef void  (* GimpColorSelectorSetChannelFunc) (gpointer       selector_data,
						   GimpColorSelectorChannelType  type);

typedef void      (* GimpColorSelectorFinishedCB) (gpointer       finished_data);


typedef struct _GimpColorSelectorMethods GimpColorSelectorMethods;

struct _GimpColorSelectorMethods
{
  GimpColorSelectorNewFunc        new;
  GimpColorSelectorFreeFunc       free;

  GimpColorSelectorSetColorFunc   set_color;
  GimpColorSelectorSetChannelFunc set_channel;
};

typedef gpointer GimpColorSelectorID;


#include <gmodule.h>

G_MODULE_EXPORT
GimpColorSelectorID
gimp_color_selector_register   (const gchar                 *name,
				const gchar                 *help_page,
				GimpColorSelectorMethods    *methods);

G_MODULE_EXPORT
gboolean
gimp_color_selector_unregister (GimpColorSelectorID          selector_id,
				GimpColorSelectorFinishedCB  finished_cb,
				gpointer                     finished_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __COLOR_SELECTOR_H__ */
