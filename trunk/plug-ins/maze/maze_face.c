/* -*- mode: C; c-file-style: "gnu"; c-basic-offset: 2; -*- */
/*
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

#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "maze.h"

#include "libgimp/stdplugins-intl.h"


#define BORDER_TOLERANCE  1.00  /* maximum ratio of (max % divs) to width */
#define ENTRY_WIDTH       75

/* entscale stuff begin */
/* FIXME: Entry-Scale stuff is probably in libgimpui by now.
          Should use that instead.*/
/* Indeed! By using the gimp_scale_entry you could get along without the
   EntscaleIntData structure, since it has accessors to all objects  (Sven) */

#define ENTSCALE_INT_SCALE_WIDTH 125
#define ENTSCALE_INT_ENTRY_WIDTH 40

#define MORE  1
#define LESS -1

typedef void (* EntscaleIntCallback) (gint value, gpointer data);

typedef struct
{
  GtkObject           *adjustment;
  GtkWidget           *entry;
  gboolean             constraint;
  EntscaleIntCallback  callback;
  gpointer	       call_data;
} EntscaleIntData;
/* entscale stuff end */


/* one buffer fits all */
#define BUFSIZE 128
gchar buffer[BUFSIZE];


gboolean     maze_dialog         (void);
static void  maze_message        (const gchar *message);


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
static GtkWidget * entscale_int_new (GtkWidget           *table,
                                     gint                 x,
                                     gint                 y,
                                     const gchar         *caption,
                                     gint                *intvar,
                                     gint                 min,
                                     gint                 max,
                                     gboolean             constraint,
                                     EntscaleIntCallback  callback,
                                     gpointer             data);

static void   entscale_int_scale_update (GtkAdjustment *adjustment,
					 gpointer       data);
static void   entscale_int_entry_update (GtkWidget     *widget,
					 gpointer       data);


#define ISODD(X) ((X) & 1)
/* entscale stuff end */

extern MazeValues mvals;
extern guint      sel_w, sel_h;

static GtkWidget *msg_label;

