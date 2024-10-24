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

#ifndef __ICO_EXPORT_H__
#define __ICO_EXPORT_H__


GimpPDBStatusType ico_export_image        (GFile                *file,
                                           GimpImage            *image,
                                           GimpProcedure        *procedure,
                                           GimpProcedureConfig  *config,
                                           gint32                run_mode,
                                           GError              **error);

GimpPDBStatusType cur_export_image        (GFile                *file,
                                           GimpImage            *image,
                                           GimpProcedure        *procedure,
                                           GimpProcedureConfig  *config,
                                           gint32                run_mode,
                                           gsize                *n_hot_spot_x,
                                           const gint32         *hot_spot_x,
                                           gint32              **new_hot_spot_x,
                                           gsize                *n_hot_spot_y,
                                           const gint32         *hot_spot_y,
                                           gint32              **new_hot_spot_y,
                                           GError              **error);

GimpPDBStatusType ani_export_image        (GFile                *file,
                                           GimpImage            *image,
                                           GimpProcedure        *procedure,
                                           GimpProcedureConfig  *config,
                                           gint32                run_mode,
                                           gsize                *n_hot_spot_x,
                                           const gint32         *hot_spot_x,
                                           gint32              **new_hot_spot_x,
                                           gsize                *n_hot_spot_y,
                                           const gint32         *hot_spot_y,
                                           gint32              **new_hot_spot_y,
                                           AniFileHeader        *header,
                                           AniSaveInfo          *ani_info,
                                           GError              **error);

gboolean          ico_cmap_contains_black (GimpPalette          *cmap);


#endif /* __ICO_EXPORT_H__ */
