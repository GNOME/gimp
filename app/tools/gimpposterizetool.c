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

#include "tools-types.h"
 
#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpposterizetool.h"
#include "tool_manager.h"

#include "app_procs.h"
#include "image_map.h"

#include "libgimp/gimpintl.h"


typedef struct _PosterizeDialog PosterizeDialog;

struct _PosterizeDialog
{
  GtkWidget     *shell;

  GtkAdjustment *levels_data;

  GimpDrawable  *drawable;
  ImageMap      *image_map;

  gint           levels;

  gboolean       preview;

  GimpLut       *lut;
};


/*  local function prototypes  */

static void   gimp_posterize_tool_class_init (GimpPosterizeToolClass *klass);
static void   gimp_posterize_tool_init       (GimpPosterizeTool      *bc_tool);

static void   gimp_posterize_tool_initialize (GimpTool    *tool,
					      GimpDisplay *gdisp);
static void   gimp_posterize_tool_control    (GimpTool    *tool,
					      ToolAction   action,
					      GimpDisplay *gdisp);

static PosterizeDialog * posterize_dialog_new    (void);

static void   posterize_dialog_hide              (void);
static void   posterize_preview                  (PosterizeDialog *pd);
static void   posterize_reset_callback           (GtkWidget       *widget,
						  gpointer         data);
static void   posterize_ok_callback              (GtkWidget       *widget,
						  gpointer         data);
static void   posterize_cancel_callback          (GtkWidget       *widget,
						  gpointer         data);
static void   posterize_preview_update           (GtkWidget       *widget,
						  gpointer         data);
static void   posterize_levels_adjustment_update (GtkAdjustment   *adjustment,
						  gpointer         data);


static PosterizeDialog *posterize_dialog = NULL;

static GimpImageMapToolClass *parent_class = NULL;


/*  functions  */

void
gimp_posterize_tool_register (Gimp                     *gimp,
                              GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_POSTERIZE_TOOL,
                NULL,
                FALSE,
                "gimp:posterize_tool",
                _("Posterize"),
                _("Reduce image to a fixed numer of colors"),
                N_("/Layer/Colors/Posterize..."), NULL,
                NULL, "tools/posterize.html",
                GIMP_STOCK_TOOL_POSTERIZE);
}

GType
gimp_posterize_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpPosterizeToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_posterize_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPosterizeTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_posterize_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpPosterizeTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_posterize_tool_class_init (GimpPosterizeToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->initialize = gimp_posterize_tool_initialize;
  tool_class->control    = gimp_posterize_tool_control;
}

static void
gimp_posterize_tool_init (GimpPosterizeTool *bc_tool)
{
}

static void
gimp_posterize_tool_initialize (GimpTool    *tool,
				GimpDisplay *gdisp)
{
  if (! gdisp)
    {
      posterize_dialog_hide ();
      return;
    }

  if (gimp_drawable_is_indexed (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Posterize does not operate on indexed drawables."));
      return;
    }

  /*  The posterize dialog  */
  if (!posterize_dialog)
    posterize_dialog = posterize_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (posterize_dialog->shell))
      gtk_widget_show (posterize_dialog->shell);

  posterize_dialog->levels = 3;

  posterize_dialog->drawable = gimp_image_active_drawable (gdisp->gimage);
  posterize_dialog->image_map =
    image_map_create (gdisp, posterize_dialog->drawable);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (posterize_dialog->levels_data), 3);

  if (posterize_dialog->preview)
    posterize_preview (posterize_dialog);
}

