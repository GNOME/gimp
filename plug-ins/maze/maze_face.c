/* -*- mode: C; c-file-style: "gnu"; c-basic-offset: 2; -*- */
/* $Id$
 * User interface for plug-in-maze.
 * 
 * Implemented as a GIMP 0.99 Plugin by 
 * Kevin Turner <acapnotic@users.sourceforge.net>
 * http://gimp-plug-ins.sourceforge.net/maze/
 */

/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef SOLO_COMPILE
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "maze.h"

#include "libgimp/stdplugins-intl.h"


#define BORDER_TOLERANCE 1.00 /* maximum ratio of (max % divs) to width */
#define ENTRY_WIDTH 75

/* entscale stuff begin */
/* FIXME: Entry-Scale stuff is probably in libgimpui by now.  
          Should use that instead.*/
/* Indeed! By using the gimp_scale_entry you could get along without the
   EntscaleIntData structure, since it has accessors to all objects  (Sven) */

#define ENTSCALE_INT_SCALE_WIDTH 125
#define ENTSCALE_INT_ENTRY_WIDTH 40

#define MORE  1
#define LESS -1

typedef void (*EntscaleIntCallbackFunc) (gint value, gpointer data);

typedef struct
{
  GtkObject               *adjustment;
  GtkWidget               *entry;
  gint                     constraint;
  EntscaleIntCallbackFunc  callback;
  gpointer	           call_data;
} EntscaleIntData;
/* entscale stuff end */


/* one buffer fits all */
#define BUFSIZE 128  
gchar buffer[BUFSIZE];


gint        maze_dialog         (void);

static void maze_msg            (gchar     *msg);
static void maze_ok_callback    (GtkWidget *widget,
				 gpointer   data);
static void maze_help           (GtkWidget *widget,
				 gpointer   foo);
#ifdef SHOW_PRNG_PRIVATES
static void maze_entry_callback (GtkWidget *widget,
				 gpointer   data);
#endif

/* Looking back, it would probably have been easier to completely
 * re-write the whole entry/scale thing to work with the divbox stuff.
 * It would undoubtably be cleaner code.  But since I already *had*
 * the entry/scale routines, I was under the (somewhat mistaken)
 * impression that it would be easier to work with them... */

/* Now, it goes like this:

   To update entscale (width) when div_entry changes:

    entscale_int_new has been slightly modified to return a pointer to
      its entry widget.

    This is fed to divbox_new as a "friend", which is in turn fed to
      the div_entry_callback routine.  And that's not really so bad,
      except...

    Oh, well, maybe it isn't so bad.  We can play with our friend's
      userdata to block his callbacks so we don't get feedback loops,
      that works nicely enough.

   To update div_entry when entscale (width) changes:

    The entry/scale setup graciously provides for callbacks.  However,
      this means we need to know about div_entry when we set up
      entry/scale, which we don't...  Chicken and egg problem.  So we
      set up a pointer to where div_entry will be, and pass this
      through to divbox_new when it happens.

    We need to block signal handlers for div_entry this time.  We
      happen to know that div_entry's callback data is our old
      "friend", so we pull our friend out from where we stuck him in
      the entry's userdata...  Hopefully that does it.  */

/* Questions:

     Gosh that was dumb.  Is there a way to
       signal_handler_block_by_name?
     That would make life so much nicer.

         You could pass the handler_id around and use 
         gtk_signal_handler_block ().   (Sven)
*/

static void div_button_callback   (GtkWidget *button, GtkWidget *entry);
static void div_entry_callback    (GtkWidget *entry, GtkWidget *friend);
static void height_width_callback (gint width, GtkWidget **div_entry);

static GtkWidget* divbox_new (guint *max,
			      GtkWidget *friend, 
			      GtkWidget **div_entry);

#if 0
static void div_buttonl_callback (GtkObject *object);
static void div_buttonr_callback (GtkObject *object);
#endif 

/* entscale stuff begin */
static GtkWidget*   entscale_int_new ( GtkWidget *table, gint x, gint y,
				       gchar *caption, gint *intvar, 
				       gint min, gint max, gboolean constraint,
				       EntscaleIntCallbackFunc callback,
				       gpointer data );

static void   entscale_int_destroy_callback (GtkWidget     *widget,
					     gpointer       data);
