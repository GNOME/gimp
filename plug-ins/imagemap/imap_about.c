/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "imap_about.h"
#include "imap_default_dialog.h"

void
do_about_dialog(void)
{
   static DefaultDialog_t *dialog;
   if (!dialog) {
      dialog = make_default_dialog("About");
      default_dialog_hide_cancel_button(dialog);
      default_dialog_hide_apply_button(dialog);
      default_dialog_set_label(
	 dialog,
	 "Imagemap plug-in 1.2\n\n"
	 "Copyright(c) 1999 by Maurits Rijk\n"
	 "lpeek.mrijk@consunet.nl \n\n"
	 "  Released under the GNU General Public License  ");
   }
   default_dialog_show(dialog);
}
