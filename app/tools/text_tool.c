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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "drawable.h"
#include "edit_selection.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "general.h"
#include "global_edit.h"
#include "interface.h"
#include "palette.h"
#include "procedural_db.h"
#include "selection.h"
#include "text_tool.h"
#include "tools.h"
#include "undo.h"

#include "tile_manager_pvt.h"
#include "drawable_pvt.h"

#define FONT_LIST_WIDTH  125
#define FONT_LIST_HEIGHT 200

#define PIXELS 0
#define POINTS 1

#define FOUNDRY   0
#define FAMILY    1
#define WEIGHT    2
#define SLANT     3
#define SET_WIDTH 4
#define SPACING   10
#define REGISTRY  12
#define ENCODING  13

#define SUPERSAMPLE 3

typedef struct _TextTool TextTool;
struct _TextTool
{
  GtkWidget *shell;
  GtkWidget *main_vbox;
  GtkWidget *font_list;
  GtkWidget *size_menu;
  GtkWidget *size_text;
  GtkWidget *border_text;
  GtkWidget *text_frame;
  GtkWidget *the_text;
  GtkWidget *menus[7];
  GtkWidget *option_menus[7];
  GtkWidget **foundry_items;
  GtkWidget **weight_items;
  GtkWidget **slant_items;
  GtkWidget **set_width_items;
  GtkWidget **spacing_items;
  GtkWidget **registry_items;
  GtkWidget **encoding_items;
  GtkWidget *antialias_toggle;
  GdkFont *font;
  int click_x;
  int click_y;
  int font_index;
  int size_type;
  int antialias;
  int foundry;
  int weight;
  int slant;
  int set_width;
  int spacing;
  int registry;
  int encoding;
  void *gdisp_ptr;
};

typedef struct _FontInfo FontInfo;
struct _FontInfo
{
  char *family;         /* The font family this info struct describes. */
  int *foundries;       /* An array of valid foundries. */
  int *weights;         /* An array of valid weights. */
  int *slants;          /* An array of valid slants. */
  int *set_widths;      /* An array of valid set widths. */
  int *spacings;        /* An array of valid spacings */
  int *registries;      /* An array of valid registries */
  int *encodings;       /* An array of valid encodings */
  int **combos;         /* An array of valid combinations of the above 7 items */
  int ncombos;          /* The number of elements in the "combos" array */
  GSList *fontnames;    /* An list of valid fontnames.
			 * This is used to make sure a family/foundry/weight/slant/set_width
			 *  combination is valid.
			 */
};

static void       text_button_press       (Tool *, GdkEventButton *, gpointer);
static void       text_button_release     (Tool *, GdkEventButton *, gpointer);
static void       text_motion             (Tool *, GdkEventMotion *, gpointer);
static void       text_cursor_update      (Tool *, GdkEventMotion *, gpointer);
static void       text_control            (Tool *, int, gpointer);

static void       text_resize_text_widget (TextTool *);
static void       text_create_dialog      (TextTool *);
static void       text_ok_callback        (GtkWidget *, gpointer);
static void       text_cancel_callback    (GtkWidget *, gpointer);
static gint       text_delete_callback    (GtkWidget *, GdkEvent *, gpointer);
static void       text_pixels_callback    (GtkWidget *, gpointer);
static void       text_points_callback    (GtkWidget *, gpointer);
static void       text_foundry_callback   (GtkWidget *, gpointer);
static void       text_weight_callback    (GtkWidget *, gpointer);
static void       text_slant_callback     (GtkWidget *, gpointer);
static void       text_set_width_callback (GtkWidget *, gpointer);
static void       text_spacing_callback   (GtkWidget *, gpointer);
static void       text_registry_callback  (GtkWidget *, gpointer);
static void       text_encoding_callback  (GtkWidget *, gpointer);
static gint       text_size_key_function  (GtkWidget *, GdkEventKey *, gpointer);
static void       text_antialias_update   (GtkWidget *, gpointer);
static void       text_font_item_update   (GtkWidget *, gpointer);
static void       text_validate_combo     (TextTool *, int);

static void       text_get_fonts          (void);
static void       text_insert_font        (FontInfo **, int *, char *);
static GSList*    text_insert_field       (GSList *, char *, int);
static char*      text_get_field          (char *, int);
static int        text_field_to_index     (char **, int, char *);
static int        text_is_xlfd_font_name  (char *);

static int        text_get_xlfd           (double, int, char *, char *, char *,
					   char *, char *, char *, char *,
					   char *, char *);
static int        text_load_font          (TextTool *);
static void       text_init_render        (TextTool *);
static void       text_gdk_image_to_region (GdkImage *, int, PixelRegion *);
static int        text_get_extents        (char *, char *, int *, int *, int *, int *);
static Layer *    text_render             (GImage *, GimpDrawable *, int, int, char *, char *, int, int);

static int        font_compare_func (gpointer, gpointer);

static Argument * text_tool_invoker                  (Argument *);
static Argument * text_tool_invoker_ext              (Argument *);
static Argument * text_tool_get_extents_invoker      (Argument *);
static Argument * text_tool_get_extents_invoker_ext  (Argument *);

static ActionAreaItem action_items[] =
{
  { "OK", text_ok_callback, NULL, NULL },
  { "Cancel", text_cancel_callback, NULL, NULL },
};

static MenuItem size_metric_items[] =
{
  { "Pixels", 0, 0, text_pixels_callback, (gpointer) PIXELS, NULL, NULL },
  { "Points", 0, 0, text_points_callback, (gpointer) POINTS, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

static TextTool *the_text_tool = NULL;
static FontInfo **font_info;
static int nfonts = -1;

static GSList *foundries = NULL;
static GSList *weights = NULL;
static GSList *slants = NULL;
static GSList *set_widths = NULL;
static GSList *spacings = NULL;
static GSList *registries = NULL;
static GSList *encodings = NULL;

static char **foundry_array = NULL;
static char **weight_array = NULL;
static char **slant_array = NULL;
static char **set_width_array = NULL;
static char **spacing_array = NULL;
static char **registry_array = NULL;
static char **encoding_array = NULL;

static int nfoundries = 0;
static int nweights = 0;
static int nslants = 0;
static int nset_widths = 0;
static int nspacings = 0;
static int nregistries = 0;
static int nencodings = 0;

static void *text_options = NULL;

Tool*
tools_new_text ()
{
  Tool * tool;

  if (! text_options)
    text_options = tools_register_no_options (TEXT, "Text Tool Options");

  tool = g_malloc (sizeof (Tool));
  if (!the_text_tool)
    {
      the_text_tool = g_malloc (sizeof (TextTool));
      the_text_tool->shell = NULL;
      the_text_tool->font_list = NULL;
      the_text_tool->size_menu = NULL;
      the_text_tool->size_text = NULL;
      the_text_tool->border_text = NULL;
      the_text_tool->text_frame = NULL;
      the_text_tool->the_text = NULL;
      the_text_tool->font = NULL;
      the_text_tool->font_index = 0;
      the_text_tool->size_type = PIXELS;
      the_text_tool->antialias = 1;
      the_text_tool->foundry = 0;
      the_text_tool->weight = 0;
      the_text_tool->slant = 0;
      the_text_tool->set_width = 0;
      the_text_tool->spacing = 0;
      the_text_tool->registry = 0;
      the_text_tool->encoding = 0;
    }

  tool->type = TEXT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /* Do not allow scrolling */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) the_text_tool;
  tool->button_press_func = text_button_press;
  tool->button_release_func = text_button_release;
  tool->motion_func = text_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = text_cursor_update;
  tool->control_func = text_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_text (Tool *tool)
{
}

static void
text_button_press (Tool           *tool,
		   GdkEventButton *bevent,
		   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  TextTool *text_tool;

  gdisp = gdisp_ptr;
  text_tool = tool->private;
  text_tool->gdisp_ptr = gdisp_ptr;

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &text_tool->click_x, &text_tool->click_y,
			       TRUE, 0);

  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, text_tool->click_x, text_tool->click_y)))
    /*  If there is a floating selection, and this aint it, use the move tool  */
    if (layer_is_floating_sel (layer))
      {
	init_edit_selection (tool, gdisp_ptr, bevent, LayerTranslate);
	return;
      }

  if (nfonts == -1)
    {
      text_get_fonts ();
      if (nfonts == 0)
       {
         g_message ("Note: No fonts found.  Text tool not available.");
         nfonts = -1;
         return;
       }
    }

  if (!text_tool->shell)
    text_create_dialog (text_tool);

  switch (gimage_base_type (gdisp->gimage))
    {
    case RGB:
    case GRAY:
      if (!GTK_WIDGET_VISIBLE (text_tool->antialias_toggle)) {
	gtk_widget_show (text_tool->antialias_toggle);
	if (GTK_TOGGLE_BUTTON (text_tool->antialias_toggle)->active)
	  text_tool->antialias = TRUE;
	else
	  text_tool->antialias = FALSE;
      }
      break;
    case INDEXED:
      if (GTK_WIDGET_VISIBLE (text_tool->antialias_toggle)) {
	gtk_widget_hide (text_tool->antialias_toggle);
	text_tool->antialias = FALSE;
      }
      break;
    }

  if (!GTK_WIDGET_VISIBLE (text_tool->shell))
    gtk_widget_show (text_tool->shell);
}

