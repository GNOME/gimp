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

#include "actions-types.h"

#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "qmask-actions.h"
#include "qmask-commands.h"

#include "gimp-intl.h"


static GimpActionEntry qmask_actions[] =
{
  { "qmask-configure", NULL,
    N_("_Configure Color and Opacity..."), NULL, NULL,
    G_CALLBACK (qmask_configure_cmd_callback),
    GIMP_HELP_QMASK_EDIT }
};

static GimpToggleActionEntry qmask_toggle_actions[] =
{
  { "qmask-toggle", NULL,
    N_("_QMask Active"), NULL, NULL,
    G_CALLBACK (qmask_toggle_cmd_callback),
    FALSE,
    GIMP_HELP_QMASK_TOGGLE }
};

static GimpRadioActionEntry qmask_invert_actions[] =
{
  { "qmask-invert-on", NULL,
    N_("Mask _Selected Areas"), NULL, NULL,
    TRUE,
    GIMP_HELP_QMASK_INVERT },

  { "qmask-invert-off", NULL,
    N_("Mask _Unselected Areas"), NULL, NULL,
    FALSE,
    GIMP_HELP_QMASK_INVERT }
};


void
qmask_actions_setup (GimpActionGroup *group,
                     gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 qmask_actions,
                                 G_N_ELEMENTS (qmask_actions),
                                 data);

  gimp_action_group_add_toggle_actions (group,
                                        qmask_toggle_actions,
                                        G_N_ELEMENTS (qmask_toggle_actions),
                                        data);

  gimp_action_group_add_radio_actions (group,
                                       qmask_invert_actions,
                                       G_N_ELEMENTS (qmask_invert_actions),
                                       FALSE,
                                       G_CALLBACK (qmask_invert_cmd_callback),
                                       data);
}

void
qmask_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
  GimpDisplay      *gdisp  = NULL;
  GimpDisplayShell *shell  = NULL;
  GimpImage        *gimage = NULL;

  if (GIMP_IS_DISPLAY_SHELL (data))
    {
      shell = GIMP_DISPLAY_SHELL (data);
      gdisp = shell->gdisp;
    }
  else if (GIMP_IS_DISPLAY (data))
    {
      gdisp = GIMP_DISPLAY (data);
      shell = GIMP_DISPLAY_SHELL (gdisp->shell);
    }

  if (gdisp)
    gimage = gdisp->gimage;

#define SET_ACTIVE(action,active) \
        gimp_action_group_set_action_active (group, action, (active))
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, (color), FALSE)

  SET_ACTIVE ("qmask-toggle", gimage->qmask_state);

  if (gimage->qmask_inverted)
    SET_ACTIVE ("qmask-invert-on", TRUE);
  else
    SET_ACTIVE ("qmask-invert-off", TRUE);

  SET_COLOR ("qmask-configure", &gimage->qmask_color);

#undef SET_SENSITIVE
#undef SET_COLOR
}
