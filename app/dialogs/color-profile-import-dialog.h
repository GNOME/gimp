/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-import-dialog.h
 * Copyright (C) 2015 Michael Natterer <mitch@ligma.org>
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

#ifndef __COLOR_PROFILE_IMPORT_DIALOG_H__
#define __COLOR_PROFILE_IMPORT_DIALOG_H__


LigmaColorProfilePolicy
    color_profile_import_dialog_run (LigmaImage                 *image,
                                     LigmaContext               *context,
                                     GtkWidget                 *parent,
                                     LigmaColorProfile         **dest_profile,
                                     LigmaColorRenderingIntent  *intent,
                                     gboolean                  *bpc,
                                     gboolean                  *dont_ask);


#endif  /*  __COLOR_PROFILE_IMPORT_DIALOG_H__  */