static void
text_button_release (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
  tool->state = INACTIVE;
}

static void
text_motion (Tool           *tool,
	     GdkEventMotion *mevent,
	     gpointer        gdisp_ptr)
{
}

static void
text_cursor_update (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);

  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
    /*  if there is a floating selection, and this aint it...  */
    if (layer_is_floating_sel (layer))
      {
	gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
	return;
      }

  gdisplay_install_tool_cursor (gdisp, GDK_XTERM);
}

static void
text_control (Tool     *tool,
	      int       action,
	      gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (the_text_tool->shell && GTK_WIDGET_VISIBLE (the_text_tool->shell))
	gtk_widget_hide (the_text_tool->shell);
      break;
    }
}

static void
text_resize_text_widget (TextTool *text_tool)
{
  GtkStyle *style;

  style = gtk_style_new ();
  gdk_font_unref (style->font);
  style->font = text_tool->font;
  gdk_font_ref (style->font);

  gtk_widget_set_style (text_tool->the_text, style);
}

static void
text_create_dialog (TextTool *text_tool)
{
  GtkWidget *top_hbox;
  GtkWidget *right_vbox;
  GtkWidget *text_hbox;
  GtkWidget *list_box;
  GtkWidget *list_item;
  GtkWidget *size_opt_menu;
  GtkWidget *border_label;
  GtkWidget *menu_table;
  GtkWidget *menu_label;
  GtkWidget **menu_items[7];
  GtkWidget *alignment;
  int nmenu_items[7];
  char *menu_strs[7];
  char **menu_item_strs[7];
  int *font_infos[7];
  MenuItemCallback menu_callbacks[7];
  int i, j;

  /* Create the shell and vertical & horizontal boxes */
  text_tool->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (text_tool->shell), "text_tool", "Gimp");
  gtk_window_set_title (GTK_WINDOW (text_tool->shell), "Text Tool");
  gtk_window_set_policy (GTK_WINDOW (text_tool->shell), FALSE, TRUE, TRUE);
  gtk_window_position (GTK_WINDOW (text_tool->shell), GTK_WIN_POS_MOUSE);
  gtk_widget_set (GTK_WIDGET (text_tool->shell),
		  "GtkWindow::auto_shrink", FALSE,
		  NULL);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (text_tool->shell), "delete_event",
		      GTK_SIGNAL_FUNC (text_delete_callback),
		      text_tool);

  text_tool->main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (text_tool->main_vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (text_tool->shell)->vbox), text_tool->main_vbox, TRUE, TRUE, 0);
  top_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (text_tool->main_vbox), top_hbox, TRUE, TRUE, 0);

  /* Create the font listbox  */
  list_box = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (list_box, FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_box_pack_start (GTK_BOX (top_hbox), list_box, TRUE, TRUE, 0);
  text_tool->font_list = gtk_list_new ();
  gtk_container_add (GTK_CONTAINER (list_box), text_tool->font_list);
  gtk_list_set_selection_mode (GTK_LIST (text_tool->font_list), GTK_SELECTION_BROWSE);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (text_tool->font_list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (list_box)));
  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (list_box)->vscrollbar, GTK_CAN_FOCUS);
  
  for (i = 0; i < nfonts; i++)
    {
      list_item = gtk_list_item_new_with_label (font_info[i]->family);
      gtk_container_add (GTK_CONTAINER (text_tool->font_list), list_item);
      gtk_signal_connect (GTK_OBJECT (list_item), "select",
			  (GtkSignalFunc) text_font_item_update,
			  (gpointer) ((long) i));
      gtk_widget_show (list_item);
    }

  gtk_widget_show (text_tool->font_list);

  /* Create the box to hold options  */
  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (top_hbox), right_vbox, TRUE, TRUE, 2);

  /* Create the text hbox, size text, and fonts size metric option menu */
  text_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), text_hbox, FALSE, FALSE, 0);
  text_tool->size_text = gtk_entry_new ();
  gtk_widget_set_usize (text_tool->size_text, 75, 0);
  gtk_entry_set_text (GTK_ENTRY (text_tool->size_text), "50");
  gtk_signal_connect (GTK_OBJECT (text_tool->size_text), "key_press_event",
		      (GtkSignalFunc) text_size_key_function,
		      text_tool);
  gtk_box_pack_start (GTK_BOX (text_hbox), text_tool->size_text, TRUE, TRUE, 0);

  /* Create the size menu */
  size_metric_items[0].user_data = text_tool;
  size_metric_items[1].user_data = text_tool;
  text_tool->size_menu = build_menu (size_metric_items, NULL);
  size_opt_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (text_hbox), size_opt_menu, FALSE, FALSE, 0);

  /* create the text entry widget */
  text_tool->text_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (text_tool->text_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (text_tool->main_vbox), text_tool->text_frame, FALSE, FALSE, 2);
  text_tool->the_text = gtk_entry_new ();
  gtk_container_add (GTK_CONTAINER (text_tool->text_frame), text_tool->the_text);

  /* create the antialiasing toggle button  */
  text_tool->antialias_toggle = gtk_check_button_new_with_label ("Antialiasing");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (text_tool->antialias_toggle), text_tool->antialias);
  gtk_box_pack_start (GTK_BOX (right_vbox), text_tool->antialias_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (text_tool->antialias_toggle), "toggled",
		      (GtkSignalFunc) text_antialias_update,
		      text_tool);

  /* Allocate the arrays for the foundry, weight, slant, set_width and spacing menu items */
  text_tool->foundry_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nfoundries);
  text_tool->weight_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nweights);
  text_tool->slant_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nslants);
  text_tool->set_width_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nset_widths);
  text_tool->spacing_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nspacings);
  text_tool->registry_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nregistries);
  text_tool->encoding_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * nencodings);

  menu_items[0] = text_tool->foundry_items;
  menu_items[1] = text_tool->weight_items;
  menu_items[2] = text_tool->slant_items;
  menu_items[3] = text_tool->set_width_items;
  menu_items[4] = text_tool->spacing_items;
  menu_items[5] = text_tool->registry_items;
  menu_items[6] = text_tool->encoding_items;

  nmenu_items[0] = nfoundries;
  nmenu_items[1] = nweights;
  nmenu_items[2] = nslants;
  nmenu_items[3] = nset_widths;
  nmenu_items[4] = nspacings;
  nmenu_items[5] = nregistries;
  nmenu_items[6] = nencodings;

  menu_strs[0] = "Foundry";
  menu_strs[1] = "Weight";
  menu_strs[2] = "Slant";
  menu_strs[3] = "Set width";
  menu_strs[4] = "Spacing";
  menu_strs[5] = "Registry";
  menu_strs[6] = "Encoding";

  menu_item_strs[0] = foundry_array;
  menu_item_strs[1] = weight_array;
  menu_item_strs[2] = slant_array;
  menu_item_strs[3] = set_width_array;
  menu_item_strs[4] = spacing_array;
  menu_item_strs[5] = registry_array;
  menu_item_strs[6] = encoding_array;

  menu_callbacks[0] = text_foundry_callback;
  menu_callbacks[1] = text_weight_callback;
  menu_callbacks[2] = text_slant_callback;
  menu_callbacks[3] = text_set_width_callback;
  menu_callbacks[4] = text_spacing_callback;
  menu_callbacks[5] = text_registry_callback;
  menu_callbacks[6] = text_encoding_callback;

  font_infos[0] = font_info[0]->foundries;
  font_infos[1] = font_info[0]->weights;
  font_infos[2] = font_info[0]->slants;
  font_infos[3] = font_info[0]->set_widths;
  font_infos[4] = font_info[0]->spacings;
  font_infos[5] = font_info[0]->registries;
  font_infos[6] = font_info[0]->encodings;

  menu_table = gtk_table_new (8, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (right_vbox), menu_table, TRUE, TRUE, 0);

  /* Create the other menus */
  for (i = 0; i < 7; i++)
    {
      menu_label = gtk_label_new (menu_strs[i]);
      gtk_misc_set_alignment (GTK_MISC (menu_label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (menu_table), menu_label, 0, 1, i, i + 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);

      alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
      gtk_table_attach (GTK_TABLE (menu_table), alignment, 1, 2, i, i + 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
      text_tool->menus[i] = gtk_menu_new ();

      for (j = 0; j < nmenu_items[i]; j++)
	{
	  menu_items[i][j] = gtk_menu_item_new_with_label (menu_item_strs[i][j]);
	  gtk_widget_set_sensitive (menu_items[i][j], font_infos[i][j]);

	  gtk_container_add (GTK_CONTAINER (text_tool->menus[i]), menu_items[i][j]);
	  gtk_signal_connect (GTK_OBJECT (menu_items[i][j]), "activate",
			      (GtkSignalFunc) menu_callbacks[i],
			      (gpointer) ((long) j));
	  gtk_widget_show (menu_items[i][j]);
	}

      text_tool->option_menus[i] = gtk_option_menu_new ();
      gtk_container_add (GTK_CONTAINER (alignment), text_tool->option_menus[i]);

      gtk_widget_show (menu_label);
      gtk_widget_show (text_tool->option_menus[i]);
      gtk_widget_show (alignment);

      gtk_option_menu_set_menu (GTK_OPTION_MENU (text_tool->option_menus[i]), text_tool->menus[i]);
    }

  /* Create the border text hbox, border text, and label  */
  border_label = gtk_label_new ("Border");
  gtk_misc_set_alignment (GTK_MISC (border_label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (menu_table), border_label, 0, 1, i, i + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (menu_table), alignment, 1, 2, i, i + 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  text_tool->border_text = gtk_entry_new ();
  gtk_widget_set_usize (text_tool->border_text, 75, 25);
  gtk_entry_set_text (GTK_ENTRY (text_tool->border_text), "0");
  gtk_container_add (GTK_CONTAINER (alignment), text_tool->border_text);
  gtk_widget_show (alignment);

  /* Create the action area */
  action_items[0].user_data = text_tool;
  action_items[1].user_data = text_tool;
  build_action_area (GTK_DIALOG (text_tool->shell), action_items, 2, 0);

  /* Show the widgets */
  gtk_widget_show (menu_table);
  gtk_widget_show (text_tool->antialias_toggle);
  gtk_widget_show (text_tool->size_text);
  gtk_widget_show (border_label);
  gtk_widget_show (text_tool->border_text);
  gtk_widget_show (size_opt_menu);
  gtk_widget_show (text_hbox);
  gtk_widget_show (list_box);
  gtk_widget_show (right_vbox);
  gtk_widget_show (top_hbox);
  gtk_widget_show (text_tool->the_text);
  gtk_widget_show (text_tool->text_frame);
  gtk_widget_show (text_tool->main_vbox);

  /* Post initialization */
  gtk_option_menu_set_menu (GTK_OPTION_MENU (size_opt_menu), text_tool->size_menu);

  if (nfonts)
    text_load_font (text_tool);

  /* Show the shell */
  gtk_widget_show (text_tool->shell);
}

static void
text_ok_callback (GtkWidget *w,
		  gpointer   client_data)
{
  TextTool * text_tool;

  text_tool = (TextTool *) client_data;

  if (GTK_WIDGET_VISIBLE (text_tool->shell))
    gtk_widget_hide (text_tool->shell);

  text_init_render (text_tool);
}

static gint
text_delete_callback (GtkWidget *w,
		      GdkEvent  *e,
		      gpointer   client_data)
{
  text_cancel_callback (w, client_data);
  
  return TRUE;
}

static void
text_cancel_callback (GtkWidget *w,
		      gpointer   client_data)
{
  TextTool * text_tool;

  text_tool = (TextTool *) client_data;

  if (GTK_WIDGET_VISIBLE (text_tool->shell))
    gtk_widget_hide (text_tool->shell);
}

static void
text_font_item_update (GtkWidget *w,
		       gpointer   data)
{
  FontInfo *font;
  int old_index;
  int i, index;

  /*  Is this font being selected?  */
  if (w->state != GTK_STATE_SELECTED)
    return;

  index = (long) data;

  old_index = the_text_tool->font_index;
  the_text_tool->font_index = index;

  font = font_info[the_text_tool->font_index];

  if (the_text_tool->foundry && !font->foundries[the_text_tool->foundry])
    {
      the_text_tool->foundry = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[0]), 0);
    }
  if (the_text_tool->weight && !font->weights[the_text_tool->weight])
    {
      the_text_tool->weight = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[1]), 0);
    }
  if (the_text_tool->slant && !font->slants[the_text_tool->slant])
    {
      the_text_tool->slant = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[2]), 0);
    }
  if (the_text_tool->set_width && !font->set_widths[the_text_tool->set_width])
    {
      the_text_tool->set_width = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[3]), 0);
    }
  if (the_text_tool->spacing && !font->spacings[the_text_tool->spacing])
    {
      the_text_tool->spacing = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[4]), 0);
    }
  if (the_text_tool->registry && !font->registries[the_text_tool->registry])
    {
      the_text_tool->registry = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[5]), 0);
    }
  if (the_text_tool->encoding && !font->encodings[the_text_tool->encoding])
    {
      the_text_tool->encoding = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[6]), 0);
    }

  for (i = 0; i < nfoundries; i++)
    gtk_widget_set_sensitive (the_text_tool->foundry_items[i], font->foundries[i]);
  for (i = 0; i < nweights; i++)
    gtk_widget_set_sensitive (the_text_tool->weight_items[i], font->weights[i]);
  for (i = 0; i < nslants; i++)
    gtk_widget_set_sensitive (the_text_tool->slant_items[i], font->slants[i]);
  for (i = 0; i < nset_widths; i++)
    gtk_widget_set_sensitive (the_text_tool->set_width_items[i], font->set_widths[i]);
  for (i = 0; i < nspacings; i++)
    gtk_widget_set_sensitive (the_text_tool->spacing_items[i], font->spacings[i]);
  for (i = 0; i < nregistries; i++)
    gtk_widget_set_sensitive (the_text_tool->registry_items[i], font->registries[i]);
  for (i = 0; i < nencodings; i++)
    gtk_widget_set_sensitive (the_text_tool->encoding_items[i], font->encodings[i]);

  if (!text_load_font (the_text_tool))
    {
      the_text_tool->font_index = old_index;
      return;
    }
}

