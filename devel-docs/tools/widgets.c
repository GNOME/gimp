#include <gdk/gdkkeysyms.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "widgets.h"


#define SMALL_WIDTH  240
#define SMALL_HEIGHT 75
#define MEDIUM_WIDTH 240
#define MEDIUM_HEIGHT 165
#define LARGE_WIDTH 240
#define LARGE_HEIGHT 240

static Window
find_toplevel_window (Window xid)
{
  Window root, parent, *children;
  int nchildren;

  do
    {
      if (XQueryTree (gdk_display (), xid, &root,
                      &parent, &children, &nchildren) == 0)
        {
          g_warning ("Couldn't find window manager window");
          return None;
        }

      if (root == parent)
        return xid;

      xid = parent;
    }
  while (TRUE);
}


static gboolean
adjust_size_callback (WidgetInfo *info)
{
  Window toplevel;
  Window root;
  gint tx;
  gint ty;
  guint twidth;
  guint theight;
  guint tborder_width;
  guint tdepth;
  gint target_width = 0;
  gint target_height = 0;

  toplevel = GDK_WINDOW_XWINDOW (info->window->window);
  XGetGeometry (GDK_WINDOW_XDISPLAY (info->window->window),
                toplevel,
                &root, &tx, &ty, &twidth, &theight, &tborder_width, &tdepth);

  switch (info->size)
    {
    case SMALL:
      target_width = SMALL_WIDTH;
      target_height = SMALL_HEIGHT;
      break;
    case MEDIUM:
      target_width = MEDIUM_WIDTH;
      target_height = MEDIUM_HEIGHT;
      break;
    case LARGE:
      target_width = LARGE_WIDTH;
      target_height = LARGE_HEIGHT;
      break;
    case ASIS:
      target_width = twidth;
      target_height = theight;
      break;
    }

  if (twidth > target_width ||
      theight > target_height)
    {
      gtk_widget_set_size_request (info->window,
                                   2 + target_width - (twidth - target_width), /* Dunno why I need the +2 fudge factor; */
                                   2 + target_height - (theight - target_height));
    }
  return FALSE;
}

static void
realize_callback (GtkWidget  *widget,
                  WidgetInfo *info)
{
  g_timeout_add (500, (GSourceFunc)adjust_size_callback, info);
}

static WidgetInfo *
new_widget_info (const char *name,
                 GtkWidget  *widget,
                 WidgetSize  size)
{
  WidgetInfo *info;

  info = g_new0 (WidgetInfo, 1);
  info->name = g_strdup (name);
  info->size = size;
  if (GTK_IS_WINDOW (widget))
    {
      info->window = widget;
      gtk_window_set_resizable (GTK_WINDOW (info->window), FALSE);
      info->include_decorations = FALSE;
      g_signal_connect (info->window, "realize", G_CALLBACK (realize_callback), info);
    }
  else
    {
      info->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      info->include_decorations = FALSE;
      gtk_widget_show_all (widget);
      gtk_container_add (GTK_CONTAINER (info->window), widget);
    }
  info->no_focus = TRUE;

  gtk_widget_set_app_paintable (info->window, TRUE);
  g_signal_connect (info->window, "focus", G_CALLBACK (gtk_true), NULL);
  gtk_container_set_border_width (GTK_CONTAINER (info->window), 12);

  switch (size)
    {
    case SMALL:
      gtk_widget_set_size_request (info->window,
				   240, 75);
      break;
    case MEDIUM:
      gtk_widget_set_size_request (info->window,
				   240, 165);
      break;
    case LARGE:
      gtk_widget_set_size_request (info->window,
				   240, 240);
      break;
    default:
	break;
    }

  return info;
}

static WidgetInfo *
create_button (void)
{
  GtkWidget *widget;
  GtkWidget *align;

  widget = gimp_button_new();
  gtk_container_add (GTK_CONTAINER (widget),
                     gtk_label_new_with_mnemonic ("_Button"));
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("gimp-button", align, SMALL);
}

