/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolordisplaystack.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_DISPLAY_STACK_H__
#define __LIGMA_COLOR_DISPLAY_STACK_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define LIGMA_TYPE_COLOR_DISPLAY_STACK            (ligma_color_display_stack_get_type ())
#define LIGMA_COLOR_DISPLAY_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_DISPLAY_STACK, LigmaColorDisplayStack))
#define LIGMA_COLOR_DISPLAY_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_DISPLAY_STACK, LigmaColorDisplayStackClass))
#define LIGMA_IS_COLOR_DISPLAY_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_DISPLAY_STACK))
#define LIGMA_IS_COLOR_DISPLAY_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_DISPLAY_STACK))
#define LIGMA_COLOR_DISPLAY_STACK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_DISPLAY_STACK, LigmaColorDisplayStackClass))


typedef struct _LigmaColorDisplayStackPrivate LigmaColorDisplayStackPrivate;
typedef struct _LigmaColorDisplayStackClass   LigmaColorDisplayStackClass;

struct _LigmaColorDisplayStack
{
  GObject                       parent_instance;

  LigmaColorDisplayStackPrivate *priv;
};

struct _LigmaColorDisplayStackClass
{
  GObjectClass  parent_class;

  void (* changed)   (LigmaColorDisplayStack *stack);

  void (* added)     (LigmaColorDisplayStack *stack,
                      LigmaColorDisplay      *display,
                      gint                   position);
  void (* removed)   (LigmaColorDisplayStack *stack,
                      LigmaColorDisplay      *display);
  void (* reordered) (LigmaColorDisplayStack *stack,
                      LigmaColorDisplay      *display,
                      gint                   position);

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


GType                   ligma_color_display_stack_get_type (void) G_GNUC_CONST;
LigmaColorDisplayStack * ligma_color_display_stack_new      (void);
LigmaColorDisplayStack * ligma_color_display_stack_clone    (LigmaColorDisplayStack *stack);

void    ligma_color_display_stack_changed         (LigmaColorDisplayStack *stack);

GList * ligma_color_display_stack_get_filters     (LigmaColorDisplayStack *stack);

void    ligma_color_display_stack_add             (LigmaColorDisplayStack *stack,
                                                  LigmaColorDisplay      *display);
void    ligma_color_display_stack_remove          (LigmaColorDisplayStack *stack,
                                                  LigmaColorDisplay      *display);
void    ligma_color_display_stack_reorder_up      (LigmaColorDisplayStack *stack,
                                                  LigmaColorDisplay      *display);
void    ligma_color_display_stack_reorder_down    (LigmaColorDisplayStack *stack,
                                                  LigmaColorDisplay      *display);
void    ligma_color_display_stack_convert_buffer  (LigmaColorDisplayStack *stack,
                                                  GeglBuffer            *buffer,
                                                  GeglRectangle         *area);


G_END_DECLS

#endif /* __LIGMA_COLOR_DISPLAY_STACK_H__ */