static void
text_pixels_callback (GtkWidget *w,
		      gpointer   client_data)
{
  TextTool *text_tool;
  int old_value;

  text_tool = (TextTool *) client_data;

  old_value = text_tool->size_type;
  text_tool->size_type = PIXELS;

  if (!text_load_font (text_tool))
    text_tool->size_type = old_value;
}

static void
text_points_callback (GtkWidget *w,
		      gpointer   client_data)
{
  TextTool *text_tool;
  int old_value;

  text_tool = (TextTool *) client_data;

  old_value = text_tool->size_type;
  text_tool->size_type = POINTS;

  if (!text_load_font (text_tool))
    text_tool->size_type = old_value;
}

static void
text_foundry_callback (GtkWidget *w,
		       gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->foundry;
  the_text_tool->foundry = (long) client_data;
  text_validate_combo (the_text_tool, 0);

  if (!text_load_font (the_text_tool))
    the_text_tool->foundry = old_value;
}

static void
text_weight_callback (GtkWidget *w,
		      gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->weight;
  the_text_tool->weight = (long) client_data;
  text_validate_combo (the_text_tool, 1);

  if (!text_load_font (the_text_tool))
    the_text_tool->weight = old_value;
}

static void
text_slant_callback (GtkWidget *w,
		     gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->slant;
  the_text_tool->slant = (long) client_data;
  text_validate_combo (the_text_tool, 2);

  if (!text_load_font (the_text_tool))
    the_text_tool->slant = old_value;
}

static void
text_set_width_callback (GtkWidget *w,
			 gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->set_width;
  the_text_tool->set_width = (long) client_data;
  text_validate_combo (the_text_tool, 3);

  if (!text_load_font (the_text_tool))
    the_text_tool->set_width = old_value;
}

static void
text_spacing_callback (GtkWidget *w,
		       gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->spacing;
  the_text_tool->spacing = (long) client_data;
  text_validate_combo (the_text_tool, 4);

  if (!text_load_font (the_text_tool))
    the_text_tool->spacing = old_value;
}

