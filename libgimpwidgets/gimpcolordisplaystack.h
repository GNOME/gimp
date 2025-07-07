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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_DISPLAY_STACK_H__
#define __GIMP_COLOR_DISPLAY_STACK_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define GIMP_TYPE_COLOR_DISPLAY_STACK (gimp_color_display_stack_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorDisplayStack, gimp_color_display_stack, GIMP, COLOR_DISPLAY_STACK, GObject)


GimpColorDisplayStack * gimp_color_display_stack_new      (void);
GimpColorDisplayStack * gimp_color_display_stack_clone    (GimpColorDisplayStack *stack);

void    gimp_color_display_stack_changed         (GimpColorDisplayStack *stack);

GIMP_DEPRECATED_FOR(g_list_model_get_item)
GList * gimp_color_display_stack_get_filters     (GimpColorDisplayStack *stack);

GimpColorDisplay *
        gimp_color_display_stack_get_display     (GimpColorDisplayStack *stack,
                                                  guint                  index);
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
gboolean
        gimp_color_display_stack_is_any_enabled  (GimpColorDisplayStack *stack);


G_END_DECLS

#endif /* __GIMP_COLOR_DISPLAY_STACK_H__ */
