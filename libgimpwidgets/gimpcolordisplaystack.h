/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolordisplaystack.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_COLOR_DISPLAY_STACK_H__
#define __GIMP_COLOR_DISPLAY_STACK_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define GIMP_TYPE_COLOR_DISPLAY_STACK            (gimp_color_display_stack_get_type ())
#define GIMP_COLOR_DISPLAY_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_DISPLAY_STACK, GimpColorDisplayStack))
#define GIMP_COLOR_DISPLAY_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_DISPLAY_STACK, GimpColorDisplayStackClass))
#define GIMP_IS_COLOR_DISPLAY_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_DISPLAY_STACK))
#define GIMP_IS_COLOR_DISPLAY_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_DISPLAY_STACK))
#define GIMP_COLOR_DISPLAY_STACK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_DISPLAY_STACK, GimpColorDisplayStackClass))


typedef struct _GimpColorDisplayStackClass GimpColorDisplayStackClass;

struct _GimpColorDisplayStack
{
  GObject  parent_instance;
};

struct _GimpColorDisplayStackClass
{
  GObjectClass  parent_class;

  void (* changed)   (GimpColorDisplayStack *stack);

  void (* added)     (GimpColorDisplayStack *stack,
                      GimpColorDisplay      *display,
                      gint                   position);
  void (* removed)   (GimpColorDisplayStack *stack,
                      GimpColorDisplay      *display);
  void (* reordered) (GimpColorDisplayStack *stack,
                      GimpColorDisplay      *display,
                      gint                   position);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType                   gimp_color_display_stack_get_type (void) G_GNUC_CONST;
GimpColorDisplayStack * gimp_color_display_stack_new      (void);
GimpColorDisplayStack * gimp_color_display_stack_clone    (GimpColorDisplayStack *stack);

void    gimp_color_display_stack_changed         (GimpColorDisplayStack *stack);

GList * gimp_color_display_stack_get_filters     (GimpColorDisplayStack *stack);

void    gimp_color_display_stack_add             (GimpColorDisplayStack *stack,
                                                  GimpColorDisplay      *display);
void    gimp_color_display_stack_remove          (GimpColorDisplayStack *stack,
                                                  GimpColorDisplay      *display);
void    gimp_color_display_stack_reorder_up      (GimpColorDisplayStack *stack,
                                                  GimpColorDisplay      *display);
void    gimp_color_display_stack_reorder_down    (GimpColorDisplayStack *stack,
                                                  GimpColorDisplay      *display);
void    gimp_color_display_stack_convert_buffer  (GimpColorDisplayStack *stack,
                                                  GeglBuffer            *buffer,
                                                  GeglRectangle         *area);
GIMP_DEPRECATED_FOR(gimp_color_display_stack_convert_buffer)
void    gimp_color_display_stack_convert_surface (GimpColorDisplayStack *stack,
                                                  cairo_surface_t       *surface);
GIMP_DEPRECATED_FOR(gimp_color_display_stack_convert_surface)
void    gimp_color_display_stack_convert         (GimpColorDisplayStack *stack,
                                                  guchar                *buf,
                                                  gint                   width,
                                                  gint                   height,
                                                  gint                   bpp,
                                                  gint                   bpl);

G_END_DECLS

#endif /* __GIMP_COLOR_DISPLAY_STACK_H__ */