static void   entscale_int_scale_update     (GtkAdjustment *adjustment,
					     gpointer       data);
static void   entscale_int_entry_update     (GtkWidget     *widget,
					     gpointer       data);

#define ISODD(X) ((X) & 1)
/* entscale stuff end */

extern MazeValues mvals;
extern guint      sel_w, sel_h;

static gint       maze_run = FALSE;
static GtkWidget *msg_label;

gint
maze_dialog (void) 
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *tilecheck;

  GtkWidget *width_entry, *height_entry;
  GtkWidget *seed_hbox;
  GtkWidget *div_x_hbox, *div_y_hbox;
  GtkWidget *div_x_entry, *div_y_entry;

  gint    trow = 0;
  gchar **argv;
  gint    argc;
  gchar  *message;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("maze");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  gdk_set_use_xshm(gimp_use_xshm());
  gimp_help_init();

  dlg = gimp_dialog_new (MAZE_TITLE, "maze", 
			 gimp_plugin_help_func, "filters/maze.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), maze_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Help"), maze_help,
			 NULL, NULL, NULL, FALSE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER(frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  
#ifdef SHOW_PRNG_PRIVATES
  table = gtk_table_new (8, 3, FALSE);
#else
  table = gtk_table_new (6, 3, FALSE);
#endif  
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* entscale == Entry and Scale pair function found in pixelize.c */
  width_entry = entscale_int_new (table, 0, trow, _("Width (Pixels):"), 
				  &mvals.width, 
				  1, sel_w/4, TRUE, 
				  (EntscaleIntCallbackFunc) height_width_callback,
				  &div_x_entry);
  trow++;

  /* Number of Divisions entry */
  div_x_hbox = divbox_new (&sel_w, width_entry, &div_x_entry);
  g_snprintf (buffer, BUFSIZE, "%d", (sel_w / mvals.width));
  gtk_entry_set_text (GTK_ENTRY (div_x_entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow, 
			     _("Pieces:"), 1.0, 0.5,
			     div_x_hbox, 1, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), trow, 8);
  trow++;

  height_entry = entscale_int_new (table, 0, trow, _("Height (Pixels):"), 
				   &mvals.height, 
				   1, sel_h/4, TRUE, 
				   (EntscaleIntCallbackFunc) height_width_callback,
				   &div_y_entry);
  trow++;

  div_y_hbox = divbox_new (&sel_h, height_entry, &div_y_entry);
  g_snprintf (buffer, BUFSIZE, "%d", (sel_h / mvals.height));
  gtk_entry_set_text (GTK_ENTRY (div_y_entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow, 
			     _("Pieces:"), 1.0, 0.5,
			     div_y_hbox, 1, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), trow, 8);
  trow++;

#ifdef SHOW_PRNG_PRIVATES
  /* Multiple input box */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, BUFSIZE, "%d", mvals.multiple);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow, 
			     _("Multiple (57):"), 1.0, 0.5,
			     entry, 1, FALSE);
  trow++;
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.multiple);

  /* Offset input box */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, BUFSIZE, "%d", mvals.offset);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow, 
			     _("Offset (1):"), 1.0, 0.5,
			     entry, 1, FALSE);
  trow++;
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.offset);
#endif

  /* Tileable checkbox */
  tilecheck = gtk_check_button_new_with_label (_("Tileable"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tilecheck), mvals.tile);
  gtk_signal_connect (GTK_OBJECT (tilecheck), "clicked",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update), 
		      &mvals.tile);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow, 
			     NULL, 1.0, 0.5,
			     tilecheck, 1, FALSE);
  trow++;

  /* Seed input box */
  seed_hbox = gimp_random_seed_new (&mvals.seed,
				    &mvals.timeseed, 
				    TRUE, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow, 
			     _("Seed:"), 1.0, 0.5,
			     seed_hbox, 1, TRUE);
  trow++;

  /* Algorithm Choice */
  frame = gimp_radio_group_new (TRUE, _("Style"),

				_("Depth First"), 
				gimp_radio_button_update,
				&mvals.algorithm,
				(gpointer) DEPTH_FIRST,
				NULL, 
				(mvals.algorithm == DEPTH_FIRST),

				_("Prim's Algorithm"), 
				gimp_radio_button_update,
				&mvals.algorithm,
				(gpointer) PRIMS_ALGORITHM,
				NULL, 
				(mvals.algorithm == PRIMS_ALGORITHM),

				NULL);

  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER(frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), frame, FALSE, FALSE, 0);

  /* Message label */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER(frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), frame, FALSE, FALSE, 0);
  
  message = g_strdup_printf (_("Selection is %dx%d"), sel_w, sel_h);
  msg_label = gtk_label_new (message);
  g_free (message);
  gtk_misc_set_padding (GTK_MISC (msg_label), 4, 4);
  gtk_misc_set_alignment (GTK_MISC (msg_label), 0.5, 0.5);
  gtk_container_add (GTK_CONTAINER (frame), msg_label);

  gtk_widget_show_all (dlg);

  gtk_main ();
  gdk_flush ();

  return maze_run;
}

