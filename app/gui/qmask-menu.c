/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "actions/qmask-commands.h"

#include "qmask-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry qmask_menu_entries[] =
{
  { { N_("/_QMask Active"), NULL,
      qmask_toggle_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_QMASK_TOGGLE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Mask _Selected Areas"), NULL,
      qmask_invert_cmd_callback, TRUE, "<RadioItem>" },
    NULL,
    GIMP_HELP_QMASK_INVERT, NULL },
  { { N_("/Mask _Unselected Areas"), NULL,
      qmask_invert_cmd_callback, FALSE, "/Mask Selected Areas" },
    NULL,
    GIMP_HELP_QMASK_INVERT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_Configure Color and Opacity..."), NULL,
      qmask_configure_cmd_callback, 0 },
    NULL,
    GIMP_HELP_QMASK_EDIT, NULL }
};

gint n_qmask_menu_entries = G_N_ELEMENTS (qmask_menu_entries);


void
qmask_menu_update (GtkItemFactory *factory,
                   gpointer        data)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (data);

#define SET_ACTIVE(menu,active) \
        gimp_item_factory_set_active (factory, "/" menu, (active))
#define SET_COLOR(menu,color) \
        gimp_item_factory_set_color (factory, "/" menu, (color), FALSE)

  SET_ACTIVE ("QMask Active", shell->gdisp->gimage->qmask_state);

  if (shell->gdisp->gimage->qmask_inverted)
    SET_ACTIVE ("Mask Selected Areas", TRUE);
  else
    SET_ACTIVE ("Mask Unselected Areas", TRUE);

  SET_COLOR ("Configure Color and Opacity...",
             &shell->gdisp->gimage->qmask_color);

#undef SET_SENSITIVE
#undef SET_COLOR
}