static void
text_registry_callback (GtkWidget *w,
		       gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->registry;
  the_text_tool->registry = (long) client_data;
  text_validate_combo (the_text_tool, 5);

  if (!text_load_font (the_text_tool))
    the_text_tool->registry = old_value;
}

static void
text_encoding_callback (GtkWidget *w,
		       gpointer   client_data)
{
  int old_value;

  old_value = the_text_tool->encoding;
  the_text_tool->encoding = (long) client_data;
  text_validate_combo (the_text_tool, 6);

  if (!text_load_font (the_text_tool))
    the_text_tool->encoding = old_value;
}

static gint
text_size_key_function (GtkWidget   *w,
			GdkEventKey *event,
			gpointer     data)
{
  TextTool *text_tool;
  char buffer[16];
  int old_value;

  text_tool = (TextTool *) data;

  if ((event->keyval == GDK_Return) || (event->keyval == GDK_Tab) ||
        (event->keyval == GDK_space))
    {
      if (event->keyval != GDK_Tab)
          gtk_signal_emit_stop_by_name (GTK_OBJECT (w), "key_press_event");

      old_value = atoi (gtk_entry_get_text (GTK_ENTRY (text_tool->size_text)));
      if (!text_load_font (text_tool))
	{
	  sprintf (buffer, "%d", old_value);
	  gtk_entry_set_text (GTK_ENTRY (text_tool->size_text), buffer);
	}
      return TRUE;
    }

  return FALSE;
}

static void
text_antialias_update (GtkWidget *w,
		       gpointer   data)
{
  TextTool *text_tool;

  text_tool = (TextTool *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    text_tool->antialias = TRUE;
  else
    text_tool->antialias = FALSE;
}

static void
text_validate_combo (TextTool *text_tool,
		     int       which)
{
  FontInfo *font;
  int which_val;
  int new_combo[7];
  int best_combo[7];
  int best_matches;
  int matches;
  int i;

  font = font_info[text_tool->font_index];

  switch (which)
    {
    case 0:
      which_val = text_tool->foundry;
      break;
    case 1:
      which_val = text_tool->weight;
      break;
    case 2:
      which_val = text_tool->slant;
      break;
    case 3:
      which_val = text_tool->set_width;
      break;
    case 4:
      which_val = text_tool->spacing;
      break;
    case 5:
      which_val = text_tool->registry;
      break;
    case 6:
      which_val = text_tool->encoding;
      break;
    default:
      which_val = 0;
      break;
    }

  best_matches = -1;
  best_combo[0] = 0;
  best_combo[1] = 0;
  best_combo[2] = 0;
  best_combo[3] = 0;
  best_combo[4] = 0;
  best_combo[5] = 0;
  best_combo[6] = 0;

  for (i = 0; i < font->ncombos; i++)
    {
      /* we must match the which field */
      if (font->combos[i][which] == which_val)
	{
	  matches = 0;
	  new_combo[0] = 0;
	  new_combo[1] = 0;
	  new_combo[2] = 0;
	  new_combo[3] = 0;
	  new_combo[4] = 0;
	  new_combo[5] = 0;
	  new_combo[6] = 0;

	  if ((text_tool->foundry == 0) || (text_tool->foundry == font->combos[i][0]))
	    {
	      matches++;
	      if (text_tool->foundry)
		new_combo[0] = font->combos[i][0];
	    }
	  if ((text_tool->weight == 0) || (text_tool->weight == font->combos[i][1]))
	    {
	      matches++;
	      if (text_tool->weight)
		new_combo[1] = font->combos[i][1];
	    }
	  if ((text_tool->slant == 0) || (text_tool->slant == font->combos[i][2]))
	    {
	      matches++;
	      if (text_tool->slant)
		new_combo[2] = font->combos[i][2];
	    }
	  if ((text_tool->set_width == 0) || (text_tool->set_width == font->combos[i][3]))
	    {
	      matches++;
	      if (text_tool->set_width)
		new_combo[3] = font->combos[i][3];
	    }
	  if ((text_tool->spacing == 0) || (text_tool->spacing == font->combos[i][4]))
	    {
	      matches++;
	      if (text_tool->spacing)
		new_combo[4] = font->combos[i][4];
	    }
	  if ((text_tool->registry == 0) || (text_tool->registry == font->combos[i][5]))
	    {
	      matches++;
	      if (text_tool->registry)
		new_combo[5] = font->combos[i][5];
	    }
	  if ((text_tool->encoding == 0) || (text_tool->encoding == font->combos[i][6]))
	    {
	      matches++;
	      if (text_tool->encoding)
		new_combo[6] = font->combos[i][6];
	    }

	  /* if we get all 7 matches simply return */
	  if (matches == 7)
	    return;

	  if (matches > best_matches)
	    {
	      best_matches = matches;
	      best_combo[0] = new_combo[0];
	      best_combo[1] = new_combo[1];
	      best_combo[2] = new_combo[2];
	      best_combo[3] = new_combo[3];
	      best_combo[4] = new_combo[4];
	      best_combo[5] = new_combo[5];
	      best_combo[6] = new_combo[6];
	    }
	}
    }

  if (best_matches > -1)
    {
      if (text_tool->foundry != best_combo[0])
	{
	  text_tool->foundry = best_combo[0];
	  if (which)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[0]), text_tool->foundry);
	}
      if (text_tool->weight != best_combo[1])
	{
	  text_tool->weight = best_combo[1];
	  if (which != 1)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[1]), text_tool->weight);
	}
      if (text_tool->slant != best_combo[2])
	{
	  text_tool->slant = best_combo[2];
	  if (which != 2)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[2]), text_tool->slant);
	}
      if (text_tool->set_width != best_combo[3])
	{
	  text_tool->set_width = best_combo[3];
	  if (which != 3)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[3]), text_tool->set_width);
	}
      if (text_tool->spacing != best_combo[4])
	{
	  text_tool->spacing = best_combo[4];
	  if (which != 4)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[4]), text_tool->spacing);
	}
      if (text_tool->registry != best_combo[5])
	{
	  text_tool->registry = best_combo[5];
	  if (which != 5)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[5]), text_tool->registry);
	}
      if (text_tool->encoding != best_combo[6])
	{
	  text_tool->encoding = best_combo[6];
	  if (which != 6)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[6]), text_tool->encoding);
	}
    }
}

