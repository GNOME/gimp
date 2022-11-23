/* LIGMA - The GNU Image Manipulation Program
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"

#include "paint/ligmamybrushoptions.h"

#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmaviewablebox.h"

#include "ligmamybrushoptions-gui.h"
#include "ligmapaintoptions-gui.h"

#include "ligma-intl.h"


GtkWidget *
ligma_mybrush_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *scale;

  /* the brush */
  button = ligma_prop_mybrush_box_new (NULL, LIGMA_CONTEXT (tool_options),
                                      _("Brush"), 2,
                                      NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* erase mode */
  scale = ligma_prop_check_button_new (config, "eraser", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /* no erasing */
  scale = ligma_prop_check_button_new (config, "no-erasing", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /* radius */
  scale = ligma_prop_spin_scale_new (config, "radius",
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /* opaque */
  scale = ligma_prop_spin_scale_new (config, "opaque",
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /* hardness */
  scale = ligma_prop_spin_scale_new (config, "hardness",
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
