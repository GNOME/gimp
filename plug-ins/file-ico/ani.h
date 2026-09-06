/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ANI_H__
#define __ANI_H__

typedef struct _AniFileHeader
{
  guint32 size_of;     /* Num. bytes in this header (incl. size_of itself). */
  guint32 frames;      /* Number of unique icons in the ani cursor. */
  guint32 steps;       /* Number of blits before the animation cycles. */
  guint32 x, y;        /* Reserved, must be 0. */
  guint32 bpp, planes; /* Reserved, must be 0. */
  guint32 jif_rate;    /* Default rate if 'rate' chunk not present, in
                        * jiffies (1/60th of a second). */
  guint32 flags;       /* Flags, see ANI_AF_*. */
} AniFileHeader;

typedef struct _AniSaveInfo
{
  gchar *inam; /* Cursor name metadata */
  gchar *iart; /* Author name metadata */
} AniSaveInfo;

#endif /* __ANI_H__ */
