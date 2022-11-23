/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolordisplay.c
 * Copyright (C) 1999 Manish Singh <yosh@ligma.org>
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

#ifndef __LIGMA_COLOR_DISPLAY_H__
#define __LIGMA_COLOR_DISPLAY_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define LIGMA_TYPE_COLOR_DISPLAY            (ligma_color_display_get_type ())
#define LIGMA_COLOR_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_DISPLAY, LigmaColorDisplay))
#define LIGMA_COLOR_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_DISPLAY, LigmaColorDisplayClass))
#define LIGMA_IS_COLOR_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_DISPLAY))
#define LIGMA_IS_COLOR_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_DISPLAY))
#define LIGMA_COLOR_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_DISPLAY, LigmaColorDisplayClass))


typedef struct _LigmaColorDisplayPrivate LigmaColorDisplayPrivate;
typedef struct _LigmaColorDisplayClass   LigmaColorDisplayClass;

struct _LigmaColorDisplay
{
  GObject                 parent_instance;

  LigmaColorDisplayPrivate *priv;
};

struct _LigmaColorDisplayClass
{
  GObjectClass  parent_class;

  const gchar  *name;
  const gchar  *help_id;
  const gchar  *icon_name;

  /*  virtual functions  */
  void        (* convert_buffer) (LigmaColorDisplay *display,
                                  GeglBuffer       *buffer,
                                  GeglRectangle    *area);
  GtkWidget * (* configure)      (LigmaColorDisplay *display);

  /*  signals  */
  void        (* changed)        (LigmaColorDisplay *display);

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


GType              ligma_color_display_get_type        (void) G_GNUC_CONST;

LigmaColorDisplay * ligma_color_display_clone           (LigmaColorDisplay *display);

void               ligma_color_display_convert_buffer  (LigmaColorDisplay *display,
                                                       GeglBuffer       *buffer,
                                                       GeglRectangle    *area);
void               ligma_color_display_load_state      (LigmaColorDisplay *display,
                                                       LigmaParasite     *state);
LigmaParasite     * ligma_color_display_save_state      (LigmaColorDisplay *display);
GtkWidget        * ligma_color_display_configure       (LigmaColorDisplay *display);
void               ligma_color_display_configure_reset (LigmaColorDisplay *display);

void               ligma_color_display_changed         (LigmaColorDisplay *display);

void               ligma_color_display_set_enabled     (LigmaColorDisplay *display,
                                                       gboolean          enabled);
gboolean           ligma_color_display_get_enabled     (LigmaColorDisplay *display);

LigmaColorConfig  * ligma_color_display_get_config      (LigmaColorDisplay *display);
LigmaColorManaged * ligma_color_display_get_managed     (LigmaColorDisplay *display);


G_END_DECLS

#endif /* __LIGMA_COLOR_DISPLAY_H__ */
