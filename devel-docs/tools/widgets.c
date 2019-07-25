
#include "config.h"

#include <gegl.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#undef GIMP_DISABLE_DEPRECATED

#include "libgimpbase/gimpbase.h"
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


static WidgetInfo *
new_widget_info (const char *name,
                 GtkWidget  *widget,
                 gboolean    show_all,
                 WidgetSize  size)
{
  WidgetInfo *info;

  info = g_new0 (WidgetInfo, 1);

  info->name     = g_strdup (name);
  info->show_all = show_all;
  info->size     = size;
  info->no_focus = TRUE;

  if (GTK_IS_WINDOW (widget))
    {
      info->widget = widget;

      gtk_window_set_resizable (GTK_WINDOW (info->widget), FALSE);
    }
  else
    {
      info->widget = gtk_offscreen_window_new ();

      gtk_container_set_border_width (GTK_CONTAINER (info->widget), 12);
      gtk_container_add (GTK_CONTAINER (info->widget), widget);
    }

  if (info->show_all)
    gtk_widget_show_all (widget);
  else
    gtk_widget_show (widget);

  g_signal_connect (info->widget, "focus", G_CALLBACK (gtk_true), NULL);

  switch (size)
    {
    case SMALL:
      gtk_widget_set_size_request (info->widget, SMALL_WIDTH, SMALL_HEIGHT);
      break;
    case MEDIUM:
      gtk_widget_set_size_request (info->widget, MEDIUM_WIDTH, MEDIUM_HEIGHT);
      break;
    case LARGE:
      gtk_widget_set_size_request (info->widget, LARGE_WIDTH, LARGE_HEIGHT);
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

static GdkPixbuf *
load_image (const gchar *name)
{
  GdkPixbuf *pixbuf;
  gchar     *filename;

  filename = g_build_filename (TOP_SRCDIR, "data", "images", name, NULL);

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

  g_free (filename);

  return pixbuf;
}

static WidgetInfo *
create_browser (void)
{
  GtkWidget *vbox;
  GtkWidget *browser;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  browser = gimp_browser_new ();
  gtk_widget_set_size_request (browser, 500, 200);
  gimp_browser_add_search_types (GIMP_BROWSER (browser),
                                 "by name", 1,
                                 NULL);
  gimp_browser_show_message (GIMP_BROWSER (browser), "Result goes here");
  gtk_box_pack_start (GTK_BOX (gimp_browser_get_left_vbox (GIMP_BROWSER (browser))),
                      gtk_label_new ("TreeView goes here"), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), browser, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Browser"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-browser", vbox, TRUE, MEDIUM);
}

static WidgetInfo *
create_busy_box (void)
{
  GtkWidget *widget;

  widget = gimp_busy_box_new ("Busy Box");
  gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);

  return new_widget_info ("gimp-widget-busy-box", widget, TRUE, SMALL);
}

static WidgetInfo *
create_button (void)
{
  GtkWidget *widget;

  widget = gimp_button_new ();
  gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);

  gtk_container_add (GTK_CONTAINER (widget),
                     gtk_label_new_with_mnemonic ("_Button"));

  return new_widget_info ("gimp-widget-button", widget, TRUE, SMALL);
}

static WidgetInfo *
create_chain_button (void)
{
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *chain;
  GtkWidget *separator;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  grid = gtk_grid_new ();
  gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (grid, GTK_ALIGN_CENTER);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, TRUE, TRUE, 0);

  chain = gimp_chain_button_new (GIMP_CHAIN_LEFT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_grid_attach (GTK_GRID (grid), chain, 0, 0, 1, 2);

  label = gtk_label_new ("Linked\nItems");
  gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);

  label = gtk_label_new ("Linked\nItems");
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);

  separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
  gtk_grid_attach (GTK_GRID (grid), separator, 2, 0, 1, 2);

  label = gtk_label_new ("Unlinked\nItems");
  gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);

  label = gtk_label_new ("Unlinked\nItems");
  gtk_grid_attach (GTK_GRID (grid), label, 3, 1, 1, 1);

  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), FALSE);
  gtk_grid_attach (GTK_GRID (grid), chain, 4, 0, 1, 2);

  gtk_box_pack_end (GTK_BOX (vbox), gtk_label_new ("Chain Button"),
                    FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-chain-button", vbox, TRUE, MEDIUM);
}