static void
text_get_fonts ()
{
  char **fontnames;
  char *fontname;
  char *field;
  GSList *temp_list;
  int num_fonts;
  int index;
  int i, j;

  /* construct a valid font pattern */

  fontnames = XListFonts (DISPLAY, "-*-*-*-*-*-*-0-0-75-75-*-0-*-*", 32767, &num_fonts);

  /* the maximum size of the table is the number of font names returned */
  font_info = g_malloc (sizeof (FontInfo**) * num_fonts);

  /* insert the fontnames into a table */
  nfonts = 0;
  for (i = 0; i < num_fonts; i++)
    if (text_is_xlfd_font_name (fontnames[i]))
      {
	text_insert_font (font_info, &nfonts, fontnames[i]);

	foundries = text_insert_field (foundries, fontnames[i], FOUNDRY);
	weights = text_insert_field (weights, fontnames[i], WEIGHT);
	slants = text_insert_field (slants, fontnames[i], SLANT);
	set_widths = text_insert_field (set_widths, fontnames[i], SET_WIDTH);
	spacings = text_insert_field (spacings, fontnames[i], SPACING);
	registries = text_insert_field (registries, fontnames[i], REGISTRY);
	encodings = text_insert_field (encodings, fontnames[i], ENCODING);
      }

  XFreeFontNames (fontnames);

  nfoundries = g_slist_length (foundries) + 1;
  nweights = g_slist_length (weights) + 1;
  nslants = g_slist_length (slants) + 1;
  nset_widths = g_slist_length (set_widths) + 1;
  nspacings = g_slist_length (spacings) + 1;
  nregistries = g_slist_length (registries) + 1;
  nencodings = g_slist_length (encodings) + 1;

  foundry_array = g_malloc (sizeof (char*) * nfoundries);
  weight_array = g_malloc (sizeof (char*) * nweights);
  slant_array = g_malloc (sizeof (char*) * nslants);
  set_width_array = g_malloc (sizeof (char*) * nset_widths);
  spacing_array = g_malloc (sizeof (char*) * nspacings);
  registry_array = g_malloc (sizeof (char*) * nregistries);
  encoding_array = g_malloc (sizeof (char*) * nencodings);

  i = 1;
  temp_list = foundries;
  while (temp_list)
    {
      foundry_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = weights;
  while (temp_list)
    {
      weight_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = slants;
  while (temp_list)
    {
      slant_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = set_widths;
  while (temp_list)
    {
      set_width_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = spacings;
  while (temp_list)
    {
      spacing_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = registries;
  while (temp_list)
    {
      registry_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = encodings;
  while (temp_list)
    {
      encoding_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  foundry_array[0] = "*";
  weight_array[0] = "*";
  slant_array[0] = "*";
  set_width_array[0] = "*";
  spacing_array[0] = "*";
  registry_array[0] = "*";
  encoding_array[0] = "*";

  for (i = 0; i < nfonts; i++)
    {
      font_info[i]->foundries = g_malloc (sizeof (int) * nfoundries);
      font_info[i]->weights = g_malloc (sizeof (int) * nweights);
      font_info[i]->slants = g_malloc (sizeof (int) * nslants);
      font_info[i]->set_widths = g_malloc (sizeof (int) * nset_widths);
      font_info[i]->spacings = g_malloc (sizeof (int) * nspacings);
      font_info[i]->registries = g_malloc (sizeof (int) * nregistries);
      font_info[i]->encodings = g_malloc (sizeof (int) * nencodings);
      font_info[i]->ncombos = g_slist_length (font_info[i]->fontnames);
      font_info[i]->combos = g_malloc (sizeof (int*) * font_info[i]->ncombos);

      for (j = 0; j < nfoundries; j++)
	font_info[i]->foundries[j] = 0;
      for (j = 0; j < nweights; j++)
	font_info[i]->weights[j] = 0;
      for (j = 0; j < nslants; j++)
	font_info[i]->slants[j] = 0;
      for (j = 0; j < nset_widths; j++)
	font_info[i]->set_widths[j] = 0;
      for (j = 0; j < nspacings; j++)
	font_info[i]->spacings[j] = 0;
      for (j = 0; j < nregistries; j++)
	font_info[i]->registries[j] = 0;
      for (j = 0; j < nencodings; j++)
	font_info[i]->encodings[j] = 0;

      font_info[i]->foundries[0] = 1;
      font_info[i]->weights[0] = 1;
      font_info[i]->slants[0] = 1;
      font_info[i]->set_widths[0] = 1;
      font_info[i]->spacings[0] = 1;
      font_info[i]->registries[0] = 1;
      font_info[i]->encodings[0] = 1;

      j = 0;
      temp_list = font_info[i]->fontnames;
      while (temp_list)
	{
	  fontname = temp_list->data;
	  temp_list = temp_list->next;

	  font_info[i]->combos[j] = g_malloc (sizeof (int) * 7);

	  field = text_get_field (fontname, FOUNDRY);
	  index = text_field_to_index (foundry_array, nfoundries, field);
	  font_info[i]->foundries[index] = 1;
	  font_info[i]->combos[j][0] = index;
	  free (field);

	  field = text_get_field (fontname, WEIGHT);
	  index = text_field_to_index (weight_array, nweights, field);
	  font_info[i]->weights[index] = 1;
	  font_info[i]->combos[j][1] = index;
	  free (field);

	  field = text_get_field (fontname, SLANT);
	  index = text_field_to_index (slant_array, nslants, field);
	  font_info[i]->slants[index] = 1;
	  font_info[i]->combos[j][2] = index;
	  free (field);

	  field = text_get_field (fontname, SET_WIDTH);
	  index = text_field_to_index (set_width_array, nset_widths, field);
	  font_info[i]->set_widths[index] = 1;
	  font_info[i]->combos[j][3] = index;
	  free (field);

	  field = text_get_field (fontname, SPACING);
	  index = text_field_to_index (spacing_array, nspacings, field);
	  font_info[i]->spacings[index] = 1;
	  font_info[i]->combos[j][4] = index;
	  free (field);

	  field = text_get_field (fontname, REGISTRY);
	  index = text_field_to_index (registry_array, nregistries, field);
	  font_info[i]->registries[index] = 1;
	  font_info[i]->combos[j][5] = index;
	  free (field);

	  field = text_get_field (fontname, ENCODING);
	  index = text_field_to_index (encoding_array, nencodings, field);
	  font_info[i]->encodings[index] = 1;
	  font_info[i]->combos[j][6] = index;
	  free (field);

	  j += 1;
	}
    }
}

static void
text_insert_font (FontInfo **table,
		  int       *ntable,
		  char      *fontname)
{
  FontInfo *temp_info;
  char *family;
  int lower, upper;
  int middle, cmp;

  /* insert a fontname into a table */
  family = text_get_field (fontname, FAMILY);
  if (!family)
    return;

  lower = 0;
  if (*ntable > 0)
    {
      /* Do a binary search to determine if we have already encountered
       *  a font from this family.
       */
      upper = *ntable;
      while (lower < upper)
	{
	  middle = (lower + upper) >> 1;

	  cmp = strcmp (family, table[middle]->family);
	  if (cmp == 0)
	    {
	      table[middle]->fontnames = g_slist_prepend (table[middle]->fontnames, g_strdup (fontname));
	      g_free (family);
	      return;
	    }
	  else if (cmp < 0)
	    upper = middle;
	  else if (cmp > 0)
	    lower = middle+1;
	}
    }

  /* Add another entry to the table for this new font family */
  table[*ntable] = g_malloc (sizeof (FontInfo));
  table[*ntable]->family = family;
  table[*ntable]->foundries = NULL;
  table[*ntable]->weights = NULL;
  table[*ntable]->slants = NULL;
  table[*ntable]->set_widths = NULL;
  table[*ntable]->spacings = NULL;
  table[*ntable]->registries = NULL;
  table[*ntable]->encodings = NULL;
  table[*ntable]->fontnames = NULL;
  table[*ntable]->fontnames = g_slist_prepend (table[*ntable]->fontnames, g_strdup (fontname));
  (*ntable)++;

  /* Quickly insert the entry into the table in sorted order
   *  using a modification of insertion sort and the knowledge
   *  that the entries proper position in the table was determined
   *  above in the binary search and is contained in the "lower"
   *  variable.
   */
  if (*ntable > 1)
    {
      temp_info = table[*ntable - 1];

      upper = *ntable - 1;
      while (lower != upper)
	{
	  table[upper] = table[upper-1];
	  upper -= 1;
	}

      table[lower] = temp_info;
    }
}

static int
font_compare_func (gpointer a, gpointer b)
{
    return strcmp (a, b);
}

static GSList *
text_insert_field (GSList  *list,
		   char    *fontname,
		   int      field_num)
{
  GSList *tmp_list = list;
  GSList *prev_list = NULL;
  GSList *new_list;
  gint cmp;
  char *field;

  field = text_get_field (fontname, field_num);
  if (!field)
    return list;

  if (!list)
    {
      new_list = g_slist_alloc();
      new_list->data = field;
      return new_list;
    }

  cmp = font_compare_func (field, tmp_list->data);
 
  while ((tmp_list->next) && (cmp > 0))
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;
      cmp = font_compare_func (field, tmp_list->data);
    }

  if (cmp == 0)
    {
      g_free (field);
      return list;
    }

  new_list = g_slist_alloc();
  new_list->data = field;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      return list;
    }

  if (prev_list)
    {
      prev_list->next = new_list;
      new_list->next = tmp_list;
      return list;
    }
  else
    {
      new_list->next = list;
      return new_list;
    }
}

static char*
text_get_field (char *fontname,
		int   field_num)
{
  char *t1, *t2;
  char *field;

  /* we assume this is a valid fontname...that is, it has 14 fields */

  t1 = fontname;
  while (*t1 && (field_num >= 0))
    if (*t1++ == '-')
      field_num--;

  t2 = t1;
  while (*t2 && (*t2 != '-'))
    t2++;

  if (t1 != t2)
    {
      field = g_malloc (1 + (long) t2 - (long) t1);
      strncpy (field, t1, (long) t2 - (long) t1);
      field[(long) t2 - (long) t1] = 0;
      return field;
    }

  return g_strdup ("(nil)");
}

static int
text_field_to_index (char **table,
		     int    ntable,
		     char  *field)
{
  int i;

  for (i = 0; i < ntable; i++)
    if (strcmp (field, table[i]) == 0)
      return i;

  return -1;
}

static int
text_is_xlfd_font_name (char *fontname)
{
  int i;

  i = 0;
  while (*fontname)
    if (*fontname++ == '-')
      i++;

  return (i == 14);
}

static int
text_get_xlfd (double  size,
	       int     size_type,
	       char   *foundry,
	       char   *family,
	       char   *weight,
	       char   *slant,
	       char   *set_width,
	       char   *spacing,
	       char   *registry,
	       char   *encoding,
	       char   *fontname)
{
  char pixel_size[12], point_size[12];

  if (size > 0)
    {
      switch (size_type)
	{
	case PIXELS:
	  sprintf (pixel_size, "%d", (int) size);
	  sprintf (point_size, "*");
	  break;
	case POINTS:
	  sprintf (pixel_size, "*");
	  sprintf (point_size, "%d", (int) (size * 10));
	  break;
	}

      /* create the fontname */
      sprintf (fontname, "-%s-%s-%s-%s-%s-*-%s-%s-75-75-%s-*-%s-%s",
	       foundry,
	       family,
	       weight,
	       slant,
	       set_width,
	       pixel_size, point_size,
	       spacing,
	       registry, encoding);
      return TRUE;
    }
  else
    return FALSE;
}

static int
text_load_font (TextTool *text_tool)
{
  GdkFont *font;
  char fontname[2048];
  double size;
  char *size_text;
  char *foundry_str;
  char *family_str;
  char *weight_str;
  char *slant_str;
  char *set_width_str;
  char *spacing_str;
  char *registry_str;
  char *encoding_str;

  size_text = gtk_entry_get_text (GTK_ENTRY (text_tool->size_text));
  size = atof (size_text);

  foundry_str = foundry_array[text_tool->foundry];
  if (strcmp (foundry_str, "(nil)") == 0)
    foundry_str = "";
  family_str = font_info[text_tool->font_index]->family;
  weight_str = weight_array[text_tool->weight];
  if (strcmp (weight_str, "(nil)") == 0)
    weight_str = "";
  slant_str = slant_array[text_tool->slant];
  if (strcmp (slant_str, "(nil)") == 0)
    slant_str = "";
  set_width_str = set_width_array[text_tool->set_width];
  if (strcmp (set_width_str, "(nil)") == 0)
    set_width_str = "";
  spacing_str = spacing_array[text_tool->spacing];
  if (strcmp (spacing_str, "(nil)") == 0)
    spacing_str = "";
  registry_str = registry_array[text_tool->registry];
  if (strcmp (registry_str, "(nil)") == 0)
    registry_str = "";
  encoding_str = encoding_array[text_tool->encoding];
  if (strcmp (encoding_str, "(nil)") == 0)
    encoding_str = "";

  if (text_get_xlfd (size, text_tool->size_type, foundry_str, family_str,
		     weight_str, slant_str, set_width_str, spacing_str,
		     registry_str, encoding_str, fontname))
    {
      /* Trap errors with bad fonts -Yosh */
      gdk_error_warnings = 0;
      gdk_error_code = 0;
      font = gdk_font_load (fontname);
      gdk_error_warnings = 1;

      if (gdk_error_code == -1)
	{
	  g_message ("I'm sorry, but the font %s is corrupt.\n"
                     "Please ask the system adminstrator to replace it.",
                     family_str);
	  return FALSE;
	}

      if (font)
	{
	  if (text_tool->font)
	    gdk_font_unref (text_tool->font);
	  text_tool->font = font;
	  text_resize_text_widget (text_tool);

	  return TRUE;
	}
    }

  return FALSE;
}

static void
text_init_render (TextTool *text_tool)
{
  GDisplay *gdisp;
  char fontname[2048];
  char *text;
  int border;
  char *border_text;
  double size;
  char *size_text;
  char *foundry_str;
  char *family_str;
  char *weight_str;
  char *slant_str;
  char *set_width_str;
  char *spacing_str;
  char *registry_str;
  char *encoding_str;

  size_text = gtk_entry_get_text (GTK_ENTRY (text_tool->size_text));
  size = atof (size_text);
  if (text_tool->antialias)
    size *= SUPERSAMPLE;

  foundry_str = foundry_array[text_tool->foundry];
  if (strcmp (foundry_str, "(nil)") == 0)
    foundry_str = "";
  family_str = font_info[text_tool->font_index]->family;
  weight_str = weight_array[text_tool->weight];
  if (strcmp (weight_str, "(nil)") == 0)
    weight_str = "";
  slant_str = slant_array[text_tool->slant];
  if (strcmp (slant_str, "(nil)") == 0)
    slant_str = "";
  set_width_str = set_width_array[text_tool->set_width];
  if (strcmp (set_width_str, "(nil)") == 0)
    set_width_str = "";
  spacing_str = spacing_array[text_tool->spacing];
  if (strcmp (spacing_str, "(nil)") == 0)
    spacing_str = "";
  registry_str = registry_array[text_tool->registry];
  if (strcmp (registry_str, "(nil)") == 0)
    registry_str = "";
  encoding_str = encoding_array[text_tool->encoding];
  if (strcmp (encoding_str, "(nil)") == 0)
    encoding_str = "";

  if (text_get_xlfd (size, text_tool->size_type, foundry_str, family_str,
		     weight_str, slant_str, set_width_str, spacing_str,
		     registry_str, encoding_str, fontname))
    {
      /* get the text */
      text = gtk_entry_get_text (GTK_ENTRY (text_tool->the_text));

      border_text = gtk_entry_get_text (GTK_ENTRY (text_tool->border_text));
      border = atoi (border_text);

      gdisp = (GDisplay *) text_tool->gdisp_ptr;

      text_render (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
		   text_tool->click_x, text_tool->click_y,
		   fontname, text, border, text_tool->antialias);

      gdisplays_flush ();
    }
}

static void
text_gdk_image_to_region (GdkImage    *image,
			  int          scale,
			  PixelRegion *textPR)
{
  int black_pixel;
  int pixel;
  int value;
  int scalex, scaley;
  int scale2;
  int x, y;
  int i, j;
  unsigned char * data;

  scale2 = scale * scale;
  black_pixel = BlackPixel (DISPLAY, DefaultScreen (DISPLAY));
  data = textPR->data;

  for (y = 0, scaley = 0; y < textPR->h; y++, scaley += scale)
    {
      for (x = 0, scalex = 0; x < textPR->w; x++, scalex += scale)
	{
	  value = 0;

	  for (i = scaley; i < scaley + scale; i++)
	    for (j = scalex; j < scalex + scale; j++)
	      {
		pixel = gdk_image_get_pixel (image, j, i);
		if (pixel == black_pixel)
		  value ++;
	      }

	  /*  store the alpha value in the data  */
	  *data++= (unsigned char) ((value * 255) / scale2);

	}
    }
}

static Layer *
text_render (GImage *gimage,
	     GimpDrawable *drawable,
	     int     text_x,
	     int     text_y,
	     char   *fontname,
	     char   *text,
	     int     border,
	     int     antialias)
{
  GdkFont *font;
  GdkPixmap *pixmap;
  GdkImage *image;
  GdkGC *gc;
  GdkColor black, white;
  Layer *layer;
  TileManager *mask, *newmask;
  PixelRegion textPR, maskPR;
  int layer_type;
  unsigned char color[MAX_CHANNELS];
  char *str;
  int nstrs;
  int crop;
  int line_width, line_height;
  int pixmap_width, pixmap_height;
  int text_width, text_height;
  int width, height;
  int x, y, k;
  void * pr;

  /*  determine the layer type  */
  if (drawable)
    layer_type = drawable_type_with_alpha (drawable);
  else
    layer_type = gimage_base_type_with_alpha (gimage);

  /* scale the text based on the antialiasing amount */
  if (antialias)
    antialias = SUPERSAMPLE;
  else
    antialias = 1;

  /* Dont crop the text if border is negative */
  crop = (border >= 0);
  if (!crop) border = 0;

  /* load the font in */
  gdk_error_warnings = 0;
  gdk_error_code = 0;
  font = gdk_font_load (fontname);
  gdk_error_warnings = 1;
  if (!font || (gdk_error_code == -1))
    return NULL;

  /* determine the bounding box of the text */
  width = -1;
  height = 0;
  line_height = font->ascent + font->descent;

  nstrs = 0;
  str = strtok (text, "\n");
  while (str)
    {
      nstrs += 1;

      /* gdk_string_measure will give the correct width of the
       *  string. However, we'll add a little "fudge" factor just
       *  to be sure.
       */
      line_width = gdk_string_measure (font, str) + 5;
      if (line_width > width)
	width = line_width;
      height += line_height;

      str = strtok (NULL, "\n");
    }

  /* We limit the largest pixmap we create to approximately 200x200.
   * This is approximate since it depends on the amount of antialiasing.
   * Basically, we want the width and height to be divisible by the antialiasing
   *  amount. (Which lies in the range 1-10).
   * This avoids problems on some X-servers (Xinside) which have problems
   *  with large pixmaps. (Specifically pixmaps which are larger - width
   *  or height - than the screen).
   */
  pixmap_width = TILE_WIDTH * antialias;
  pixmap_height = TILE_HEIGHT * antialias;

  /* determine the actual text size based on the amount of antialiasing */
  text_width = width / antialias;
  text_height = height / antialias;

  /* create the pixmap of depth 1 */
  pixmap = gdk_pixmap_new (NULL, pixmap_width, pixmap_height, 1);

  /* create the gc */
  gc = gdk_gc_new (pixmap);
  gdk_gc_set_font (gc, font);

  /*  get black and white pixels for this gdisplay  */
  black.pixel = BlackPixel (DISPLAY, DefaultScreen (DISPLAY));
  white.pixel = WhitePixel (DISPLAY, DefaultScreen (DISPLAY));

  /* Render the text into the pixmap.
   * Since the pixmap may not fully bound the text (because we limit its size)
   *  we must tile it around the texts actual bounding box.
   */
  mask = tile_manager_new (text_width, text_height, 1);
  pixel_region_init (&maskPR, mask, 0, 0, text_width, text_height, TRUE);

  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      /* erase the pixmap */
      gdk_gc_set_foreground (gc, &white);
      gdk_draw_rectangle (pixmap, gc, 1, 0, 0, pixmap_width, pixmap_height);
      gdk_gc_set_foreground (gc, &black);

      /* adjust the x and y values */
      x = -maskPR.x * antialias;
      y = font->ascent - maskPR.y * antialias;
      str = text;

      for (k = 0; k < nstrs; k++)
	{
	  gdk_draw_string (pixmap, font, gc, x, y, str);
	  str += strlen (str) + 1;
	  y += line_height;
	}

      /* create the GdkImage */
      image = gdk_image_get (pixmap, 0, 0, pixmap_width, pixmap_height);

      if (!image)
	fatal_error ("sanity check failed: could not get gdk image");

      if (image->depth != 1)
	fatal_error ("sanity check failed: image should have 1 bit per pixel");

      /* convert the GdkImage bitmap to a region */
      text_gdk_image_to_region (image, antialias, &maskPR);

      /* free the image */
      gdk_image_destroy (image);
    }

  /*  Crop the mask buffer  */
  newmask = crop ? crop_buffer (mask, border) : mask;
  if (newmask != mask)
    tile_manager_destroy (mask);

  if (newmask && 
      (layer = layer_new (gimage->ID, newmask->levels[0].width,
			 newmask->levels[0].height, layer_type,
			 "Text Layer", OPAQUE_OPACITY, NORMAL_MODE)))
    {
      /*  color the layer buffer  */
      gimage_get_foreground (gimage, drawable, color);
      color[GIMP_DRAWABLE(layer)->bytes - 1] = OPAQUE_OPACITY;
      pixel_region_init (&textPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
      color_region (&textPR, color);

      /*  apply the text mask  */
      pixel_region_init (&textPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
      pixel_region_init (&maskPR, newmask, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
      apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);

      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      /*  Set the layer offsets  */
      GIMP_DRAWABLE(layer)->offset_x = text_x;
      GIMP_DRAWABLE(layer)->offset_y = text_y;

      /*  If there is a selection mask clear it--
       *  this might not always be desired, but in general,
       *  it seems like the correct behavior.
       */
      if (! gimage_mask_is_empty (gimage))
	channel_clear (gimage_get_mask (gimage));

      /*  If the drawable id is invalid, create a new layer  */
      if (drawable == NULL)
	gimage_add_layer (gimage, layer, -1);
      /*  Otherwise, instantiate the text as the new floating selection */
      else
	floating_sel_attach (layer, drawable);

      /*  end the group undo  */
      undo_push_group_end (gimage);

      tile_manager_destroy (newmask);
    }
  else 
    {
      if (newmask) 
	{
	  g_message ("text_render: could not allocate image");
          tile_manager_destroy (newmask);
	}
      layer = NULL;
    }

  /* free the pixmap */
  gdk_pixmap_unref (pixmap);

  /* free the gc */
  gdk_gc_destroy (gc);

  /* free the font */
  gdk_font_unref (font);

  return layer;
}


static int
text_get_extents (char *fontname,
		  char *text,
		  int  *width,
		  int  *height,
		  int  *ascent,
		  int  *descent)
{
  GdkFont *font;
  char *str;
  int nstrs;
  int line_width, line_height;

  /* load the font in */
  gdk_error_warnings = 0;
  gdk_error_code = 0;
  font = gdk_font_load (fontname);
  gdk_error_warnings = 1;
  if (!font || (gdk_error_code == -1))
    return FALSE;

  /* determine the bounding box of the text */
  *width = -1;
  *height = 0;
  *ascent = font->ascent;
  *descent = font->descent;
  line_height = *ascent + *descent;

  nstrs = 0;
  str = strtok (text, "\n");
  while (str)
    {
      nstrs += 1;

      /* gdk_string_measure will give the correct width of the
       *  string. However, we'll add a little "fudge" factor just
       *  to be sure.
       */
      line_width = gdk_string_measure (font, str) + 5;
      if (line_width > *width)
	*width = line_width;
      *height += line_height;

      str = strtok (NULL, "\n");
    }

  if (*width < 0)
    return FALSE;
  else
    return TRUE;
}

/****************************************/
/*  The text_tool procedure definition  */
ProcArg text_tool_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The affected drawable: (-1 for a new text layer)"
  },
  { PDB_FLOAT,
    "x",
    "the x coordinate for the left side of text bounding box"
  },
  { PDB_FLOAT,
    "y",
    "the y coordinate for the top of text bounding box"
  },
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_INT32,
    "border",
    "the size of the border: border >= 0"
  },
  { PDB_INT32,
    "antialias",
    "generate antialiased text"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  }
};

ProcArg text_tool_out_args[] =
{
  { PDB_LAYER,
    "text_layer",
    "the new text layer"
  }
};

ProcRecord text_tool_proc =
{
  "gimp_text",
  "Add text at the specified location as a floating selection or a new layer.",
  "This tool requires font information in the form of seven parameters: {size, foundry, family, weight, slant, set_width, spacing}.  The font size can either be specified in units of pixels or points, and the appropriate metric is specified using the size_type "
  "argument.  The x and y parameters together control the placement of the new text by specifying the upper left corner of the text bounding box.  If the antialias parameter is non-zero, the generated text will blend more smoothly with underlying layers.  "
  "This option requires more time and memory to compute than non-antialiased text; the resulting floating selection or layer, however, will require the same amount of memory with or without antialiasing.  If the specified drawable parameter is valid, the "
  "text will be created as a floating selection attached to the drawable.  If the drawable parameter is not valid (-1), the text will appear as a new layer.  Finally, a border can be specified around the final rendered text.  The border is measured in pixels.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  15,
  text_tool_args,

  /*  Output arguments  */
  1,
  text_tool_out_args,

  /*  Exec method  */
  { { text_tool_invoker } },
};

/********************************************/
/*  The text_tool_ext procedure definition  */
ProcArg text_tool_args_ext[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The affected drawable: (-1 for a new text layer)"
  },
  { PDB_FLOAT,
    "x",
    "the x coordinate for the left side of text bounding box"
  },
  { PDB_FLOAT,
    "y",
    "the y coordinate for the top of text bounding box"
  },
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_INT32,
    "border",
    "the size of the border: border >= 0"
  },
  { PDB_INT32,
    "antialias",
    "generate antialiased text"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  },
  { PDB_STRING,
    "registry",
    "the font registry, \"*\" for any"
  },
  { PDB_STRING,
    "encoding",
    "the font encoding, \"*\" for any"
  }
};

ProcArg text_tool_out_args_ext[] =
{
  { PDB_LAYER,
    "text_layer",
    "the new text layer"
  }
};

ProcRecord text_tool_proc_ext =
{
  "gimp_text_ext",
  "Add text at the specified location as a floating selection or a new layer.",
  "This tool requires font information in the form of nine parameters: {size, foundry, family, weight, slant, set_width, spacing, registry, encoding}.  The font size can either be specified in units of pixels or points, and the appropriate metric is specified using the size_type "
  "argument.  The x and y parameters together control the placement of the new text by specifying the upper left corner of the text bounding box.  If the antialias parameter is non-zero, the generated text will blend more smoothly with underlying layers.  "
  "This option requires more time and memory to compute than non-antialiased text; the resulting floating selection or layer, however, will require the same amount of memory with or without antialiasing.  If the specified drawable parameter is valid, the "
  "text will be created as a floating selection attached to the drawable.  If the drawable parameter is not valid (-1), the text will appear as a new layer.  Finally, a border can be specified around the final rendered text.  The border is measured in pixels.",
  "Martin Edlman",
  "Spencer Kimball & Peter Mattis",
  "1998",
  PDB_INTERNAL,
    
  /*  Input arguments  */
  17,
  text_tool_args_ext,

  /*  Output arguments  */
  1,
  text_tool_out_args_ext,

  /*  Exec method  */
  { { text_tool_invoker_ext } },
};


/**********************/
/*  TEXT_GET_EXTENTS  */

ProcArg text_tool_get_extents_args[] =
{
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  }
};

