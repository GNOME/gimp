/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpuitypes.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_UI_TYPES_H__
#define __GIMP_UI_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the html documentation */


typedef enum
{
  GIMP_SIZE_ENTRY_UPDATE_NONE       = 0,
  GIMP_SIZE_ENTRY_UPDATE_SIZE       = 1,
  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} GimpSizeEntryUpdatePolicy;

typedef struct _GimpColorButton     GimpColorButton;
typedef struct _GimpPathEditor      GimpPathEditor;
typedef struct _GimpSizeEntry       GimpSizeEntry;
typedef struct _GimpUnitMenu        GimpUnitMenu;

typedef void (* GimpHelpFunc) (const gchar *help_data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_TYPES_H__ */