static void
gimp_posterize_tool_control (GimpTool    *tool,
			     ToolAction   action,
			     GimpDisplay *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      posterize_dialog_hide ();
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

/**********************/
/*  Posterize dialog  */
/**********************/

static PosterizeDialog *
posterize_dialog_new (void)
{
  PosterizeDialog *pd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *toggle;
  GtkObject *data;

  pd = g_new (PosterizeDialog, 1);
  pd->preview = TRUE;
  pd->levels  = 3;
  pd->lut     = gimp_lut_new ();

  /*  The shell and main vbox  */
  pd->shell =
    gimp_dialog_new (_("Posterize"), "posterize",
		     tool_manager_help_func, NULL,
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     GIMP_STOCK_RESET, posterize_reset_callback,
		     pd, NULL, NULL, TRUE, FALSE,

		     GTK_STOCK_CANCEL, posterize_cancel_callback,
		     pd, NULL, NULL, FALSE, TRUE,

		     GTK_STOCK_OK, posterize_ok_callback,
		     pd, NULL, NULL, TRUE, FALSE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (pd->shell)->vbox), vbox);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Posterize Levels:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  levels spinbutton  */
  data = gtk_adjustment_new (3, 2, 256, 1.0, 10.0, 0.0);
  pd->levels_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (pd->levels_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (pd->levels_data), "value_changed",
                    G_CALLBACK (posterize_levels_adjustment_update),
                    pd);

  gtk_widget_show (spinbutton);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pd->preview);
  gtk_box_pack_end (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (posterize_preview_update),
                    pd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (pd->shell);

  return pd;
}

static void
posterize_dialog_hide (void)
{
  if (posterize_dialog)
    posterize_cancel_callback (NULL, (gpointer) posterize_dialog);
}

static void
posterize_preview (PosterizeDialog *pd)
{
  GimpTool *active_tool;

  active_tool = tool_manager_get_active (the_gimp);

  if (!pd->image_map)
    {
      g_warning ("posterize_preview(): No image map");
      return;
    }

  active_tool->preserve = TRUE;
  posterize_lut_setup (pd->lut, pd->levels, gimp_drawable_bytes (pd->drawable));
  image_map_apply (pd->image_map,  (ImageMapApplyFunc) gimp_lut_process_2,
		   (void *) pd->lut);
  active_tool->preserve = FALSE;
}

static void
posterize_reset_callback (GtkWidget *widget,
			  gpointer   data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) data;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (pd->levels_data), 3);
}

static void
posterize_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  PosterizeDialog *pd;
  GimpTool        *active_tool;

  pd = (PosterizeDialog *) data;

  gtk_widget_hide (pd->shell);

  active_tool = tool_manager_get_active (the_gimp);

  active_tool->preserve = TRUE;

  if (!pd->preview)
    {
      posterize_lut_setup( pd->lut, pd->levels,
			   gimp_drawable_bytes (pd->drawable));
      image_map_apply (pd->image_map, (ImageMapApplyFunc) gimp_lut_process_2,
		       (gpointer) pd->lut);
    }

  if (pd->image_map)
    image_map_commit (pd->image_map);

  active_tool->preserve = FALSE;

  pd->image_map = NULL;

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
posterize_cancel_callback (GtkWidget *widget,
			   gpointer   data)
{
  PosterizeDialog *pd;
  GimpTool        *active_tool;

  pd = (PosterizeDialog *) data;

  gtk_widget_hide (pd->shell);

  active_tool = tool_manager_get_active (the_gimp);

  if (pd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (pd->image_map);
      active_tool->preserve = FALSE;

      pd->image_map = NULL;
      gdisplays_flush ();
    }

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
posterize_preview_update (GtkWidget *widget,
			  gpointer   data)
{
  PosterizeDialog *pd;
  GimpTool        *active_tool;

  pd = (PosterizeDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      pd->preview = TRUE;
      posterize_preview (pd);
    }
  else
    {
      pd->preview = FALSE;
      if (pd->image_map)
	{
	  active_tool = tool_manager_get_active (the_gimp);

	  active_tool->preserve = TRUE;
	  image_map_clear (pd->image_map);
	  active_tool->preserve = FALSE;
	  gdisplays_flush ();
	}
    }
}

static void
posterize_levels_adjustment_update (GtkAdjustment *adjustment,
				    gpointer       data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) data;

  if (pd->levels != adjustment->value)
    {
      pd->levels = adjustment->value;

      if (pd->preview)
	posterize_preview (pd);
    }
}
