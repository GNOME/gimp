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

#ifndef __GIMP_EDIT_SELECTION_TOOL_H__
#define __GIMP_EDIT_SELECTION_TOOL_H__


typedef enum
{
  EDIT_VECTORS_TRANSLATE,
  EDIT_CHANNEL_TRANSLATE,
  EDIT_LAYER_MASK_TRANSLATE,
  EDIT_MASK_TRANSLATE,
  EDIT_MASK_TO_LAYER_TRANSLATE,
  EDIT_MASK_COPY_TO_LAYER_TRANSLATE,
  EDIT_LAYER_TRANSLATE,
  EDIT_FLOATING_SEL_TRANSLATE
} EditType;


void   init_edit_selection (GimpTool       *tool,
                            GimpDisplay    *gdisp,
                            GimpCoords     *coords,
                            EditType        edit_type);


void   gimp_edit_selection_tool_key_press (GimpTool       *tool,
                                           GdkEventKey    *kevent,
                                           GimpDisplay    *gdisp);


#endif  /*  __GIMP_EDIT_SELECTION_TOOL_H__  */
