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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef HAVE_RINT
#define rint(x) floor (x + 0.5)
#endif

#include "appenv.h"
#include "actionarea.h"
#include "drawable.h"
#include "gdisplay.h"
#include "image_map.h"
#include "interface.h"
#include "posterize.h"
#include "gimplut.h"
#include "lut_funcs.h"

#include "libgimp/gimpintl.h"

#define TEXT_WIDTH 55

/*  the posterize structures  */

typedef struct _Posterize Posterize;
struct _Posterize
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _PosterizeDialog PosterizeDialog;
struct _PosterizeDialog
{
  GtkWidget    *shell;
  GtkWidget    *levels_text;

  GimpDrawable *drawable;
  ImageMap      image_map;
  int           levels;

  gint          preview;

  GimpLut      *lut;
};


/*  the posterize tool options  */
static ToolOptions *posterize_options = NULL;

/* the posterize tool dialog  */
static PosterizeDialog *posterize_dialog = NULL;


/*  posterize action functions  */
static void   posterize_button_press   (Tool *, GdkEventButton *, gpointer);
static void   posterize_button_release (Tool *, GdkEventButton *, gpointer);
static void   posterize_motion         (Tool *, GdkEventMotion *, gpointer);
static void   posterize_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   posterize_control        (Tool *, ToolAction,       gpointer);

static PosterizeDialog * posterize_new_dialog (void);

static void   posterize_preview            (PosterizeDialog *);
static void   posterize_ok_callback        (GtkWidget *, gpointer);
static void   posterize_cancel_callback    (GtkWidget *, gpointer);
static void   posterize_preview_update     (GtkWidget *, gpointer);
static void   posterize_levels_text_update (GtkWidget *, gpointer);
static gint   posterize_delete_callback    (GtkWidget *, GdkEvent *, gpointer);


/*  posterize select action functions  */

static void
posterize_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
posterize_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
}

static void
posterize_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
}

static void
posterize_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

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
tools_new_posterize ()
{
  Tool * tool;
  Posterize * private;

  /*  The tool options  */
  if (! posterize_options)
    {
      posterize_options = tool_options_new (("Posterize Options"));
      tools_register (POSTERIZE, posterize_options);
    }

  /*  The posterize dialog  */
  if (!posterize_dialog)
    posterize_dialog = posterize_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (posterize_dialog->shell))
      gtk_widget_show (posterize_dialog->shell);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Posterize *) g_malloc (sizeof (Posterize));

  tool->type = POSTERIZE;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;

  tool->button_press_func = posterize_button_press;
  tool->button_release_func = posterize_button_release;
  tool->motion_func = posterize_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->modifier_key_func = standard_modifier_key_func;
  tool->cursor_update_func = posterize_cursor_update;
  tool->control_func = posterize_control;

  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_posterize (Tool *tool)
{
  Posterize * post;

  post = (Posterize *) tool->private;

  /*  Close the color select dialog  */
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
    posterize_dialog = posterize_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (posterize_dialog->shell))
      gtk_widget_show (posterize_dialog->shell);
  posterize_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  posterize_dialog->image_map = image_map_create (gdisp, posterize_dialog->drawable);
  if (posterize_dialog->preview)
    posterize_preview (posterize_dialog);
}


/**********************/
/*  Posterize dialog  */
/**********************/

static PosterizeDialog *
posterize_new_dialog ()
{
  PosterizeDialog *pd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *toggle;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), posterize_ok_callback, NULL, NULL },
    { N_("Cancel"), posterize_cancel_callback, NULL, NULL }
  };

  pd = g_malloc (sizeof (PosterizeDialog));
  pd->preview = TRUE;
  pd->levels = 3;
  pd->lut = gimp_lut_new();

  /*  The shell and main vbox  */
  pd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (pd->shell), "posterize", "Gimp");
  gtk_window_set_title (GTK_WINDOW (pd->shell), _("Posterize"));

  gtk_signal_connect (GTK_OBJECT (pd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (posterize_delete_callback),
		      pd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Posterize Levels: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  /*  levels text  */
  pd->levels_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (pd->levels_text), "3");
  gtk_widget_set_usize (pd->levels_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), pd->levels_text, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (pd->levels_text), "changed",
		      (GtkSignalFunc) posterize_levels_text_update,
		      pd);
  gtk_widget_show (pd->levels_text);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) posterize_preview_update,
		      pd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = pd;
  action_items[1].user_data = pd;
  build_action_area (GTK_DIALOG (pd->shell), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (pd->shell);

  return pd;
}

static void
posterize_preview (PosterizeDialog *pd)
{
  if (!pd->image_map)
    g_warning ("posterize_preview(): No image map");
  active_tool->preserve = TRUE;
  posterize_lut_setup(pd->lut, pd->levels, gimp_drawable_bytes(pd->drawable));
  image_map_apply (pd->image_map,  (ImageMapApplyFunc)gimp_lut_process_2,
		   (void *) pd->lut);
  active_tool->preserve = FALSE;
}

static void
posterize_ok_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (pd->shell))
    gtk_widget_hide (pd->shell);

  active_tool->preserve = TRUE;

  if (!pd->preview)
  {
    posterize_lut_setup(pd->lut, pd->levels,
			gimp_drawable_bytes(pd->drawable));
    image_map_apply (pd->image_map, (ImageMapApplyFunc)gimp_lut_process_2,
		     (void *) pd->lut);
  }
  if (pd->image_map)
    image_map_commit (pd->image_map);

  active_tool->preserve = FALSE;

  pd->image_map = NULL;
}

static gint 
posterize_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  posterize_cancel_callback (w, data);

  return TRUE;
}

static void
posterize_cancel_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) client_data;
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
}

static void
posterize_preview_update (GtkWidget *w,
			  gpointer   data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      pd->preview = TRUE;
      posterize_preview (pd);
    }
  else
    pd->preview = FALSE;
}

static void
posterize_levels_text_update (GtkWidget *w,
			      gpointer   data)
{
  PosterizeDialog *pd;
  char *str;
  int value;

  pd = (PosterizeDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), 2, 256);

  if (value != pd->levels)
    {
      pd->levels = value;
      if (pd->preview)
	posterize_preview (pd);
    }
}