ProcArg text_tool_get_extents_out_args[] =
{
  { PDB_INT32,
    "width",
    "the width of the specified text"
  },
  { PDB_INT32,
    "height",
    "the height of the specified text"
  },
  { PDB_INT32,
    "ascent",
    "the ascent of the specified font"
  },
  { PDB_INT32,
    "descent",
    "the descent of the specified font"
  }
};

ProcRecord text_tool_get_extents_proc =
{
  "gimp_text_get_extents",
  "Get extents of the bounding box for the specified text",
  "This tool returns the width and height of a bounding box for the specified text string with the specified font information.  Ascent and descent for the specified font are returned as well.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  9,
  text_tool_get_extents_args,

  /*  Output arguments  */
  4,
  text_tool_get_extents_out_args,

  /*  Exec method  */
  { { text_tool_get_extents_invoker } },
};

/**************************/
/*  TEXT_GET_EXTENTS_EXT  */
ProcArg text_tool_get_extents_args_ext[] =
{
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  },
  { PDB_STRING,
    "registry",
    "the font registry, \"*\" for any"
  },
  { PDB_STRING,
    "encoding",
    "the font encoding, \"*\" for any"
  }
};

ProcArg text_tool_get_extents_out_args_ext[] =
{
  { PDB_INT32,
    "width",
    "the width of the specified text"
  },
  { PDB_INT32,
    "height",
    "the height of the specified text"
  },
  { PDB_INT32,
    "ascent",
    "the ascent of the specified font"
  },
  { PDB_INT32,
    "descent",
    "the descent of the specified font"
  }
};

