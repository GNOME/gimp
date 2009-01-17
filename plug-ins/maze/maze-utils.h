/*
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MAZE_UTILS_H__
#define __MAZE_UTILS_H__


void   get_colors (GimpDrawable *drawable,
                   guint8       *fg,
                   guint8       *bg);
void   drawbox    (GimpPixelRgn *dest_rgn,
                   guint         x,
                   guint         y,
                   guint         w,
                   guint         h,
                   guint8        clr[4]);


#endif /* __MAZE_UTILS_H__ */
