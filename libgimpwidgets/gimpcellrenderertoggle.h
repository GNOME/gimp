/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacellrenderertoggle.h
 * Copyright (C) 2003-2004  Sven Neumann <sven@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_CELL_RENDERER_TOGGLE_H__
#define __LIGMA_CELL_RENDERER_TOGGLE_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_CELL_RENDERER_TOGGLE            (ligma_cell_renderer_toggle_get_type ())
#define LIGMA_CELL_RENDERER_TOGGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CELL_RENDERER_TOGGLE, LigmaCellRendererToggle))
#define LIGMA_CELL_RENDERER_TOGGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CELL_RENDERER_TOGGLE, LigmaCellRendererToggleClass))
#define LIGMA_IS_CELL_RENDERER_TOGGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CELL_RENDERER_TOGGLE))
#define LIGMA_IS_CELL_RENDERER_TOGGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CELL_RENDERER_TOGGLE))
#define LIGMA_CELL_RENDERER_TOGGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CELL_RENDERER_TOGGLE, LigmaCellRendererToggleClass))


typedef struct _LigmaCellRendererTogglePrivate LigmaCellRendererTogglePrivate;
typedef struct _LigmaCellRendererToggleClass   LigmaCellRendererToggleClass;

struct _LigmaCellRendererToggle
{
  GtkCellRendererToggle          parent_instance;

  LigmaCellRendererTogglePrivate *priv;
};

struct _LigmaCellRendererToggleClass
{
  GtkCellRendererToggleClass  parent_class;

  void (* clicked) (LigmaCellRendererToggle *cell,
                    const gchar            *path,
                    GdkModifierType         state);

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


GType             ligma_cell_renderer_toggle_get_type (void) G_GNUC_CONST;

GtkCellRenderer * ligma_cell_renderer_toggle_new      (const gchar *icon_name);

void    ligma_cell_renderer_toggle_clicked (LigmaCellRendererToggle *cell,
                                           const gchar            *path,
                                           GdkModifierType         state);


G_END_DECLS

#endif /* __LIGMA_CELL_RENDERER_TOGGLE_H__ */