static GtkWidget*
divbox_new (guint      *max,
	    GtkWidget  *friend,
	    GtkWidget **div_entry)
{
  GtkWidget *align;
  GtkWidget *hbox;
  GtkWidget *arrowl, *arrowr;
  GtkWidget *buttonl, *buttonr;
#if DIVBOX_LOOKS_LIKE_SPINBUTTON
  GtkWidget *buttonbox;
#endif

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), hbox);

#if DIVBOX_LOOKS_LIKE_SPINBUTTON     
  arrowl = gtk_arrow_new (GTK_ARROW_DOWN,  GTK_SHADOW_OUT);
  arrowr = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
#else
  arrowl = gtk_arrow_new (GTK_ARROW_LEFT,  GTK_SHADOW_IN);
  arrowr = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_IN);
#endif

  buttonl = gtk_button_new ();
  buttonr = gtk_button_new ();
     
  gtk_object_set_data (GTK_OBJECT (buttonl), "direction", GINT_TO_POINTER (LESS));
  gtk_object_set_data (GTK_OBJECT (buttonr), "direction", GINT_TO_POINTER (MORE));

  *div_entry = gtk_entry_new ();

  gtk_object_set_data (GTK_OBJECT (*div_entry), "max", max);
  gtk_object_set_data (GTK_OBJECT (*div_entry), "friend", friend);

  gtk_container_add (GTK_CONTAINER (buttonl), arrowl);
  gtk_container_add (GTK_CONTAINER (buttonr), arrowr);

  gtk_widget_set_usize (*div_entry, ENTRY_WIDTH, 0);

#if DIVBOX_LOOKS_LIKE_SPINBUTTON
  buttonbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (buttonbox), buttonr, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonl, FALSE, FALSE, 0);
  gtk_widget_show (buttonbox);

  gtk_box_pack_start (GTK_BOX (hbox), *div_entry, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), buttonbox, FALSE, FALSE, 0);
#else
  gtk_misc_set_padding (GTK_MISC (arrowl), 2, 2);
  gtk_misc_set_padding (GTK_MISC (arrowr), 2, 2);

  gtk_box_pack_start (GTK_BOX (hbox), buttonl, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), *div_entry, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), buttonr, FALSE, FALSE, 0);
#endif     

  gtk_widget_show_all (hbox);

  gtk_signal_connect (GTK_OBJECT (buttonl), "clicked",
		      (GtkSignalFunc) div_button_callback, 
		      *div_entry);
  gtk_signal_connect (GTK_OBJECT (buttonr), "clicked",
		      (GtkSignalFunc) div_button_callback, 
		      *div_entry);
  gtk_signal_connect (GTK_OBJECT (*div_entry), "changed",
		      (GtkSignalFunc) div_entry_callback,
		      friend);

  return align;
}

