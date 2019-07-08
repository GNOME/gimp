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

#ifndef __SCREENSHOT_OSX_H__
#define __SCREENSHOT_OSX_H__


#ifdef PLATFORM_OSX

gboolean               screenshot_osx_available        (void);

ScreenshotCapabilities screenshot_osx_get_capabilities (void);

GimpPDBStatusType      screenshot_osx_shoot            (ScreenshotValues  *shootvals,
                                                        GdkScreen         *screen,
                                                        gint32            *image_ID,
                                                        GError           **error);

#endif /* PLATFORM_OSX */


#endif /* __SCREENSHOT_OSX_H__ */
