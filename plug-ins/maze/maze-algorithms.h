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
 *
 */

#ifndef __MAZE_ALGORITHMS_H__
#define __MAZE_ALGORITHMS_H__


void      mazegen          (gint    pos,
                            guchar *maz,
                            gint    x,
                            gint    y,
                            gint    rnd);
void      mazegen_tileable (gint    pos,
                            guchar *maz,
                            gint    x,
                            gint    y,
                            gint    rnd);

void      prim             (gint    pos,
                            guchar *maz,
                            guint   x,
                            guint   y);
void      prim_tileable    (guchar *maz,
                            guint   x,
                            guint   y);


#endif /* __MAZE_ALGORITHMS_H__ */