static WidgetInfo *
create_chain_button (void)
{
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *chain;
  GtkWidget *separator;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.8);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), align);
  table = gtk_table_new (2, 5, FALSE);
  gtk_container_add (GTK_CONTAINER (align), table);
  chain = gimp_chain_button_new (GIMP_CHAIN_LEFT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_table_attach (GTK_TABLE (table), chain, 0,1, 0,2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  label = gtk_label_new ("Linked ");
  gtk_table_attach (GTK_TABLE (table), label, 1,2, 0,1,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  label = gtk_label_new ("Linked ");
  gtk_table_attach (GTK_TABLE (table), label, 1,2, 1,2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  separator = gtk_vseparator_new ();
  gtk_table_attach (GTK_TABLE (table), separator, 2,3, 0,2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  label = gtk_label_new (" Unlinked");
  gtk_table_attach (GTK_TABLE (table), label, 3,4, 0,1,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  label = gtk_label_new (" Unlinked");
  gtk_table_attach (GTK_TABLE (table), label, 3,4, 1,2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), FALSE);
  gtk_table_attach (GTK_TABLE (table), chain, 4,5, 0,2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_box_pack_end_defaults (GTK_BOX (vbox), gtk_label_new ("Chain Button"));
  return new_widget_info ("gimp-chain-button", vbox, SMALL);
}

static WidgetInfo *
create_toggle_button (void)
{
  GtkWidget *widget;
  GtkWidget *align;

  widget = gtk_toggle_button_new_with_mnemonic ("_Toggle Button");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("toggle-button", align, SMALL);
}

static WidgetInfo *
create_radio (void)
{
  GtkWidget *widget;
  GtkWidget *radio;
  GtkWidget *align;

  widget = gtk_vbox_new (FALSE, 3);
  radio = gtk_radio_button_new_with_mnemonic (NULL, "Radio Button _One");
  gtk_box_pack_start (GTK_BOX (widget), radio, FALSE, FALSE, 0);
  radio = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (radio), "Radio Button _Two");
  gtk_box_pack_start (GTK_BOX (widget), radio, FALSE, FALSE, 0);
  radio = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (radio), "Radio Button T_hree");
  gtk_box_pack_start (GTK_BOX (widget), radio, FALSE, FALSE, 0);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("radio-group", align, MEDIUM);
}

static WidgetInfo *
create_label (void)
{
  GtkWidget *widget;
  GtkWidget *align;

  widget = gtk_label_new ("Label");
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("label", align, SMALL);
}

static WidgetInfo *
create_combo_box_entry (void)
{
  GtkWidget *widget;
  GtkWidget *align;
  
  gtk_rc_parse_string ("style \"combo-box-entry-style\" {\n"
		       "  GtkComboBox::appears-as-list = 1\n"
		       "}\n"
		       "widget_class \"GtkComboBoxEntry\" style \"combo-box-entry-style\"\n" );
  widget = gtk_combo_box_entry_new_text ();
  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (widget)->child), "Combo Box Entry");
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box-entry", align, SMALL);
}

static WidgetInfo *
create_combo_box (void)
{
  GtkWidget *widget;
  GtkWidget *align;
  
  gtk_rc_parse_string ("style \"combo-box-style\" {\n"
		       "  GtkComboBox::appears-as-list = 0\n"
		       "}\n"
		       "widget_class \"GtkComboBox\" style \"combo-box-style\"\n" );

  widget = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (widget), "Combo Box");
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box", align, SMALL);
}

static WidgetInfo *
create_color_area (void)
{
  GtkWidget *vbox;
  GtkWidget *area;
  GtkWidget *align;
  GimpRGB    color;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
  color.r = 0.8;
  color.g = 0.4;
  color.b = 0.2;
  color.a = 0.7;
  area = gimp_color_area_new (&color,
                              GIMP_COLOR_AREA_SMALL_CHECKS,
                              GDK_SHIFT_MASK);
  gimp_color_area_set_draw_border (GIMP_COLOR_AREA (area), TRUE);
  gtk_widget_set_size_request (area, -1, 25);
  gtk_container_add (GTK_CONTAINER (align), area);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Area"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-color-area", vbox, SMALL);
}

static WidgetInfo *
create_color_button (void)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *align;
  GimpRGB    color;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
  color.r = 0.8;
  color.g = 0.4;
  color.b = 0.2;
  color.a = 0.7;
  button =  gimp_color_button_new ("Color Button",
                                   80, 20, &color,
                                   GIMP_COLOR_AREA_SMALL_CHECKS);
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Button"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-color-button", vbox, SMALL);
}

static WidgetInfo *
create_color_hex_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *align;
  GimpRGB    color;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  entry = gimp_color_hex_entry_new ();
  color.r = 0.8;
  color.g = 0.4;
  color.b = 0.2;
  color.a = 0.7;
  gimp_color_hex_entry_set_color (GIMP_COLOR_HEX_ENTRY (entry), &color);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Hex Entry"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-color-hex-entry", vbox, SMALL);
}

