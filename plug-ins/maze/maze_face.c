/* maze_face.c
 * User interface for plug-in-maze.
 * 
 * Implemented as a GIMP 0.99 Plugin by 
 * Kevin Turner <kevint@poboxes.com>
 * http://www.poboxes.com/kevint/gimp/maze.html
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "maze.h"

#include "libgimp/stdplugins-intl.h"


#define BORDER_TOLERANCE 1.00 /* maximum ratio of (max % divs) to width */
#define ENTRY_WIDTH 75

/* entscale stuff begin */
#define ENTSCALE_INT_SCALE_WIDTH 125
#define ENTSCALE_INT_ENTRY_WIDTH 40

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

gint        maze_dialog         (void);

static void maze_msg            (gchar     *msg);
static void maze_ok_callback    (GtkWidget *widget,
				 gpointer   data);
static void maze_entry_callback (GtkWidget *widget,
				 gpointer   data);
static void maze_help           (GtkWidget *widget,
				 gpointer   foo);

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

     Pointing to static variables "less" and "more" (for the buttons
     in divbox_new) is stupid.  Is there a way to store integer values
     in userdata or use intergers as parameters to callbacks?  The
     only alternative I could think of was seperate "button_less" and
     "button_more" callbacks, which did nothing but pass data on to
     what is now the div_button_callback function with an additional
     -1 or 1 parameter...  And that idea was at least as brain-damaged. 

*/

static void div_button_callback   (GtkWidget *button, GtkWidget *entry);
static void div_entry_callback    (GtkWidget *entry, GtkWidget *friend);
static void height_width_callback (gint width, GtkWidget **div_entry);
static void toggle_callback       (GtkWidget *widget, gboolean *data);
static void alg_radio_callback    (GtkWidget *widget, gpointer data);

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
/* entscale stuff end */

extern MazeValues mvals;
extern guint      sel_w, sel_h;

static gint       maze_run = FALSE;
static GtkWidget *msg_label;

/* I only deal with setting up a few widgets at a time, so I could get
   by on a handful of generic GtkWidget variables.  But I've noticed
   that's not the way things are done around here...  I read it
   enhances optimization or some such thing.  Oh well.  Pointers are
   cheap, right? */
/* This is silly! Optimizing UI code like dialog creation makes no sense.
   The whole point of using more then one or two generic widget variables
   is code readability; but you obviously overexagerted it here!! 
   But pointers are cheap, so I'll leave it as is ...   -- Sven */
gint
maze_dialog (void) 
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *msg_frame;
  GtkWidget *msg_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *notebook;
  GtkWidget *tilecheck;

  GtkWidget *width_entry, *height_entry;
  GtkWidget *seed_hbox, *seed_entry, *time_button;
  GtkWidget *div_x_hbox, *div_y_hbox;
  GtkWidget *div_x_label, *div_y_label, *div_x_entry, *div_y_entry;

  GtkWidget *alg_box, *alg_button;

  gint    trow;
  gchar **argv;
  gint    argc;
  gchar   buffer[32];
  gchar  *message;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("maze");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  gdk_set_use_xshm(gimp_use_xshm());

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

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  msg_frame = gtk_frame_new (MAZE_TITLE);
  gtk_frame_set_shadow_type (GTK_FRAME (msg_frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), msg_frame, FALSE, FALSE, 0);
  gtk_widget_show (msg_frame);

  msg_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (msg_vbox), 4);
  gtk_container_add (GTK_CONTAINER (msg_frame), msg_vbox);
  gtk_widget_show (msg_vbox);

  message = g_strdup_printf (_("Selection is %dx%d"),sel_w, sel_h);
  msg_label = gtk_label_new (message);
  g_free (message);
  gtk_container_add (GTK_CONTAINER (msg_vbox), msg_label);
  gtk_widget_show (msg_label);

#if 0
  g_print ("label_width: %d, %d\n",
	   GTK_FRAME (msg_frame)->label_width,
	   gdk_string_measure (GTK_WIDGET (msg_frame)->style->font, 
			       GTK_FRAME (msg_frame)->label) + 7);
