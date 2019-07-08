/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "paint/gimpmybrushoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"

#include "gimpmybrushoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


GtkWidget *
gimp_mybrush_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *scale;

  /* the brush */
  button = gimp_prop_mybrush_box_new (NULL, GIMP_CONTEXT (tool_options),
                                      _("Brush"), 2,
                                      NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* erase mode */
  scale = gimp_prop_check_button_new (config, "eraser", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* no erasing */
  scale = gimp_prop_check_button_new (config, "no-erasing", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* radius */
  scale = gimp_prop_spin_scale_new (config, "radius", NULL,
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* opaque */
  scale = gimp_prop_spin_scale_new (config, "opaque", NULL,
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* hardness */
  scale = gimp_prop_spin_scale_new (config, "hardness", NULL,
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}
