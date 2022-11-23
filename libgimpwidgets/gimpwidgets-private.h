/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmawidgets-private.h
 * Copyright (C) 2003 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_WIDGETS_PRIVATE_H__
#define __LIGMA_WIDGETS_PRIVATE_H__

/* Used to compare similar colors across several widgets. */
#define LIGMA_RGBA_EPSILON 1e-6


typedef gboolean (* LigmaGetColorFunc)      (LigmaRGB *color);
typedef void     (* LigmaEnsureModulesFunc) (void);


extern LigmaHelpFunc          _ligma_standard_help_func;
extern LigmaGetColorFunc      _ligma_get_foreground_func;
extern LigmaGetColorFunc      _ligma_get_background_func;
extern LigmaEnsureModulesFunc _ligma_ensure_modules_func;


G_BEGIN_DECLS


void  ligma_widgets_init              (LigmaHelpFunc           standard_help_func,
                                      LigmaGetColorFunc       get_foreground_func,
                                      LigmaGetColorFunc       get_background_func,
                                      LigmaEnsureModulesFunc  ensure_modules_func,
                                      const gchar           *test_base_dir);

void  ligma_widget_set_identifier     (GtkWidget             *widget,
                                      const gchar           *identifier);
void  ligma_widget_set_bound_property (GtkWidget             *widget,
                                      GObject               *config,
                                      const gchar           *property_name);


G_END_DECLS

#endif /* __LIGMA_WIDGETS_PRIVATE_H__ */