static WidgetInfo *
create_color_scale (void)
{
  GtkWidget *vbox;
  GtkWidget *scale;
  GtkWidget *align;
  GimpRGB    rgb;
  GimpHSV    hsv;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.8, 0.0);
  scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                GIMP_COLOR_SELECTOR_HUE);
  rgb.r = 0.8;
  rgb.g = 0.4;
  rgb.b = 0.2;
  rgb.a = 0.7;
  gimp_rgb_to_hsv (&rgb, &hsv);
  gimp_color_scale_set_color (GIMP_COLOR_SCALE (scale),
                              &rgb, &hsv);
  gtk_range_set_value (GTK_RANGE (scale), 40);
  gtk_container_add (GTK_CONTAINER (align), scale);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Scale"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-color-scale", vbox, SMALL);
}

static WidgetInfo *
create_color_selection (void)
{
  GtkWidget *vbox;
  GtkWidget *selection;
  GtkWidget *align;
  GimpRGB    color;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  selection = gimp_color_selection_new ();
  color.r = 0.8;
  color.g = 0.4;
  color.b = 0.2;
  color.a = 0.7;
  gimp_color_selection_set_show_alpha(GIMP_COLOR_SELECTION (selection),
                                      TRUE);
  gimp_color_selection_set_color  (GIMP_COLOR_SELECTION (selection),
                                   &color);
  gtk_widget_set_size_request (selection, 400, -1);
  gtk_container_add (GTK_CONTAINER (align), selection);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Selection"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-color-selection", vbox, ASIS);
}

static WidgetInfo *
create_file_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *align;

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  entry = gimp_file_entry_new ("File Entry",
                               "wilber.png",
                               FALSE, TRUE);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("File Entry"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-file-entry", vbox, SMALL);
}

static WidgetInfo *
create_file_button (void)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *picker;
  GtkWidget *align;

  vbox = gtk_vbox_new (FALSE, 12);
  vbox2 = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = gtk_file_chooser_button_new ("File Chooser Button",
		  			GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_set_size_request (picker, 150, -1);
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (picker), "/etc/yum.conf");
  gtk_container_add (GTK_CONTAINER (align), picker);
  gtk_box_pack_start (GTK_BOX (vbox2), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2),
		      gtk_label_new ("File Button (Files)"),
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
		      vbox2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_hseparator_new (),
		      FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = gtk_file_chooser_button_new ("File Chooser Button",
		  			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_widget_set_size_request (picker, 150, -1);
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (picker), "/");
  gtk_container_add (GTK_CONTAINER (align), picker);
  gtk_box_pack_start (GTK_BOX (vbox2), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2),
		      gtk_label_new ("File Button (Select Folder)"),
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      vbox2, TRUE, TRUE, 0);

  return new_widget_info ("file-button", vbox, MEDIUM);
}

static WidgetInfo *
create_frame (void)
{
  GtkWidget *widget;

  widget = gtk_frame_new ("Frame");

  return new_widget_info ("frame", widget, MEDIUM);
}

