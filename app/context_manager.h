/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CONTEXT_MANAGER_H__
#define __CONTEXT_MANAGER_H__


/*
 *  the list of all images
 */
extern GimpContainer *image_context;

/*
 *  the global data factories which contain the global data lists
 */
extern GimpDataFactory *global_brush_factory;
extern GimpDataFactory *global_pattern_factory;
extern GimpDataFactory *global_gradient_factory;
extern GimpDataFactory *global_palette_factory;

/*
 *  the global tool context
 */
extern GimpContext *global_tool_context;


void   context_manager_init                     (void);
void   context_manager_free                     (void);

void   context_manager_set_global_paint_options (gboolean global);


#endif /* __CONTEXT_MANAGER_H__ */