#endif

  /*  Set up Options page  */
  frame = gtk_frame_new (_("Maze Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  trow = 0;

  /* Tileable checkbox */
  tilecheck = gtk_check_button_new_with_label (_("Tileable?"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tilecheck), mvals.tile);
  gtk_signal_connect (GTK_OBJECT (tilecheck), "clicked",
		      GTK_SIGNAL_FUNC (toggle_callback), &mvals.tile);
  gtk_table_attach (GTK_TABLE (table), tilecheck, 0, 3, trow, trow+1, 
		    GTK_FILL, 0, 0, 0 );
  gtk_widget_show (tilecheck);

  trow++;

  /* entscale == Entry and Scale pair function found in pixelize.c */
  width_entry = entscale_int_new (table, 0, trow, "Width (Pixels):", 
				  &mvals.width, 
				  1, sel_w/4, TRUE, 
				  (EntscaleIntCallbackFunc) height_width_callback,
				  &div_x_entry);


  /* Number of Divisions entry */
  trow++;

  div_x_label = gtk_label_new (_("Pieces:"));
  gtk_misc_set_alignment (GTK_MISC (div_x_label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), div_x_label, 0,1, trow, trow+1, 
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (div_x_label);

  div_x_hbox = divbox_new (&sel_w, 
			   width_entry,
			   &div_x_entry);

  g_snprintf (buffer, sizeof (buffer), "%d", (sel_w / mvals.width) );
  gtk_entry_set_text (GTK_ENTRY (div_x_entry), buffer);

  gtk_table_attach (GTK_TABLE (table), div_x_hbox, 1, 2, trow, trow+1, 
		    0, 0, 0, 0);

  gtk_widget_show (div_x_hbox);

  trow++;

  height_entry = entscale_int_new (table, 0, trow, _("Height (Pixels):"), 
				   &mvals.height, 
				   1, sel_h/4, TRUE, 
				   (EntscaleIntCallbackFunc) height_width_callback,
				   &div_y_entry);

  trow++;

  div_y_label = gtk_label_new (_("Pieces:"));
  gtk_misc_set_alignment (GTK_MISC (div_y_label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), div_y_label, 0, 1, trow, trow+1, 
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (div_y_label);

  div_y_hbox = divbox_new(&sel_h,
			  height_entry,
			  &div_y_entry);

  g_snprintf (buffer, sizeof (buffer), "%d", (sel_h / mvals.height) );
  gtk_entry_set_text (GTK_ENTRY (div_y_entry), buffer);

  gtk_table_attach (GTK_TABLE (table), div_y_hbox, 1, 2, trow, trow+1, 
		    0, 0, 0, 0);

  gtk_widget_show (div_y_hbox);

  /* Add Options page to notebook */
  gtk_widget_show (frame);
  gtk_widget_show (table);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
			    gtk_label_new (_("Options")));

  /* Set up other page */
  frame = gtk_frame_new (_("At Your Own Risk"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* Multiple input box */
  label = gtk_label_new (_("Multiple (57):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%d", mvals.multiple);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.multiple);
  gtk_widget_show (entry);

  /* Offset input box */
  label = gtk_label_new (_("Offset (1):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%d", mvals.offset);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.offset);
  gtk_widget_show (entry);

  /* Seed input box */
  label = gtk_label_new (_("Seed:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, 
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  seed_hbox = gtk_hbox_new (FALSE, 2);
  gtk_table_attach (GTK_TABLE (table), seed_hbox, 1, 2, 2, 3, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  seed_entry = gtk_entry_new ();
  gtk_widget_set_usize (seed_entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%d", mvals.seed);
  gtk_entry_set_text (GTK_ENTRY (seed_entry), buffer);
  gtk_signal_connect (GTK_OBJECT (seed_entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.seed);
  gtk_box_pack_start(GTK_BOX(seed_hbox), seed_entry, TRUE, TRUE, 0);
  gtk_widget_show (seed_entry);

  time_button = gtk_toggle_button_new_with_label (_("Time"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_button),mvals.timeseed);
  gtk_signal_connect (GTK_OBJECT (time_button), "clicked",
		      (GtkSignalFunc) toggle_callback,
		      &mvals.timeseed);
  gtk_box_pack_end (GTK_BOX (seed_hbox), time_button, FALSE, FALSE, 0);
  gtk_widget_show (time_button);
  gtk_widget_show (seed_hbox);

  label = gtk_label_new (_("Algorithm:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, 
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  alg_box = gtk_vbox_new (FALSE, 2);
  gtk_table_attach (GTK_TABLE (table), alg_box, 1, 2, 3, 4, 
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (alg_box);

  alg_button = gtk_radio_button_new_with_label (NULL, _("Depth First"));
  gtk_signal_connect (GTK_OBJECT (alg_button), "toggled",
		      GTK_SIGNAL_FUNC (alg_radio_callback),
		      (gpointer) DEPTH_FIRST);
  if (mvals.algorithm == DEPTH_FIRST)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (alg_button), TRUE);
  gtk_container_add (GTK_CONTAINER (alg_box), alg_button);
  gtk_widget_show (alg_button);

  alg_button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON (alg_button)),
     _("Prim's Algorithm"));
  gtk_signal_connect (GTK_OBJECT (alg_button), "toggled",
		      GTK_SIGNAL_FUNC (alg_radio_callback),
		      (gpointer) PRIMS_ALGORITHM);
  if (mvals.algorithm == PRIMS_ALGORITHM)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (alg_button), TRUE);

  gtk_container_add (GTK_CONTAINER (alg_box), alg_button);
  gtk_widget_show (alg_button);

   /* Add Advanced page to notebook */
  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, 
			    gtk_label_new (_("Advanced")));

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return maze_run;
}

static GtkWidget*
divbox_new (guint      *max,
	    GtkWidget  *friend,
	    GtkWidget **div_entry)
{
  GtkWidget *div_hbox;
  GtkWidget *arrowl, *arrowr, *buttonl, *buttonr;
  static gshort less= -1, more= 1;
#if DIVBOX_LOOKS_LIKE_SPINBUTTON
  GtkWidget *buttonbox;
#endif

  div_hbox = gtk_hbox_new (FALSE, 0);

#if DIVBOX_LOOKS_LIKE_SPINBUTTON     
  arrowl = gtk_arrow_new (GTK_ARROW_DOWN,  GTK_SHADOW_OUT);
  arrowr = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
#else
  arrowl = gtk_arrow_new (GTK_ARROW_LEFT,  GTK_SHADOW_IN);
  arrowr = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_IN);
#endif

  buttonl = gtk_button_new ();
  buttonr = gtk_button_new ();
     
  gtk_object_set_data (GTK_OBJECT (buttonl), "direction", &less);
  gtk_object_set_data (GTK_OBJECT (buttonr), "direction", &more);

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

  gtk_box_pack_start (GTK_BOX (div_hbox), *div_entry, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (div_hbox), buttonbox, FALSE, FALSE, 0);
#else
  gtk_misc_set_padding (GTK_MISC (arrowl), 2, 2);
  gtk_misc_set_padding (GTK_MISC (arrowr), 2, 2);

  gtk_box_pack_start (GTK_BOX (div_hbox), buttonl, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (div_hbox), *div_entry,   FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (div_hbox), buttonr, FALSE, FALSE, 0);
#endif     

  gtk_widget_show (arrowl);
  gtk_widget_show (arrowr);
  gtk_widget_show (*div_entry);
  gtk_widget_show (buttonl);
  gtk_widget_show (buttonr);

  gtk_signal_connect (GTK_OBJECT (buttonl), "clicked",
		      (GtkSignalFunc) div_button_callback, 
		      *div_entry);

  gtk_signal_connect (GTK_OBJECT (buttonr), "clicked",
		      (GtkSignalFunc) div_button_callback, 
		      *div_entry);     

  gtk_signal_connect (GTK_OBJECT (*div_entry), "changed",
		      (GtkSignalFunc) div_entry_callback,
		      friend);

  return div_hbox;
}

static void
div_button_callback (GtkWidget *button,
		     GtkWidget *entry)
{
  guint max, divs, even;
  gchar *text, *text2;
  gshort direction;

  direction = *((gshort*) gtk_object_get_data(GTK_OBJECT(button), "direction"));
  max = *((guint*) gtk_object_get_data(GTK_OBJECT(entry), "max"));

  /* Tileable mazes shall have only an even number of divisions.
     Other mazes have odd. */
  /* Logic games!

     If BIT1 is and "even" is then add:
     FALSE        TRUE        0
     TRUE         TRUE        1
     FALSE        FALSE       1
     TRUE         FALSE       0

     That's where the +((foo & 1) == even) stuff comes from. */

  /* Sanity check: */
  if (mvals.tile && (max & 1))
    {
      maze_msg(_("Selection size is not even.  \nTileable maze won't work perfectly."));
      return;
    }

  even = mvals.tile ? 1 : 0;

  text = gtk_entry_get_text (GTK_ENTRY (entry));

  divs = atoi (text);
  if (divs <= 3)
    {
      divs= max - ((max & 1) == even);	  
    }
  else if (divs > max)
    {
      divs= 5 + even;
    }
     
  /* Makes sure we're appropriately even or odd, adjusting in the
     proper direction. */
  divs += direction * ((divs & 1) == even);
	  
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
	  while ((max % divs > max / divs * BORDER_TOLERANCE ) && divs < max);
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
      divs= max - ((max & 1) == even);	  
    }
  else if (divs > max)
    {
      divs= 5 - even;
    }

  text2 = g_new (gchar, 16);
  sprintf (text2, "%d", divs);

  gtk_entry_set_text (GTK_ENTRY (entry), text2);

  return;
}

static void
div_entry_callback (GtkWidget *entry,
		    GtkWidget *friend)
{
  guint divs, width, max;
  gchar *buffer;
  EntscaleIntData *userdata;
  EntscaleIntCallbackFunc friend_callback;

  divs = atoi(gtk_entry_get_text (GTK_ENTRY (entry)));
  if (divs < 4) /* If this is under 4 (e.g. 0), something's weird. */
    return;  /* But it'll probably be ok, so just return and ignore. */

  max = *((guint*) gtk_object_get_data(GTK_OBJECT(entry), "max"));     
  buffer = g_new(gchar, 16);

  /* I say "width" here, but it could be height.*/

  width = max/divs;
  sprintf (buffer,"%d", width );

  /* No tagbacks from our friend... */
  userdata = gtk_object_get_user_data (GTK_OBJECT (friend));
  friend_callback = userdata->callback;
  userdata->callback = NULL;

  gtk_entry_set_text(GTK_ENTRY(friend), buffer);

  userdata->callback = friend_callback;
}

static void
height_width_callback (gint        width,
		       GtkWidget **div_entry)
{
  guint divs, max;
  gpointer data;
  gchar *buffer;

  max = *((guint*) gtk_object_get_data(GTK_OBJECT(*div_entry), "max"));
  divs = max / width;

  buffer = g_new(gchar, 16);
  sprintf (buffer,"%d", divs );

  data = gtk_object_get_data(GTK_OBJECT(*div_entry), "friend");
  gtk_signal_handler_block_by_data ( GTK_OBJECT(*div_entry), data );
     
  gtk_entry_set_text(GTK_ENTRY(*div_entry), buffer);

  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(*div_entry), data );
}

static void
maze_help (GtkWidget *widget,
	   gpointer   foo)
{
  char *proc_blurb, *proc_help, *proc_author, *proc_copyright, *proc_date;
  int proc_type, nparams, nreturn_vals;
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

static void 
maze_entry_callback (GtkWidget *widget,
		       gpointer data)
{
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void 
toggle_callback (GtkWidget *widget,
		 gboolean  *data)
{
  *data = GTK_TOGGLE_BUTTON (widget)->active;
}

static void
alg_radio_callback (GtkWidget *widget,
		    gpointer   data)
{
  mvals.algorithm = (MazeAlgoType) data;
}

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
entscale_int_new (GtkWidget *table, gint x, gint y,
		  gchar *caption, gint *intvar,
		  gint min, gint max, gboolean constraint,
		  EntscaleIntCallbackFunc callback,
		  gpointer call_data)
{
  EntscaleIntData *userdata;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  gchar      buffer[256];
  gint	     constraint_val;

  userdata = g_new ( EntscaleIntData, 1 );

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
    gtk_adjustment_new ( constraint_val, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, ENTSCALE_INT_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  userdata->entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_INT_ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", *intvar );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );

  userdata->callback = callback;
  userdata->call_data = call_data;

  /* userdata is done */
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);

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
			       gpointer data)
{
  EntscaleIntData *userdata;

  userdata = data;
  g_free (userdata);
}

static void
entscale_int_scale_update (GtkAdjustment *adjustment,
			   gpointer      data)
{
  EntscaleIntData *userdata;
  GtkEntry	*entry;
  gchar		buffer[256];
  gint		*intvar = data;
  gint		new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = (gint) adjustment->value;

  *intvar = new_val;

  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%d", (int) new_val );
  
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );

  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}

static void
entscale_int_entry_update (GtkWidget *widget,
			   gpointer   data)
{
  EntscaleIntData *userdata;
  GtkAdjustment	*adjustment;
  int		new_val, constraint_val;
  int		*intvar = data;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

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
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );
  
  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}
