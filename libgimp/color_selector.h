/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Colour selector module (C) 1999 Austin Donnelly <austin@greenend.org.uk>
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

#ifndef __COLOR_SELECTOR_H__
#define __COLOR_SELECTOR_H__


/********************************/
/* color selector registration */


/* A function of this type should be called by the color selector each
 * time the user modifies the selected color. */
typedef void (*GimpColorSelector_Callback)(void *data, int r, int g, int b);


/* A function of this type is called to create a new instance of the
 * color selector.  The new selector should set its current color to
 * the RGB triple given (each component is in the range 0 - 255
 * inclusive, with white at 255,255,255 and black at 0,0,0).
 *
 * The selector should call "cb" with argument "data" each time the
 * user modifies the selected color.
 *
 * The selector must return a GtkWidget which implements the color
 * selection UI.  The selector can optionally return "selector_data",
 * an opaque pointer which will be passed in to subsequent invokations
 * on the selector. */
typedef GtkWidget * (*GimpColorSelector_NewFunc)(int r, int g, int b,
						 GimpColorSelector_Callback cb,
						 void *data,
						 /* RETURNS: */
						 void  **selector_data);

/* A function of this type is called when the color selector is no
 * longer required.  This function should not free widgets that are
 * containted within the UI widget returned by new(), since they are
 * destroyed on behalf of the selector by the caller of this
 * function. */
typedef void (*GimpColorSelector_FreeFunc)(void *selector_data);


/* A function of this type is called to change the selector's current
 * color.  The required color is specified as in the new() function.
 * If the "set_current" parameter is FALSE, then only the old color
 * should be set - if "set_current" is TRUE, both the old color and
 * the current color should be set to the RGB triple given.  This
 * function merely gives a hint to the color selector; the selector
 * can choose to ignore this information. */
typedef void (*GimpColorSelector_SetColorFunc)(void *selector_data,
					       int r, int g, int b,
					       int set_current);

typedef struct {
  GimpColorSelector_NewFunc      new;
  GimpColorSelector_FreeFunc     free;
  GimpColorSelector_SetColorFunc setcolor;
} GimpColorSelectorMethods;

typedef void *GimpColorSelectorID;

#ifndef __COLOR_NOTEBOOK_C__	/* Bypass when compiling the source for
				 * these functions. */

/* Register a color selector.  Returns an identifier for the color
 * selector on success, or NULL if the name is already in use.  Both
 * the name and method table are internalised, so may be freed after
 * this call. */
GimpColorSelectorID gimp_color_selector_register (const char *name,
				  GimpColorSelectorMethods *methods);

/* Remove the selector "id" from active service.  New instances of the
 * selector will not be created, but existing ones are allowed to
 * continue.  If "callback" is non-NULL, it will be called once all
 * instances have finished.  The callback could be used to unload
 * dynamiclly loaded code, for example.
 *
 * Returns TRUE on success, FALSE if "id" was not found. */
gboolean gimp_color_selector_unregister (GimpColorSelectorID id,
					 void (*callback)(void *data),
					 void *data);
#endif /* !__COLOR_NOTEBOOK_C__ */

#endif /* __COLOR_SELECTOR_H__ */
