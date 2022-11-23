/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapatternselect.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PATTERN_SELECT_H__
#define __LIGMA_PATTERN_SELECT_H__

G_BEGIN_DECLS

/**
 * LigmaRunPatternCallback:
 * @pattern_name: Name of the pattern
 * @width: width
 * @height: height
 * @bpp: bytes per pixel
 * @mask_data: (array): Mask data
 * @dialog_closing: Dialog closing?
 * @user_data: (closure): user data
 */
typedef void (* LigmaRunPatternCallback)   (const gchar  *pattern_name,
                                           gint          width,
                                           gint          height,
                                           gint          bpp,
                                           const guchar *mask_data,
                                           gboolean      dialog_closing,
                                           gpointer      user_data);


const gchar * ligma_pattern_select_new     (const gchar            *title,
                                           const gchar            *pattern_name,
                                           LigmaRunPatternCallback  callback,
                                           gpointer                data,
                                           GDestroyNotify          data_destroy);
void          ligma_pattern_select_destroy (const gchar            *pattern_callback);


G_END_DECLS

#endif /* __LIGMA_PATTERN_SELECT_H__ */
