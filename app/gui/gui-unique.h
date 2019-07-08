/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GUI_UNIQUE_H__
#define __GUI_UNIQUE_H__

#ifdef G_OS_WIN32
#define GIMP_UNIQUE_WIN32_WINDOW_CLASS  L"GimpWin32UniqueHandler"
#define GIMP_UNIQUE_WIN32_WINDOW_NAME   L"GimpProxy"
#endif


void  gui_unique_init (Gimp *gimp);
void  gui_unique_exit (void);


#endif /* __GUI_UNIQUE_H__ */
