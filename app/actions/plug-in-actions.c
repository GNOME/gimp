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

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "plug-in/plug-ins.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


static GimpActionEntry plug_in_actions[] =
{
  { "plug-in-menu",                NULL, N_("/Filte_rs")      },
  { "plug-in-blur-menu",           NULL, N_("_Blur")          },
  { "plug-in-colors-menu",         NULL, N_("_Colors")        },
  { "plug-in-colors-map-menu",     NULL, N_("Ma_p")           },
  { "plug-in-noise-menu",          NULL, N_("_Noise")         },
  { "pluf-in-edge-detect-menu",    NULL, N_("Edge-De_tect")   },
  { "plug-in-enhance-menu",        NULL, N_("En_hance")       },
  { "plug-in-generic-menu",        NULL, N_("_Generic")       },
  { "plug-in-glass-effects-menu",  NULL, N_("Gla_ss Effects") },
  { "plug-in-light-effects-menu",  NULL, N_("_Light Effects") },
  { "plug-in-distorts-menu",       NULL, N_("_Distorts")      },
  { "plug-in-artistic-menu",       NULL, N_("_Artistic")      },
  { "plug-in-map-menu",            NULL, N_("_Map")           },
  { "plug-in-render-menu",         NULL, N_("_Render")        },
  { "plug-in-render-clouds-menu",  NULL, N_("_Clouds")        },
  { "plug-in-render-nature-menu",  NULL, N_("_Nature")        },
  { "plug-in-render-pattern-menu", NULL, N_("_Pattern")       },
  { "plug-in-web-menu",            NULL, N_("_Web")           },
  { "plug-in-animation-menu",      NULL, N_("An_imation")     },
  { "plug-in-combine-menu",        NULL, N_("C_ombine")       },
  { "plug-in-toys-menu",           NULL, N_("To_ys")          }
};

static GimpEnumActionEntry plug_in_repeat_actions[] =
{
  { "plug-in-repeat", GTK_STOCK_EXECUTE,
    N_("Repeat Last"), "<control>F", NULL,
    FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "plug-in-reshow", GIMP_STOCK_RESHOW_FILTER,
    N_("Re-Show Last"), "<control><shift>F", NULL,
    TRUE,
    GIMP_HELP_FILTER_RESHOW }
};


void
plug_in_actions_setup (GimpActionGroup *group,
                       gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions),
                                 data);

  gimp_action_group_add_enum_actions (group,
                                      plug_in_repeat_actions,
                                      G_N_ELEMENTS (plug_in_repeat_actions),
                                      G_CALLBACK (plug_in_repeat_cmd_callback),
                                      data);
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
}
