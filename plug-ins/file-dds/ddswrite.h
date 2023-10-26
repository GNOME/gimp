/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
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

#ifndef __DDSWRITE_H__
#define __DDSWRITE_H__


extern GimpPDBStatusType write_dds (GFile               *file,
                                    GimpImage           *image,
                                    GimpDrawable        *drawable,
                                    gboolean             interactive,
                                    GimpProcedure       *procedure,
                                    GimpProcedureConfig *config,
                                    gboolean             is_duplicate_image);


#endif /* __DDSWRITE_H__ */
