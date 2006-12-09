/* GIMP - The GNU Image Manipulation Program
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

#ifndef  __GIMP_DRAWABLE_FOREGROUND_EXTRACT_H__
#define  __GIMP_DRAWABLE_FOREGROUND_EXTRACT_H__


/*  general API (as seen from the PDB)  */

void       gimp_drawable_foreground_extract (GimpDrawable              *drawable,
                                             GimpForegroundExtractMode  mode,
                                             GimpDrawable              *mask,
                                             GimpProgress              *progress);

/*  SIOX specific API  */

SioxState * gimp_drawable_foreground_extract_siox_init   (GimpDrawable *drawable,
                                                          gint          x,
                                                          gint          y,
                                                          gint          width,
                                                          gint          height);
void        gimp_drawable_foreground_extract_siox  (GimpDrawable       *mask,
                                                    SioxState          *state,
                                                    SioxRefinementType  refinemane,
                                                    gint                smoothness,
                                                    const gdouble       sensitivity[3],
                                                    gboolean            multiblob,
                                                    GimpProgress       *progress);
void        gimp_drawable_foreground_extract_siox_done (SioxState      *state);


#endif  /*  __GIMP_DRAWABLE_FOREGROUND_EXTRACT_H__  */