ProcRecord text_tool_get_extents_proc_ext =
{
  "gimp_text_get_extents_ext",
  "Get extents of the bounding box for the specified text",
  "This tool returns the width and height of a bounding box for the specified text string with the specified font information.  Ascent and descent for the specified font are returned as well.",
  "Martin Edlman",
  "Spencer Kimball & Peter Mattis",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  11,
  text_tool_get_extents_args_ext,

  /*  Output arguments  */
  4,
  text_tool_get_extents_out_args_ext,

  /*  Exec method  */
  { { text_tool_get_extents_invoker_ext } },
};


static Argument *
text_tool_invoker (Argument *args)
{
  int i;
  Argument argv[17];
 
  for (i=0; i<15; i++)
    argv[i] = args[i];
  argv[15].arg_type = PDB_STRING;
  argv[15].value.pdb_pointer = (gpointer)"*";
  argv[16].arg_type = PDB_STRING;
  argv[16].value.pdb_pointer = (gpointer)"*";
  return text_tool_invoker_ext (argv);
}

static Argument *
text_tool_invoker_ext (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  Layer *text_layer;
  GimpDrawable *drawable;
  double x, y;
  char *text;
  int border;
  int antialias;
  double size;
  int size_type;
  char *foundry;
  char *family;
  char *weight;
  char *slant;
  char *set_width;
  char *spacing;
  char *registry;
  char *encoding;
  int int_value;
  double fp_value;
  char fontname[2048];
  Argument *return_args;

  text_layer  = NULL;
  drawable    = NULL;
  x           = 0;
  y           = 0;
  text        = NULL;
  border      = FALSE;
  antialias   = FALSE;
  size        = 0;
  size_type   = PIXELS;
  foundry     = NULL;
  family      = NULL;
  weight      = NULL;
  slant       = NULL;
  set_width   = NULL;
  spacing     = NULL;
  registry    = NULL;
  encoding    = NULL;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable && gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  x, y coordinates  */
  if (success)
    {
      x = args[2].value.pdb_float;
      y = args[3].value.pdb_float;
    }
  /*  text  */
  if (success)
    text = (char *) args[4].value.pdb_pointer;
  /*  border  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      if (int_value >= -1)
	border = int_value;
      else
	success = FALSE;
    }
  /*  antialias  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  size  */
  if (success)
    {
      fp_value = args[7].value.pdb_float;
      if (fp_value > 0)
	size = fp_value;
      else
	success = FALSE;
    }
  /*  size type  */
  if (success)
    {
      int_value = args[8].value.pdb_int;
      switch (int_value)
	{
	case 0: size_type = PIXELS; break;
	case 1: size_type = POINTS; break;
	default: success = FALSE;
	}
    }
  /*  foundry, family, weight, slant, set_width, spacing, registry, encoding  */
  if (success)
    {
      foundry = (char *) args[9].value.pdb_pointer;
      family = (char *) args[10].value.pdb_pointer;
      weight = (char *) args[11].value.pdb_pointer;
      slant = (char *) args[12].value.pdb_pointer;
      set_width = (char *) args[13].value.pdb_pointer;
      spacing = (char *) args[14].value.pdb_pointer;
      registry = (char *) args[15].value.pdb_pointer;
      encoding = (char *) args[16].value.pdb_pointer;
    }

  /*  increase the size by SUPERSAMPLE if we're antialiasing  */
  if (antialias)
    size *= SUPERSAMPLE;

  /*  attempt to get the xlfd for the font  */
  if (success)
    success = text_get_xlfd (size, size_type, foundry, family, weight,
			     slant, set_width, spacing, registry, encoding, fontname);

  /*  call the text render procedure  */
  if (success)
    success = ((text_layer = text_render (gimage, drawable, x, y, fontname,
					  text, border, antialias)) != NULL);

  return_args = procedural_db_return_args (&text_tool_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(text_layer));

  return return_args;
}


