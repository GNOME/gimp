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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __DIALOG_TYPES_H__
#define __DIALOG_TYPES_H__

/* Types of dialog items that can be made.
 */

enum {
  GROUP_ROWS = 1,
  GROUP_COLUMNS,
  GROUP_FORM,
  ITEM_PUSH_BUTTON,
  ITEM_CHECK_BUTTON,
  ITEM_RADIO_BUTTON,
  ITEM_IMAGE_MENU,
  ITEM_SCALE,
  ITEM_FRAME,
  ITEM_LABEL,
  ITEM_TEXT,

  GROUP_RADIO = 1 << 16
};

#endif /* __DIALOG_TYPES_H__ */
