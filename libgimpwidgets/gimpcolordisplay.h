/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolordisplay.c
 * Copyright (C) 1999 Manish Singh <yosh@gimp.org>
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

#ifndef __GIMP_COLOR_DISPLAY_H__
#define __GIMP_COLOR_DISPLAY_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define GIMP_TYPE_COLOR_DISPLAY            (gimp_color_display_get_type ())
#define GIMP_COLOR_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_DISPLAY, GimpColorDisplay))
#define GIMP_COLOR_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_DISPLAY, GimpColorDisplayClass))
#define GIMP_IS_COLOR_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_DISPLAY))
#define GIMP_IS_COLOR_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_DISPLAY))
#define GIMP_COLOR_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_DISPLAY, GimpColorDisplayClass))


typedef struct _GimpColorDisplayClass GimpColorDisplayClass;

struct _GimpColorDisplay
{
  GObject  parent_instance;
};

struct _GimpColorDisplayClass
{
  GObjectClass  parent_class;

  const gchar  *name;
  const gchar  *help_id;
  const gchar  *icon_name;

  /*  virtual functions  */

  /*  implementing the GimpColorDisplay::clone method is deprecated       */
  GimpColorDisplay * (* clone)           (GimpColorDisplay *display);

  void               (* convert_buffer)  (GimpColorDisplay *display,
                                          GeglBuffer       *buffer,
                                          GeglRectangle    *area);

  /*  implementing the GimpColorDisplay::convert_surface method is deprecated  */
  void               (* convert_surface) (GimpColorDisplay *display,
                                          cairo_surface_t  *surface);

  /*  implementing the GimpColorDisplay::convert method is deprecated     */
  void               (* convert)         (GimpColorDisplay *display,
                                          guchar           *buf,
                                          gint              width,
                                          gint              height,
                                          gint              bpp,
                                          gint              bpl);

  /*  implementing the GimpColorDisplay::load_state method is deprecated  */
  void               (* load_state)      (GimpColorDisplay *display,
                                          GimpParasite     *state);

  /*  implementing the GimpColorDisplay::save_state method is deprecated  */
  GimpParasite     * (* save_state)      (GimpColorDisplay *display);

  GtkWidget        * (* configure)       (GimpColorDisplay *display);

  /*  implementing the GimpColorDisplay::configure_reset method is deprecated */
  void               (* configure_reset) (GimpColorDisplay *display);

  /*  signals  */
  void               (* changed)         (GimpColorDisplay *display);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType              gimp_color_display_get_type    (void) G_GNUC_CONST;

GIMP_DEPRECATED_FOR(g_object_new)
GimpColorDisplay * gimp_color_display_new         (GType             display_type);
GimpColorDisplay * gimp_color_display_clone       (GimpColorDisplay *display);

void           gimp_color_display_convert_buffer  (GimpColorDisplay *display,
                                                   GeglBuffer       *buffer,
                                                   GeglRectangle    *area);
GIMP_DEPRECATED_FOR(gimp_color_display_convert_buffer)
void           gimp_color_display_convert_surface (GimpColorDisplay *display,
                                                   cairo_surface_t  *surface);
GIMP_DEPRECATED_FOR(gimp_color_display_convert_buffer)
void           gimp_color_display_convert         (GimpColorDisplay *display,
                                                   guchar           *buf,
                                                   gint              width,
                                                   gint              height,
                                                   gint              bpp,
                                                   gint              bpl);
void           gimp_color_display_load_state      (GimpColorDisplay *display,
                                                   GimpParasite     *state);
GimpParasite * gimp_color_display_save_state      (GimpColorDisplay *display);
GtkWidget    * gimp_color_display_configure       (GimpColorDisplay *display);
void           gimp_color_display_configure_reset (GimpColorDisplay *display);

void           gimp_color_display_changed         (GimpColorDisplay *display);

void           gimp_color_display_set_enabled     (GimpColorDisplay *display,
                                                   gboolean          enabled);
gboolean       gimp_color_display_get_enabled     (GimpColorDisplay *display);

GimpColorConfig  * gimp_color_display_get_config  (GimpColorDisplay *display);
GimpColorManaged * gimp_color_display_get_managed (GimpColorDisplay *display);


G_END_DECLS

#endif /* __GIMP_COLOR_DISPLAY_H__ */
