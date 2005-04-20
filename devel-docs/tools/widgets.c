
#include "config.h"

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"


#include "widgets.h"


#define SMALL_WIDTH   240
#define SMALL_HEIGHT   75
#define MEDIUM_WIDTH  240
#define MEDIUM_HEIGHT 165
#define LARGE_WIDTH   240
#define LARGE_HEIGHT  240


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
  guint target_width = 0;
  guint target_height = 0;

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
realize_callback (WidgetInfo *info)
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
      g_signal_connect_swapped (info->window, "realize",
                                G_CALLBACK (realize_callback), info);
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

static void
color_init (GimpRGB *rgb)
{
  gimp_rgb_parse_name (rgb, "goldenrod", -1);
  gimp_rgb_set_alpha (rgb, 0.7);
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
create_color_area (void)
{
  GtkWidget *vbox;
  GtkWidget *area;
  GtkWidget *align;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
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

  color_init (&color);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
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

  color_init (&color);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  entry = gimp_color_hex_entry_new ();
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

  color_init (&rgb);
  gimp_rgb_to_hsv (&rgb, &hsv);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.8, 0.0);
  scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                GIMP_COLOR_SELECTOR_HUE);
  gimp_color_scale_set_color (GIMP_COLOR_SCALE (scale), &rgb, &hsv);
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

  color_init (&color);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  selection = gimp_color_selection_new ();
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
create_frame (void)
{
  GtkWidget *frame;
  GtkWidget *content;

  frame = gimp_frame_new ("Frame");
  content = gtk_label_new ("Frame Content\nThis Frame is HIG compliant");
  gtk_misc_set_alignment (GTK_MISC (content), 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), content);

  return new_widget_info ("gimp-frame", frame, MEDIUM);
}

static WidgetInfo *
create_path_editor (void)
{
  GtkWidget *vbox;
  GtkWidget *editor;
  GtkWidget *align;
  gchar     *config = gimp_config_build_data_path ("patterns");
  gchar     *path   = gimp_config_path_expand (config, TRUE, NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  editor = gimp_path_editor_new ("Path Editor", path);
  gtk_widget_set_size_request (editor, -1, 240);
  gtk_container_add (GTK_CONTAINER (align), editor);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Path Editor"),
                      FALSE, FALSE, 0);

  g_free (path);
  g_free (config);

  return new_widget_info ("gimp-path-editor", vbox, ASIS);
}

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
  retval = g_list_append (retval, create_frame ());
  retval = g_list_append (retval, create_path_editor ());

  return retval;
}