gboolean
maze_dialog (void)
{
  GtkWidget    *dialog;
  GtkWidget    *vbox;
  GtkWidget    *vbox2;
  GtkWidget    *table;
  GtkWidget    *table2;
  GtkWidget    *tilecheck;
  GtkWidget    *width_entry;
  GtkWidget    *height_entry;
  GtkWidget    *hbox;
  GtkWidget    *frame;
  GtkSizeGroup *group;
  gboolean      run;
  gint          trow = 0;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_(MAZE_TITLE), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      vbox, FALSE, FALSE, 0);

  /* The maze size frame */
  frame = gimp_frame_new (_("Maze Size"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (6, 3, FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* entscale == Entry and Scale pair function found in pixelize.c */
  width_entry = entscale_int_new (table, 0, trow, _("Width (pixels):"),
                                  &mvals.width,
                                  1, sel_w/4, TRUE,
                                  (EntscaleIntCallback) height_width_callback,
                                  &width_entry);
  trow++;

  /* Number of Divisions entry */
  hbox = divbox_new (&sel_w, width_entry, &width_entry);
  g_snprintf (buffer, BUFSIZE, "%d", (sel_w / mvals.width));
  gtk_entry_set_text (GTK_ENTRY (width_entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow,
			     _("Pieces:"), 0.0, 0.5,
			     hbox, 1, TRUE);
  gtk_table_set_row_spacing (GTK_TABLE (table), trow, 12);
  trow++;

  height_entry = entscale_int_new (table, 0, trow, _("Height (pixels):"),
                                   &mvals.height,
                                   1, sel_h/4, TRUE,
                                   (EntscaleIntCallback) height_width_callback,
                                   &height_entry);
  trow++;

  hbox = divbox_new (&sel_h, height_entry, &height_entry);
  g_snprintf (buffer, BUFSIZE, "%d", (sel_h / mvals.height));
  gtk_entry_set_text (GTK_ENTRY (height_entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, trow,
			     _("Pieces:"), 0.0, 0.5,
			     hbox, 1, TRUE);
  gtk_table_set_row_spacing (GTK_TABLE (table), trow, 12);
  trow++;

  g_object_unref (group);

  /* The maze algorithm frame */
  frame = gimp_frame_new (_("Algorithm"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* Seed input box */
  table2 = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 6);
  gtk_box_pack_start (GTK_BOX (vbox2), table2, FALSE, FALSE, 0);
  gtk_widget_show (table2);

  hbox = gimp_random_seed_new (&mvals.seed, &mvals.random_seed);
  gimp_table_attach_aligned (GTK_TABLE (table2), 0, 0,
			     _("Seed:"), 0.0, 0.5,
			     hbox, 1, TRUE);

  /* Algorithm Choice */
  frame =
    gimp_int_radio_group_new (FALSE, NULL,
                              G_CALLBACK (gimp_radio_button_update),
                              &mvals.algorithm, mvals.algorithm,

                              _("Depth first"),      DEPTH_FIRST,     NULL,
                              _("Prim's algorithm"), PRIMS_ALGORITHM, NULL,

                              NULL);

  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

  /* Tileable checkbox */
  tilecheck = gtk_check_button_new_with_label (_("Tileable"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tilecheck), mvals.tile);
  g_signal_connect (tilecheck, "clicked",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mvals.tile);

  gtk_box_pack_start (GTK_BOX (vbox2), tilecheck, FALSE, FALSE, 0);

  msg_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (msg_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), msg_label, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
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

  g_object_set_data (G_OBJECT (buttonl), "direction", GINT_TO_POINTER (LESS));
  g_object_set_data (G_OBJECT (buttonr), "direction", GINT_TO_POINTER (MORE));

  *div_entry = gtk_entry_new ();

  g_object_set_data (G_OBJECT (*div_entry), "max", max);
  g_object_set_data (G_OBJECT (*div_entry), "friend", friend);

  gtk_container_add (GTK_CONTAINER (buttonl), arrowl);
  gtk_container_add (GTK_CONTAINER (buttonr), arrowr);

  gtk_widget_set_size_request (*div_entry, ENTRY_WIDTH, -1);

#if DIVBOX_LOOKS_LIKE_SPINBUTTON
  buttonbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (buttonbox), buttonr, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonl, FALSE, FALSE, 0);
  gtk_widget_show (buttonbox);

  gtk_box_pack_start (GTK_BOX (hbox), *div_entry, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), buttonbox, FALSE, FALSE, 0);
#else
  gtk_box_pack_start (GTK_BOX (hbox), buttonl, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), *div_entry, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), buttonr, FALSE, FALSE, 0);
#endif

  gtk_widget_show_all (hbox);

  g_signal_connect (buttonl, "clicked",
                    G_CALLBACK (div_button_callback),
                    *div_entry);
  g_signal_connect (buttonr, "clicked",
                    G_CALLBACK (div_button_callback),
                    *div_entry);
  g_signal_connect (*div_entry, "changed",
                    G_CALLBACK (div_entry_callback),
                    friend);

  return align;
}

static void
div_button_callback (GtkWidget *button,
		     GtkWidget *entry)
{
  const gchar *text;
  guint        max, divs;
  gint         direction;

  direction = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                  "direction"));
  max = *((guint*) g_object_get_data (G_OBJECT (entry), "max"));

  /* Tileable mazes shall have only an even number of divisions.
     Other mazes have odd. */

  /* Sanity check: */
  if (mvals.tile && ISODD(max))
    {
      maze_message (_("Selection size is not even.\n"
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
  EntscaleIntCallback friend_callback;

  divs = atoi (gtk_entry_get_text (GTK_ENTRY (entry)));
  if (divs < 4) /* If this is under 4 (e.g. 0), something's weird.      */
    return;     /* But it'll probably be ok, so just return and ignore. */

  max = *((guint*) g_object_get_data (G_OBJECT (entry), "max"));

  /* I say "width" here, but it could be height.*/

  width = max / divs;
  g_snprintf (buffer, BUFSIZE, "%d", width);

  /* No tagbacks from our friend... */
  userdata = g_object_get_data (G_OBJECT (friend), "userdata");
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

  max = *((guint*) g_object_get_data(G_OBJECT(*div_entry), "max"));
  divs = max / width;

  g_snprintf (buffer, BUFSIZE, "%d", divs );

  data = g_object_get_data (G_OBJECT(*div_entry), "friend");

  g_signal_handlers_block_by_func (*div_entry,
                                   entscale_int_entry_update,
                                   data);

  gtk_entry_set_text (GTK_ENTRY (*div_entry), buffer);

  g_signal_handlers_unblock_by_func (*div_entry,
                                     entscale_int_entry_update,
                                     data);
}

static void
maze_message (const gchar *message)
{
  gtk_label_set_text (GTK_LABEL (msg_label), message);
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
entscale_int_new (GtkWidget           *table,
		  gint                 x,
		  gint                 y,
		  const gchar         *caption,
		  gint                *intvar,
		  gint                 min,
		  gint                 max,
		  gboolean             constraint,
		  EntscaleIntCallback  callback,
		  gpointer             call_data)
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
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value-changed" signal. I don't like this, since I want to leave
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
  gtk_widget_set_size_request (scale, ENTSCALE_INT_SCALE_WIDTH, -1);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  userdata->entry = entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, ENTSCALE_INT_ENTRY_WIDTH, -1);
  g_snprintf (buffer, BUFSIZE, "%d", *intvar);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  userdata->callback = callback;
  userdata->call_data = call_data;

  /* userdata is done */
  g_object_set_data (G_OBJECT (adjustment), "userdata", userdata);
  g_object_set_data (G_OBJECT (entry), "userdata", userdata);

  /* now ready for signals */
  g_signal_connect (entry, "changed",
                    G_CALLBACK (entscale_int_entry_update),
                    intvar);
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (entscale_int_scale_update),
                    intvar);
  g_signal_connect_swapped (entry, "destroy",
                            G_CALLBACK (g_free),
                            userdata);

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


static void
entscale_int_scale_update (GtkAdjustment *adjustment,
			   gpointer       data)
{
  EntscaleIntData *userdata;
  GtkEntry *entry;
  gint     *intvar = data;
  gint      new_val;

  userdata = g_object_get_data (G_OBJECT (adjustment), "userdata");

  new_val = (gint) adjustment->value;

  *intvar = new_val;

  entry = GTK_ENTRY (userdata->entry);
  g_snprintf (buffer, BUFSIZE, "%d", (int) new_val);

  /* avoid infinite loop (scale, entry, scale, entry ...) */
  g_signal_handlers_block_by_func (entry,
                                   entscale_int_entry_update,
                                   data);

  gtk_entry_set_text (entry, buffer);

  g_signal_handlers_unblock_by_func (entry,
                                     entscale_int_entry_update,
                                     data);

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

  userdata = g_object_get_data (G_OBJECT (widget), "userdata");
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

  g_signal_handlers_block_by_func (adjustment,
                                   entscale_int_scale_update,
                                   data);

  gtk_adjustment_value_changed (adjustment);

  g_signal_handlers_unblock_by_func (adjustment,
                                     entscale_int_scale_update,
                                     data);

  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}
