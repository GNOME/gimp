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
#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "image_map.h"
#include "interface.h"
#include "posterize.h"
#include "gimplut.h"
#include "gimpui.h"
#include "lut_funcs.h"

#include "libgimp/gimpintl.h"

/*  the posterize structures  */

typedef struct _Posterize Posterize;

struct _Posterize
{
  gint x, y;    /*  coords for last mouse click  */
};

typedef struct _PosterizeDialog PosterizeDialog;

struct _PosterizeDialog
{
  GtkWidget     *shell;

  GtkAdjustment *levels_data;

  GimpDrawable  *drawable;
  ImageMap       image_map;

  gint           levels;

  gboolean       preview;

  GimpLut       *lut;
};

/*  the posterize tool options  */
static ToolOptions *posterize_options = NULL;

/* the posterize tool dialog  */
static PosterizeDialog *posterize_dialog = NULL;


/*  posterize action functions  */
static void   posterize_control (Tool *, ToolAction, gpointer);

static PosterizeDialog * posterize_dialog_new (void);

static void   posterize_preview                  (PosterizeDialog *);
static void   posterize_reset_callback           (GtkWidget *, gpointer);
static void   posterize_ok_callback              (GtkWidget *, gpointer);
static void   posterize_cancel_callback          (GtkWidget *, gpointer);
static void   posterize_preview_update           (GtkWidget *, gpointer);
static void   posterize_levels_adjustment_update (GtkAdjustment *, gpointer);

/*  posterize select action functions  */

static void
posterize_control (Tool       *tool,
		   ToolAction  action,
		   gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (posterize_dialog)
	posterize_cancel_callback (NULL, (gpointer) posterize_dialog);
      break;

    default:
      break;
    }
}

Tool *
tools_new_posterize (void)
{
  Tool * tool;
  Posterize * private;

  /*  The tool options  */
  if (! posterize_options)
    {
      posterize_options = tool_options_new (("Posterize Options"));
      tools_register (POSTERIZE, posterize_options);
    }

  tool = tools_new_tool (POSTERIZE);
  private = g_new (Posterize, 1);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->control_func = posterize_control;

  return tool;
}

void
tools_free_posterize (Tool *tool)
{
  Posterize * post;

  post = (Posterize *) tool->private;

  /*  Close the posterize dialog  */
  if (posterize_dialog)
    posterize_cancel_callback (NULL, (gpointer) posterize_dialog);

  g_free (post);
}

void
posterize_initialize (GDisplay *gdisp)
{
  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
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

  posterize_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  posterize_dialog->image_map =
    image_map_create (gdisp, posterize_dialog->drawable);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (posterize_dialog->levels_data), 3);

  if (posterize_dialog->preview)
    posterize_preview (posterize_dialog);
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
		     tools_help_func, NULL,
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("OK"), posterize_ok_callback,
		     pd, NULL, NULL, TRUE, FALSE,
		     _("Reset"), posterize_reset_callback,
		     pd, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), posterize_cancel_callback,
		     pd, NULL, NULL, FALSE, TRUE,

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

  gtk_signal_connect (GTK_OBJECT (pd->levels_data), "value_changed",
		      GTK_SIGNAL_FUNC (posterize_levels_adjustment_update),
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

  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (posterize_preview_update),
		      pd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (pd->shell);

  return pd;
}

static void
posterize_preview (PosterizeDialog *pd)
{
  if (!pd->image_map)
    {
      g_warning ("posterize_preview(): No image map");
      return;
    }

  active_tool->preserve = TRUE;
  posterize_lut_setup (pd->lut, pd->levels, gimp_drawable_bytes(pd->drawable));
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

  pd = (PosterizeDialog *) data;

  if (GTK_WIDGET_VISIBLE (pd->shell))
    gtk_widget_hide (pd->shell);

  active_tool->preserve = TRUE;

  if (!pd->preview)
    {
      posterize_lut_setup( pd->lut, pd->levels,
			   gimp_drawable_bytes (pd->drawable));
      image_map_apply (pd->image_map, (ImageMapApplyFunc) gimp_lut_process_2,
		       (void *) pd->lut);
    }

  if (pd->image_map)
    image_map_commit (pd->image_map);

  active_tool->preserve = FALSE;

  pd->image_map = NULL;

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
posterize_cancel_callback (GtkWidget *widget,
			   gpointer   data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) data;

  if (GTK_WIDGET_VISIBLE (pd->shell))
    gtk_widget_hide (pd->shell);

  if (pd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (pd->image_map);
      active_tool->preserve = FALSE;

      pd->image_map = NULL;
      gdisplays_flush ();
    }

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
posterize_preview_update (GtkWidget *widget,
			  gpointer   data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      pd->preview = TRUE;
      posterize_preview (pd);
    }
  else
    pd->preview = FALSE;
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
