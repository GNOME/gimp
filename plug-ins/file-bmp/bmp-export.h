/*
 * GIMP - The GNU Image Manipulation Program
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

#ifndef __BMP_EXPORT_H__
#define __BMP_EXPORT_H__


typedef enum
{
  RGB_565,
  RGBA_5551,
  RGB_555,
  RGB_888,
  RGBA_8888,
  RGBX_8888
} RGBMode;


GimpPDBStatusType   export_image (GFile         *file,
                                  GimpImage     *image,
                                  GimpDrawable  *drawable,
                                  GimpRunMode    run_mode,
                                  GimpProcedure *procedure,
                                  GObject       *config,
                                  GError       **error);


#endif /* __BMP_EXPORT_H__ */