static WidgetInfo *
create_colorsel (void)
{
  WidgetInfo *info;
  GtkWidget *widget;
  GtkColorSelection *colorsel;
  GdkColor color;

  widget = gtk_color_selection_dialog_new ("Color Selection Dialog");
  colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (widget)->colorsel);

  color.red   = 0x7979;
  color.green = 0xdbdb;
  color.blue  = 0x9595;

  gtk_color_selection_set_previous_color (colorsel, &color);
  
  color.red   = 0x7d7d;
  color.green = 0x9393;
  color.blue  = 0xc3c3;
  
  gtk_color_selection_set_current_color (colorsel, &color);

  info = new_widget_info ("colorsel", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_fontsel (void)
{
  WidgetInfo *info;
  GtkWidget *widget;

  widget = gtk_font_selection_dialog_new ("Font Selection Dialog");
  info = new_widget_info ("fontsel", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}
static WidgetInfo *
create_filesel (void)
{
  WidgetInfo *info;
  GtkWidget *widget;

  widget = gtk_file_chooser_dialog_new ("File Chooser Dialog",
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL); 
  gtk_window_set_default_size (GTK_WINDOW (widget), 505, 305);
  
  info = new_widget_info ("filechooser", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_toolbar (void)
{
  GtkWidget *widget, *menu;
  GtkToolItem *item;

  widget = gtk_toolbar_new ();

  item = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
  gtk_toolbar_insert (GTK_TOOLBAR (widget), item, -1);

  item = gtk_menu_tool_button_new_from_stock (GTK_STOCK_OPEN);
  menu = gtk_menu_new ();
  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (item), menu);
  gtk_toolbar_insert (GTK_TOOLBAR (widget), item, -1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  gtk_toolbar_insert (GTK_TOOLBAR (widget), item, -1);

  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (widget), FALSE);

  return new_widget_info ("toolbar", widget, SMALL);
}

static WidgetInfo *
create_menubar (void)
{
  GtkWidget *widget, *vbox, *align, *item;

  widget = gtk_menu_bar_new ();

  item = gtk_menu_item_new_with_mnemonic ("_File");
  gtk_menu_shell_append (GTK_MENU_SHELL (widget), item);

  item = gtk_menu_item_new_with_mnemonic ("_Edit");
  gtk_menu_shell_append (GTK_MENU_SHELL (widget), item);

  item = gtk_menu_item_new_with_mnemonic ("_Help");
  gtk_menu_shell_append (GTK_MENU_SHELL (widget), item);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_label_new ("Menu Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("menubar", vbox, SMALL);
}

static WidgetInfo *
create_message_dialog (void)
{
  GtkWidget *widget;

  widget = gtk_message_dialog_new (NULL,
				   0,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (widget),
				 "<b>Message Dialog</b>\n\nWith secondary text");
  return new_widget_info ("messagedialog", widget, MEDIUM);
}

static WidgetInfo *
create_notebook (void)
{
  GtkWidget *widget;

  widget = gtk_notebook_new ();

  gtk_notebook_append_page (GTK_NOTEBOOK (widget), 
			    gtk_label_new ("Notebook"),
			    NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (widget), gtk_event_box_new (), NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (widget), gtk_event_box_new (), NULL);

  return new_widget_info ("notebook", widget, MEDIUM);
}

static WidgetInfo *
create_progressbar (void)
{
  GtkWidget *vbox;
  GtkWidget *widget;
  GtkWidget *align;

  widget = gtk_progress_bar_new ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.5);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_label_new ("Progress Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("progressbar", vbox, SMALL);
}

static WidgetInfo *
create_scrolledwindow (void)
{
  GtkWidget *scrolledwin, *label;

  scrolledwin = gtk_scrolled_window_new (NULL, NULL);
  label = gtk_label_new ("Scrolled Window");

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwin), 
					 label);

  return new_widget_info ("scrolledwindow", scrolledwin, MEDIUM);
}

static WidgetInfo *
create_spinbutton (void)
{
  GtkWidget *widget;
  GtkWidget *vbox, *align;

  widget = gtk_spin_button_new_with_range (0.0, 100.0, 1.0);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_label_new ("Spin Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("spinbutton", vbox, SMALL);
}

static WidgetInfo *
create_statusbar (void)
{
  WidgetInfo *info;
  GtkWidget *widget;
  GtkWidget *vbox, *align;

  vbox = gtk_vbox_new (FALSE, 0);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), gtk_label_new ("Status Bar"));
  gtk_box_pack_start (GTK_BOX (vbox),
		      align,
		      TRUE, TRUE, 0);
  widget = gtk_statusbar_new ();
  align = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (widget), TRUE);
  gtk_statusbar_push (GTK_STATUSBAR (widget), 0, "Hold on...");

  gtk_box_pack_end (GTK_BOX (vbox), align, FALSE, FALSE, 0);

  info = new_widget_info ("statusbar", vbox, SMALL);
  gtk_container_set_border_width (GTK_CONTAINER (info->window), 0);

  return info;
}

static WidgetInfo *
create_scales (void)
{
  GtkWidget *hbox;
  GtkWidget *vbox;

  vbox = gtk_vbox_new (FALSE, 3);
  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),
		      gtk_hscale_new_with_range (0.0, 100.0, 1.0),
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),
		      gtk_vscale_new_with_range (0.0, 100.0, 1.0),
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      g_object_new (GTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nScales",
				    "justify", GTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("scales", vbox, MEDIUM);}

GList *
get_all_widgets (void)
{
  GList *retval = NULL;

  retval = g_list_append (retval, create_button ());
  retval = g_list_append (retval, create_chain_button ());
  retval = g_list_append (retval, create_color_area ());
  retval = g_list_append (retval, create_color_button ());
  retval = g_list_append (retval, create_color_hex_entry ());
  retval = g_list_append (retval, create_color_scale ());
  retval = g_list_append (retval, create_color_selection ());
  retval = g_list_append (retval, create_file_entry ());

  return retval;
}