static void
div_button_callback (GtkWidget *button,
		     GtkWidget *entry)
{
  guint  max, divs;
  gchar *text;
  gint   direction;

  direction = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (button), "direction"));
  max = *((guint*) gtk_object_get_data (GTK_OBJECT (entry), "max"));

  /* Tileable mazes shall have only an even number of divisions.
     Other mazes have odd. */

  /* Sanity check: */
  if (mvals.tile && ISODD(max))
    {
      maze_msg (_("Selection size is not even.\n"
		  "Tileable maze won't work perfectly."));
      return;
    }

  text = gtk_entry_get_text (GTK_ENTRY (entry));

  divs = atoi (text);

  if (divs <= 3)
    {
      divs = mvals.tile ? 
	max - (ISODD(max) ? 1 : 0) : 
	max - (ISODD(max) ? 0 : 1);
    }
  else if (divs > max)
    {
      divs = mvals.tile ? 6 : 5;
    }
     
  /* Makes sure we're appropriately even or odd, adjusting in the
     proper direction. */

  divs += direction * (mvals.tile ? (ISODD(divs) ? 1 : 0) :
                                    (ISODD(divs) ? 0 : 1));
	  
  if (mvals.tile)
    {
      if (direction > 0)
	{
	  do
	    {
	      divs += 2;
	      if (divs > max)
		divs = 4;
	    }
	  while (max % divs);
	}
      else
	{ /* direction < 0 */
	  do
	    {
	      divs -= 2;
	      if (divs < 4)
		divs = max - (max & 1);
	    }
	  while (max % divs);
	} /* endif direction < 0 */
    }
  else
    { /* If not tiling, having a non-zero remainder doesn't bother us much. */
      if (direction > 0)
	{
	  do
	    {
	      divs += 2;
	    }
	  while ((max % divs > max / divs * BORDER_TOLERANCE) && divs < max);
	}
      else
	{ /* direction < 0 */
	  do
	    {
	      divs -= 2;
	    }
	  while ((max % divs > max / divs * BORDER_TOLERANCE) && divs > 5);
	} /* endif direction < 0 */
    } /* endif not tiling */

  if (divs <= 3)
    {
      divs = mvals.tile ? 
	max - (ISODD(max) ? 1 : 0) : 
	max - (ISODD(max) ? 0 : 1);
    }
  else if (divs > max)
    {
      divs = mvals.tile ? 4 : 5;
    }

  g_snprintf (buffer, BUFSIZE, "%d", divs);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  return;
}

static void
div_entry_callback (GtkWidget *entry,
		    GtkWidget *friend)
{
  guint divs, width, max;
  EntscaleIntData *userdata;
  EntscaleIntCallbackFunc friend_callback;

  divs = atoi (gtk_entry_get_text (GTK_ENTRY (entry)));
  if (divs < 4) /* If this is under 4 (e.g. 0), something's weird.      */
    return;     /* But it'll probably be ok, so just return and ignore. */

  max = *((guint*) gtk_object_get_data (GTK_OBJECT (entry), "max"));     

  /* I say "width" here, but it could be height.*/

  width = max / divs;
  g_snprintf (buffer, BUFSIZE, "%d", width);

  /* No tagbacks from our friend... */
  userdata = gtk_object_get_user_data (GTK_OBJECT (friend));
  friend_callback = userdata->callback;
  userdata->callback = NULL;

  gtk_entry_set_text (GTK_ENTRY (friend), buffer);

  userdata->callback = friend_callback;
}

static void
height_width_callback (gint        width,
		       GtkWidget **div_entry)
{
  guint divs, max;
  gpointer data;

  max = *((guint*) gtk_object_get_data(GTK_OBJECT(*div_entry), "max"));
  divs = max / width;

  g_snprintf (buffer, BUFSIZE, "%d", divs );

  data = gtk_object_get_data (GTK_OBJECT(*div_entry), "friend");
  gtk_signal_handler_block_by_data (GTK_OBJECT (*div_entry), data );
     
  gtk_entry_set_text (GTK_ENTRY (*div_entry), buffer);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT(*div_entry), data );
}

static void
maze_help (GtkWidget *widget,
	   gpointer   foo)
{
  gchar *proc_blurb, *proc_help, *proc_author, *proc_copyright, *proc_date;
  gint proc_type, nparams, nreturn_vals;
  GParamDef *params, *return_vals;
  gint baz;
  gchar *message;

  if (gimp_query_procedure ("extension_web_browser",
			    &proc_blurb, &proc_help, 
			    &proc_author, &proc_copyright, &proc_date,
			    &proc_type, &nparams, &nreturn_vals,
			    &params, &return_vals)) 
    {
      /* open URL for help */ 
      message = g_strdup_printf (_("Opening %s"), MAZE_URL);
      maze_msg (message);
      g_free (message);
      gimp_run_procedure ("extension_web_browser", &baz,
			  PARAM_INT32, RUN_NONINTERACTIVE,
			  PARAM_STRING, MAZE_URL,
			  PARAM_INT32, HELP_OPENS_NEW_WINDOW,
			  PARAM_END);
    } 
  else 
    {
      message = g_strdup_printf (_("See %s"), MAZE_URL);
      maze_msg (message);
      g_free (message);
    }                                            
}