static WidgetInfo *
create_color_area (void)
{
  GtkWidget *vbox;
  GtkWidget *area;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  area = gimp_color_area_new (&color, GIMP_COLOR_AREA_SMALL_CHECKS, 0);
  gtk_widget_set_halign (area, GTK_ALIGN_FILL);
  gtk_widget_set_valign (area, GTK_ALIGN_CENTER);
  gimp_color_area_set_draw_border (GIMP_COLOR_AREA (area), TRUE);
  gtk_widget_set_size_request (area, -1, 25);
  gtk_box_pack_start (GTK_BOX (vbox), area, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Area"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-area", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_color_button (void)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  button =  gimp_color_button_new ("Color Button",
                                   80, 20, &color,
                                   GIMP_COLOR_AREA_SMALL_CHECKS);
  gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  gtk_container_add (GTK_CONTAINER (vbox), button);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Button"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-button", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_color_hex_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  entry = gimp_color_hex_entry_new ();
  gtk_widget_set_halign (entry, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (entry, GTK_ALIGN_CENTER);
  gimp_color_hex_entry_set_color (GIMP_COLOR_HEX_ENTRY (entry), &color);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Hex Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-hex-entry", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_color_notebook (void)
{
  GtkWidget *vbox;
  GtkWidget *notebook;
  GtkWidget *label;
  GimpRGB    rgb;
  GimpHSV    hsv;

  color_init (&rgb);
  gimp_rgb_to_hsv (&rgb, &hsv);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  notebook = gimp_color_selector_new (GIMP_TYPE_COLOR_NOTEBOOK,
                                      &rgb, &hsv,
                                      GIMP_COLOR_SELECTOR_HUE);
  gtk_widget_set_halign (notebook, GTK_ALIGN_FILL);
  gtk_widget_set_valign (notebook, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  label = gtk_label_new ("Color Scales");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return new_widget_info ("gimp-widget-color-notebook", vbox, FALSE, SMALL);
}

static WidgetInfo *
create_color_profile_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  combo = gimp_color_profile_combo_box_new (gtk_dialog_new (), NULL);
  gtk_widget_set_halign (combo, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (combo, GTK_ALIGN_CENTER);
  gimp_color_profile_combo_box_add_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                         NULL, "sRGB");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Profile Combo Box"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-profile-combo-box", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_color_profile_view (void)
{
  GtkWidget *vbox;
  GtkWidget *view;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  view = gimp_color_profile_view_new ();
  gtk_widget_set_halign (view, GTK_ALIGN_FILL);
  gtk_widget_set_valign (view, GTK_ALIGN_CENTER);
  gimp_color_profile_view_set_profile (GIMP_COLOR_PROFILE_VIEW (view),
                                       gimp_color_profile_new_rgb_srgb ());
  gtk_box_pack_start (GTK_BOX (vbox), view, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Profile View"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-profile-view", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_color_scale (void)
{
  GtkWidget *vbox;
  GtkWidget *scale;
  GimpRGB    rgb;
  GimpHSV    hsv;

  color_init (&rgb);
  gimp_rgb_to_hsv (&rgb, &hsv);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                GIMP_COLOR_SELECTOR_HUE);
  gtk_widget_set_halign (scale, GTK_ALIGN_FILL);
  gtk_widget_set_valign (scale, GTK_ALIGN_CENTER);
  gimp_color_scale_set_color (GIMP_COLOR_SCALE (scale), &rgb, &hsv);
  gtk_range_set_value (GTK_RANGE (scale), 40);
  gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Scale"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-scale", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_color_scales (void)
{
  GtkWidget *vbox;
  GtkWidget *scales;
  GtkWidget *label;
  GimpRGB    rgb;
  GimpHSV    hsv;

  color_init (&rgb);
  gimp_rgb_to_hsv (&rgb, &hsv);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  scales = gimp_color_selector_new (GIMP_TYPE_COLOR_SCALES,
                                    &rgb, &hsv,
                                    GIMP_COLOR_SELECTOR_HUE);
  gtk_widget_set_halign (scales, GTK_ALIGN_FILL);
  gtk_widget_set_valign (scales, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), scales, TRUE, TRUE, 0);
  gtk_widget_show (scales);

  label = gtk_label_new ("Color Scales");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return new_widget_info ("gimp-widget-color-scales", vbox, FALSE, SMALL);
}

static WidgetInfo *
create_color_select (void)
{
  GtkWidget *vbox;
  GtkWidget *select;
  GtkWidget *label;
  GimpRGB    rgb;
  GimpHSV    hsv;

  color_init (&rgb);
  gimp_rgb_to_hsv (&rgb, &hsv);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  select = gimp_color_selector_new (GIMP_TYPE_COLOR_SELECT,
                                    &rgb, &hsv,
                                    GIMP_COLOR_SELECTOR_HUE);
  gtk_widget_set_halign (select, GTK_ALIGN_FILL);
  gtk_widget_set_valign (select, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), select, TRUE, TRUE, 0);
  gtk_widget_show (select);

  label = gtk_label_new ("Color Select");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return new_widget_info ("gimp-widget-color-select", vbox, FALSE, SMALL);
}

static WidgetInfo *
create_color_selection (void)
{
  GtkWidget *vbox;
  GtkWidget *selection;
  GtkWidget *label;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  selection = gimp_color_selection_new ();
  gtk_widget_set_halign (selection, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (selection, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (selection, 400, -1);
  gimp_color_selection_set_show_alpha (GIMP_COLOR_SELECTION (selection), TRUE);
  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (selection), &color);
  gtk_box_pack_start (GTK_BOX (vbox), selection, TRUE, TRUE, 0);
  gtk_widget_show (selection);

  label = gtk_label_new ("Color Selection");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return new_widget_info ("gimp-widget-color-selection", vbox, FALSE, ASIS);
}

static WidgetInfo *
create_dialog (void)
{
  WidgetInfo *info;
  GtkWidget  *widget;
  GtkWidget  *content;
  GtkWidget  *label;

  widget = gimp_dialog_new ("Gimp Dialog",
                            "gimp-widget-dialog",
                            NULL, 0, NULL, NULL,
                            "_Cancel", GTK_RESPONSE_CANCEL,
                            "_OK",     GTK_RESPONSE_OK,

                            NULL);

  label = gtk_label_new ("Gimp Dialog");
  content = gtk_dialog_get_content_area (GTK_DIALOG (widget));
  gtk_container_add (GTK_CONTAINER (content), label);
  gtk_widget_show (label);
  info = new_widget_info ("gimp-widget-dialog", widget, TRUE, MEDIUM);

  return info;
}

static WidgetInfo *
create_enum_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  combo = gimp_enum_combo_box_new (GIMP_TYPE_CHANNEL_TYPE);
  gtk_widget_set_halign (combo, GTK_ALIGN_FILL);
  gtk_widget_set_valign (combo, GTK_ALIGN_CENTER);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), GIMP_CHANNEL_BLUE);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Enum Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-enum-combo-box", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_enum_label (void)
{
  GtkWidget *vbox;
  GtkWidget *label;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  label = gimp_enum_label_new (GIMP_TYPE_IMAGE_BASE_TYPE, GIMP_RGB);
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Enum Label"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-enum-label", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_file_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  entry = gimp_file_entry_new ("File Entry",
                               "wilber.png",
                               FALSE, TRUE);
  gtk_widget_set_halign (entry, GTK_ALIGN_FILL);
  gtk_widget_set_valign (entry, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("File Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-file-entry", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_frame (void)
{
  GtkWidget *frame;
  GtkWidget *content;

  frame = gimp_frame_new ("Frame");
  content = gtk_label_new ("Frame Content\nThis Frame is HIG compliant");
  gtk_label_set_xalign (GTK_LABEL (content), 0.0);
  gtk_label_set_yalign (GTK_LABEL (content), 0.0);
  gtk_container_add (GTK_CONTAINER (frame), content);

  return new_widget_info ("gimp-widget-frame", frame, TRUE, MEDIUM);
}

static WidgetInfo *
create_hint_box (void)
{
  GtkWidget *box = gimp_hint_box_new ("This is a user hint.");

  return new_widget_info ("gimp-widget-hint-box", box, TRUE, MEDIUM);
}

static WidgetInfo *
create_number_pair_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  entry =  gimp_number_pair_entry_new (":/", TRUE, 0.001, GIMP_MAX_IMAGE_SIZE);
  gtk_widget_set_halign (entry, GTK_ALIGN_FILL);
  gtk_widget_set_valign (entry, GTK_ALIGN_CENTER);
  gimp_number_pair_entry_set_values (GIMP_NUMBER_PAIR_ENTRY (entry), 4, 3);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Number Pair Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-number-pair-entry", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_int_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  combo = gimp_int_combo_box_new ("Sobel",        1,
                                  "Prewitt",      2,
                                  "Gradient",     3,
                                  "Roberts",      4,
                                  "Differential", 5,
                                  "Laplace",      6,
                                  NULL);
  gtk_widget_set_halign (combo, GTK_ALIGN_FILL);
  gtk_widget_set_valign (combo, GTK_ALIGN_CENTER);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 1);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Int Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-int-combo-box", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_memsize_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  entry = gimp_memsize_entry_new ((3 * 1024 + 512) * 1024,
                                  0, 1024 * 1024 * 1024);
  gtk_widget_set_halign (entry, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (entry, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Memsize Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-memsize-entry", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_offset_area (void)
{
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *area;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  frame = gtk_frame_new (NULL);
  gtk_widget_set_halign (frame, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (frame, GTK_ALIGN_CENTER);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  area = gimp_offset_area_new (100, 100);
  gimp_offset_area_set_size (GIMP_OFFSET_AREA (area), 180, 160);
  gimp_offset_area_set_offsets (GIMP_OFFSET_AREA (area), 30, 30);
  gtk_container_add (GTK_CONTAINER (frame), area);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Offset Area"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-offset-area", vbox, TRUE, LARGE);
}

static WidgetInfo *
create_page_selector (void)
{
  GtkWidget *vbox;
  GtkWidget *selector;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  selector = gimp_page_selector_new ();
  gtk_widget_set_size_request (selector, -1, 240);
  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector), 16);
  gimp_page_selector_select_range (GIMP_PAGE_SELECTOR (selector),
                                   "1,3,7-9,12-15");
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Page Selector"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-page-selector", vbox, TRUE, ASIS);
}

static WidgetInfo *
create_path_editor (void)
{
  GtkWidget *vbox;
  GtkWidget *editor;
  gchar     *config = gimp_config_build_data_path ("patterns");
  gchar     *path   = gimp_config_path_expand (config, TRUE, NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  editor = gimp_path_editor_new ("Path Editor", path);
  gtk_widget_set_halign (editor, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (editor, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (editor, -1, 240);
  gtk_box_pack_start (GTK_BOX (vbox), editor, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Path Editor"), FALSE, FALSE, 0);

  g_free (path);
  g_free (config);

  return new_widget_info ("gimp-widget-path-editor", vbox, TRUE, ASIS);
}

static WidgetInfo *
create_pick_button (void)
{
  GtkWidget *vbox;
  GtkWidget *button;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  button =  gimp_pick_button_new ();
  gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Pick Button"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-pick-button", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_preview_area (void)
{
  GtkWidget     *vbox;
  GtkWidget     *area;
  GdkPixbuf     *pixbuf;
  GtkAllocation  allocation;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  pixbuf = load_image ("wilber-wizard.png");

  allocation.x      = 0;
  allocation.y      = 0;
  allocation.width  = gdk_pixbuf_get_width (pixbuf);
  allocation.height = gdk_pixbuf_get_height (pixbuf);

  area = gimp_preview_area_new ();
  gtk_widget_set_halign (area, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (area, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (area, allocation.width, allocation.height);
  gtk_widget_size_allocate (area, &allocation);
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (area), 0, 0,
                          gdk_pixbuf_get_width (pixbuf),
                          gdk_pixbuf_get_height (pixbuf),
                          GIMP_RGBA_IMAGE,
                          gdk_pixbuf_get_pixels (pixbuf),
                          gdk_pixbuf_get_rowstride (pixbuf));
  gtk_box_pack_start (GTK_BOX (vbox), area, TRUE, TRUE, 0);

  g_object_unref (pixbuf);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Preview Area"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-preview-area", vbox, TRUE, MEDIUM);
}

static WidgetInfo *
create_ruler (void)
{
  GtkWidget *grid;
  GtkWidget *ruler;

  grid = gtk_grid_new ();
  gtk_widget_set_size_request (grid, 200, 200);

  ruler = gimp_ruler_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_hexpand (ruler, TRUE);
  gimp_ruler_set_range (GIMP_RULER (ruler), 0, 100, 100);
  gimp_ruler_set_position (GIMP_RULER (ruler), 25);
  gtk_grid_attach (GTK_GRID (grid), ruler, 1, 0, 1, 1);

  ruler = gimp_ruler_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_vexpand (ruler, TRUE);
  gimp_ruler_set_range (GIMP_RULER (ruler), 0, 100, 100);
  gimp_ruler_set_position (GIMP_RULER (ruler), 75);
  gtk_grid_attach (GTK_GRID (grid), ruler, 0, 1, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Ruler"), 1, 1, 1, 1);

  return new_widget_info ("gimp-widget-ruler", grid, TRUE, MEDIUM);
}

static WidgetInfo *
create_string_combo_box (void)
{
  GtkWidget    *vbox;
  GtkWidget    *combo;
  GtkListStore *store;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  store = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_list_store_insert_with_values (store, NULL, 0, 0, "Foo", -1);
  gtk_list_store_insert_with_values (store, NULL, 1, 0, "Bar", -1);
  combo = gimp_string_combo_box_new (GTK_TREE_MODEL (store), 0, 0);
  gtk_widget_set_halign (combo, GTK_ALIGN_FILL);
  gtk_widget_set_valign (combo, GTK_ALIGN_CENTER);
  g_object_unref (store);
  gimp_string_combo_box_set_active (GIMP_STRING_COMBO_BOX (combo), "Foo");
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("String Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-string-combo-box", vbox, TRUE, SMALL);
}

static WidgetInfo *
create_unit_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  combo = gimp_unit_combo_box_new ();
  gtk_widget_set_halign (combo, GTK_ALIGN_FILL);
  gtk_widget_set_valign (combo, GTK_ALIGN_CENTER);
  gimp_unit_combo_box_set_active (GIMP_UNIT_COMBO_BOX (combo), GIMP_UNIT_INCH);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Unit Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-unit-combo-box", vbox, TRUE, SMALL);
}

GList *
get_all_widgets (void)
{
  GList *retval = NULL;

  retval = g_list_append (retval, create_browser ());
  retval = g_list_append (retval, create_busy_box ());
  retval = g_list_append (retval, create_button ());
  retval = g_list_append (retval, create_chain_button ());
  retval = g_list_append (retval, create_color_area ());
  retval = g_list_append (retval, create_color_button ());
  retval = g_list_append (retval, create_color_hex_entry ());
  /* put selection before notebook, selection ensures the modules */
  retval = g_list_append (retval, create_color_selection ());
  retval = g_list_append (retval, create_color_notebook ());
  retval = g_list_append (retval, create_color_profile_combo_box ());
  retval = g_list_append (retval, create_color_profile_view ());
  retval = g_list_append (retval, create_color_scale ());
  retval = g_list_append (retval, create_color_scales ());
  retval = g_list_append (retval, create_color_select ());
  retval = g_list_append (retval, create_dialog ());
  retval = g_list_append (retval, create_enum_combo_box ());
  retval = g_list_append (retval, create_enum_label ());
  retval = g_list_append (retval, create_file_entry ());
  retval = g_list_append (retval, create_frame ());
  retval = g_list_append (retval, create_hint_box ());
  retval = g_list_append (retval, create_int_combo_box ());
  retval = g_list_append (retval, create_memsize_entry ());
  retval = g_list_append (retval, create_number_pair_entry ());
  retval = g_list_append (retval, create_offset_area ());
  retval = g_list_append (retval, create_page_selector ());
  retval = g_list_append (retval, create_path_editor ());
  retval = g_list_append (retval, create_pick_button ());
  retval = g_list_append (retval, create_preview_area ());
  retval = g_list_append (retval, create_ruler ());
  retval = g_list_append (retval, create_string_combo_box ());
  retval = g_list_append (retval, create_unit_combo_box ());

  return retval;
}
