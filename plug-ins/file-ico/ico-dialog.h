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

#ifndef __ICO_DIALOG_H__
#define __ICO_DIALOG_H__


GtkWidget * ico_dialog_new                 (GimpImage           *image,
                                            GimpProcedure       *procedure,
                                            GimpProcedureConfig *config,
                                            IcoSaveInfo         *info,
                                            AniFileHeader       *ani_header,
                                            AniSaveInfo         *ani_info);
void        ico_dialog_add_icon            (GtkWidget           *dialog,
                                            GimpDrawable        *layer,
                                            gint                 layer_num);

#endif /* __ICO_DIALOG_H__ */