static Argument *
text_tool_get_extents_invoker (Argument *args)
{
  int i;
  Argument argv[11];

  for (i=0; i<9; i++)
    argv[i] = args[i];
  argv[9].arg_type = PDB_STRING;
  argv[9].value.pdb_pointer = (gpointer)"*";
  argv[10].arg_type = PDB_STRING;
  argv[10].value.pdb_pointer = (gpointer)"*";
  return text_tool_get_extents_invoker_ext (argv);
}

static Argument *
text_tool_get_extents_invoker_ext (Argument *args)
{
  int success = TRUE;
  char *text;
  double size;
  int size_type;
  char *foundry;
  char *family;
  char *weight;
  char *slant;
  char *set_width;
  char *spacing;
  char *registry;
  char *encoding;
  int width, height;
  int ascent, descent;
  int int_value;
  double fp_value;
  char fontname[2048];
  Argument *return_args;

  size = 0.0;
  size_type = PIXELS;

  /*  text  */
  if (success)
    text = (char *) args[0].value.pdb_pointer;
  /*  size  */
  if (success)
    {
      fp_value = args[1].value.pdb_float;
      if (fp_value > 0)
	size = fp_value;
      else
	success = FALSE;
    }
  /*  size type  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: size_type = PIXELS; break;
	case 1: size_type = POINTS; break;
	default: success = FALSE;
	}
    }
  /*  foundry, family, weight, slant, set_width, spacing, registry, encoding  */
  if (success)
    {
      foundry = (char *) args[3].value.pdb_pointer;
      family = (char *) args[4].value.pdb_pointer;
      weight = (char *) args[5].value.pdb_pointer;
      slant = (char *) args[6].value.pdb_pointer;
      set_width = (char *) args[7].value.pdb_pointer;
      spacing = (char *) args[8].value.pdb_pointer;
      registry = (char *) args[9].value.pdb_pointer;
      encoding = (char *) args[10].value.pdb_pointer;
    }

  /*  attempt to get the xlfd for the font  */
  if (success)
    success = text_get_xlfd (size, size_type, foundry, family, weight,
			     slant, set_width, spacing, registry, encoding, fontname);

  /*  call the text render procedure  */
  if (success)
    success = text_get_extents (fontname, text, &width, &height, &ascent, &descent);

  return_args = procedural_db_return_args (&text_tool_get_extents_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = width;
      return_args[2].value.pdb_int = height;
      return_args[3].value.pdb_int = ascent;
      return_args[4].value.pdb_int = descent;
    }

  return return_args;
}

