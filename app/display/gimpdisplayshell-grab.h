/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdisplayshell-grab.h
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

#ifndef __GIMP_DISPLAY_SHELL_GRAB_H__
#define __GIMP_DISPLAY_SHELL_GRAB_H__


gboolean   gimp_display_shell_pointer_grab    (GimpDisplayShell *shell,
                                               const GdkEvent   *event,
                                               GdkEventMask      event_mask);
void       gimp_display_shell_pointer_ungrab  (GimpDisplayShell *shell,
                                               const GdkEvent   *event);


#endif /* __GIMP_DISPLAY_SHELL_GRAB_H__ */