static void
maze_msg (gchar *msg)
{
  gtk_label_set_text (GTK_LABEL (msg_label), msg);
}

static void
maze_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  maze_run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

#ifdef SHOW_PRNG_PRIVATES
static void 
maze_entry_callback (GtkWidget *widget,
		     gpointer   data)
{
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}
#endif

/* ==================================================================== */
/* As found in pixelize.c, 
 * hacked to return a pointer to the entry widget. */

/*
  Entry and Scale pair 1.03

  TODO:
  - Do the proper thing when the user changes value in entry,
  so that callback should not be called when value is actually not changed.
  - Update delay
 */

/*
 *  entscale: create new entscale with label. (int)
 *  1 row and 2 cols of table are needed.
 *  Input:
 *    x, y:       starting row and col in table
 *    caption:    label string
 *    intvar:     pointer to variable
 *    min, max:   the boundary of scale
 *    constraint: (bool) true iff the value of *intvar should be constraint
 *                by min and max
 *    callback:	  called when the value is actually changed
 *    call_data:  data for callback func
 */
static GtkWidget*
entscale_int_new (GtkWidget *table, 
		  gint       x, 
		  gint       y,
		  gchar     *caption, 
		  gint      *intvar,
		  gint       min, 
		  gint       max, 
		  gboolean   constraint,
		  EntscaleIntCallbackFunc callback,
		  gpointer   call_data)
{
  EntscaleIntData *userdata;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  gint	     constraint_val;

  userdata = g_new (EntscaleIntData, 1);

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value_changed" signal. I don't like this, since I want to leave
    *intvar untouched when `constraint' is false.
    The lines below might look oppositely, but this is OK.
   */
  userdata->constraint = constraint;
  if( constraint )
    constraint_val = *intvar;
  else
    constraint_val = ( *intvar < min ? min : *intvar > max ? max : *intvar );

  userdata->adjustment = adjustment = 
    gtk_adjustment_new (constraint_val, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, ENTSCALE_INT_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  userdata->entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_INT_ENTRY_WIDTH, 0);
  g_snprintf (buffer, BUFSIZE, "%d", *intvar);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  userdata->callback = callback;
  userdata->call_data = call_data;

  /* userdata is done */
  gtk_object_set_user_data (GTK_OBJECT (adjustment), userdata);
  gtk_object_set_user_data (GTK_OBJECT (entry), userdata);

  /* now ready for signals */
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entscale_int_entry_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) entscale_int_scale_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (entry), "destroy",
		      (GtkSignalFunc) entscale_int_destroy_callback,
		      userdata );

  /* start packing */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  return entry;
}  


/* when destroyed, userdata is destroyed too */
static void
entscale_int_destroy_callback (GtkWidget *widget,
			       gpointer   data)
{
  EntscaleIntData *userdata;

  userdata = data;
  g_free (userdata);
}

static void
entscale_int_scale_update (GtkAdjustment *adjustment,
			   gpointer       data)
{
  EntscaleIntData *userdata;
  GtkEntry *entry;
  gint     *intvar = data;
  gint      new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = (gint) adjustment->value;

  *intvar = new_val;

  entry = GTK_ENTRY (userdata->entry);
  g_snprintf (buffer, BUFSIZE, "%d", (int) new_val);
  
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data (GTK_OBJECT (entry), data);
  gtk_entry_set_text (entry, buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (entry), data);

  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}

static void
entscale_int_entry_update (GtkWidget *widget,
			   gpointer   data)
{
  EntscaleIntData *userdata;
  GtkAdjustment	  *adjustment;
  gint		   new_val, constraint_val;
  gint		  *intvar = data;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT (userdata->adjustment);

  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *intvar = constraint_val;
  else
    *intvar = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data (GTK_OBJECT (adjustment), data);
  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (adjustment), data);
  
  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}
