/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-icc-profile.h
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PLUG_IN_ICC_PROFILE_H__
#define __PLUG_IN_ICC_PROFILE_H__


gboolean  plug_in_icc_profile_apply_rgb (GimpImage     *image,
                                         GimpContext   *context,
                                         GimpProgress  *progress,
                                         GimpRunMode    run_mode,
                                         GError       **error);


#endif /* __PLUG_IN_ICC_PROFILE_H__ */
