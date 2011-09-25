
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


#if 0
static gboolean
adjust_size_callback (WidgetInfo *info)
{
  Window toplevel;
  Window root;
  gint   tx;
  gint   ty;
  guint  twidth;
  guint  theight;
  guint  tborder_width;
  guint  tdepth;
  guint  target_width = 0;
  guint  target_height = 0;

  toplevel = GDK_WINDOW_XWINDOW (gtk_widget_get_window (info->window));
  XGetGeometry (GDK_WINDOW_XDISPLAY (gtk_widget_get_window (info->window)),
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
#endif

static WidgetInfo *
new_widget_info (const char *name,
                 GtkWidget  *widget,
                 WidgetSize  size)
{
  WidgetInfo *info;

  info = g_new0 (WidgetInfo, 1);

  info->name     = g_strdup (name);
  info->size     = size;
  info->no_focus = TRUE;

  if (GTK_IS_WINDOW (widget))
    {
      info->window = widget;

      gtk_window_set_resizable (GTK_WINDOW (info->window), FALSE);
#if 0
      g_signal_connect_swapped (info->window, "realize",
                                G_CALLBACK (realize_callback), info);
#endif
    }
  else
    {
      info->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_window_set_accept_focus (GTK_WINDOW (info->window), FALSE);

      gtk_container_set_border_width (GTK_CONTAINER (info->window), 12);
      gtk_container_add (GTK_CONTAINER (info->window), widget);
      gtk_widget_show_all (widget);
   }

  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (info->window), TRUE);

  gtk_widget_set_app_paintable (info->window, TRUE);
  g_signal_connect (info->window, "focus", G_CALLBACK (gtk_true), NULL);

  switch (size)
    {
    case SMALL:
      gtk_widget_set_size_request (info->window, SMALL_WIDTH, SMALL_HEIGHT);
      break;
    case MEDIUM:
      gtk_widget_set_size_request (info->window, MEDIUM_WIDTH, MEDIUM_HEIGHT);
      break;
    case LARGE:
      gtk_widget_set_size_request (info->window, LARGE_WIDTH, LARGE_HEIGHT);
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
  GtkWidget *align;
  GtkWidget *browser;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  browser = gimp_browser_new ();
  gtk_box_pack_start (GTK_BOX (gimp_browser_get_left_vbox (GIMP_BROWSER (browser))),
                      gtk_label_new ("TreeView goes here"), TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (align), browser);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Browser"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-browser", vbox, MEDIUM);
}

static WidgetInfo *
create_button (void)
{
  GtkWidget *widget;
  GtkWidget *align;

  widget = gimp_button_new ();
  gtk_container_add (GTK_CONTAINER (widget),
                     gtk_label_new_with_mnemonic ("_Button"));
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);

  return new_widget_info ("gimp-widget-button", align, SMALL);
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.8);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  table = gtk_table_new (2, 5, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
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
  separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
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

  gtk_box_pack_end (GTK_BOX (vbox), gtk_label_new ("Chain Button"),
                    TRUE, TRUE, 0);

  return new_widget_info ("gimp-widget-chain-button", vbox, MEDIUM);
}

static WidgetInfo *
create_color_area (void)
{
  GtkWidget *vbox;
  GtkWidget *area;
  GtkWidget *align;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
  area = gimp_color_area_new (&color, GIMP_COLOR_AREA_SMALL_CHECKS, 0);
  gimp_color_area_set_draw_border (GIMP_COLOR_AREA (area), TRUE);
  gtk_widget_set_size_request (area, -1, 25);
  gtk_container_add (GTK_CONTAINER (align), area);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Area"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-area", vbox, SMALL);
}

static WidgetInfo *
create_color_button (void)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *align;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
  button =  gimp_color_button_new ("Color Button",
                                   80, 20, &color,
                                   GIMP_COLOR_AREA_SMALL_CHECKS);
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Button"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-button", vbox, SMALL);
}

static WidgetInfo *
create_color_hex_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *align;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  entry = gimp_color_hex_entry_new ();
  gimp_color_hex_entry_set_color (GIMP_COLOR_HEX_ENTRY (entry), &color);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Hex Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-hex-entry", vbox, SMALL);
}

static WidgetInfo *
create_color_profile_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  combo = gimp_color_profile_combo_box_new (gtk_dialog_new (), NULL);
  gimp_color_profile_combo_box_add_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                         NULL, "sRGB");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  gtk_container_add (GTK_CONTAINER (align), combo);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Profile Combo Box"),
                      FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-profile-combo-box", vbox, SMALL);
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.8, 0.0);
  scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                GIMP_COLOR_SELECTOR_HUE);
  gimp_color_scale_set_color (GIMP_COLOR_SCALE (scale), &rgb, &hsv);
  gtk_range_set_value (GTK_RANGE (scale), 40);
  gtk_container_add (GTK_CONTAINER (align), scale);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Scale"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-scale", vbox, SMALL);
}

static WidgetInfo *
create_color_selection (void)
{
  GtkWidget *vbox;
  GtkWidget *selection;
  GtkWidget *align;
  GimpRGB    color;

  color_init (&color);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  selection = gimp_color_selection_new ();
  gimp_color_selection_set_show_alpha(GIMP_COLOR_SELECTION (selection), TRUE);
  gimp_color_selection_set_color  (GIMP_COLOR_SELECTION (selection), &color);
  gtk_widget_set_size_request (selection, 400, -1);
  gtk_container_add (GTK_CONTAINER (align), selection);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Color Selection"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-color-selection", vbox, ASIS);
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
  info = new_widget_info ("gimp-widget-dialog", widget, MEDIUM);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_enum_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  combo = gimp_enum_combo_box_new (GIMP_TYPE_CHANNEL_TYPE);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), GIMP_BLUE_CHANNEL);
  gtk_container_add (GTK_CONTAINER (align), combo);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Enum Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-enum-combo-box", vbox, SMALL);
}

static WidgetInfo *
create_enum_label (void)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  label = gimp_enum_label_new (GIMP_TYPE_IMAGE_BASE_TYPE, GIMP_RGB);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Enum Label"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-enum-label", vbox, SMALL);
}

static WidgetInfo *
create_file_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  entry = gimp_file_entry_new ("File Entry",
                               "wilber.png",
                               FALSE, TRUE);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("File Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-file-entry", vbox, SMALL);
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

  return new_widget_info ("gimp-widget-frame", frame, MEDIUM);
}

static WidgetInfo *
create_hint_box (void)
{
  GtkWidget *box = gimp_hint_box_new ("This is a user hint.");

  return new_widget_info ("gimp-widget-hint-box", box, MEDIUM);
}

static WidgetInfo *
create_number_pair_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  entry =  gimp_number_pair_entry_new (":/", TRUE, 0.001, GIMP_MAX_IMAGE_SIZE);
  gimp_number_pair_entry_set_values (GIMP_NUMBER_PAIR_ENTRY (entry), 4, 3);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Number Pair Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-number-pair-entry", vbox, SMALL);
}

static WidgetInfo *
create_int_combo_box (void)
{
  GtkWidget *vbox;
  GtkWidget *combo;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  combo = gimp_int_combo_box_new ("Sobel",        1,
                                  "Prewitt",      2,
                                  "Gradient",     3,
                                  "Roberts",      4,
                                  "Differential", 5,
                                  "Laplace",      6,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 1);

  gtk_container_add (GTK_CONTAINER (align), combo);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Int Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-int-combo-box", vbox, SMALL);
}

static WidgetInfo *
create_memsize_entry (void)
{
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  entry = gimp_memsize_entry_new ((3 * 1024 + 512) * 1024,
                                  0, 1024 * 1024 * 1024);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Memsize Entry"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-memsize-entry", vbox, SMALL);
}

static WidgetInfo *
create_offset_area (void)
{
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *area;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (align), frame);
  area = gimp_offset_area_new (100, 100);
  gimp_offset_area_set_size (GIMP_OFFSET_AREA (area), 180, 160);
  gimp_offset_area_set_offsets (GIMP_OFFSET_AREA (area), 30, 30);
  gtk_container_add (GTK_CONTAINER (frame), area);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Offset Area"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-offset-area", vbox, LARGE);
}

static WidgetInfo *
create_page_selector (void)
{
  GtkWidget *vbox;
  GtkWidget *selector;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  selector = gimp_page_selector_new ();
  gtk_widget_set_size_request (selector, -1, 240);
  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector), 16);
  gimp_page_selector_select_range (GIMP_PAGE_SELECTOR (selector),
                                   "1,3,7-9,12-15");
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Page Selector"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-page-selector", vbox, ASIS);
}

static WidgetInfo *
create_path_editor (void)
{
  GtkWidget *vbox;
  GtkWidget *editor;
  GtkWidget *align;
  gchar     *config = gimp_config_build_data_path ("patterns");
  gchar     *path   = gimp_config_path_expand (config, TRUE, NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  editor = gimp_path_editor_new ("Path Editor", path);
  gtk_widget_set_size_request (editor, -1, 240);
  gtk_container_add (GTK_CONTAINER (align), editor);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Path Editor"), FALSE, FALSE, 0);

  g_free (path);
  g_free (config);

  return new_widget_info ("gimp-widget-path-editor", vbox, ASIS);
}

static WidgetInfo *
create_pick_button (void)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *align;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 1.0);
  button =  gimp_pick_button_new ();
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Pick Button"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-pick-button", vbox, SMALL);
}

static gboolean
area_realize (GimpPreviewArea *area)
{
  GdkPixbuf *pixbuf;

  pixbuf = load_image ("wilber-wizard.png");
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (area), 0, 0,
                          gdk_pixbuf_get_width (pixbuf),
                          gdk_pixbuf_get_height (pixbuf),
                          GIMP_RGBA_IMAGE,
                          gdk_pixbuf_get_pixels (pixbuf),
                          gdk_pixbuf_get_rowstride (pixbuf));
  g_object_unref (pixbuf);

  return FALSE;
}

static WidgetInfo *
create_preview_area (void)
{
  GtkWidget *vbox;
  GtkWidget *area;
  GtkWidget *align;
  GdkPixbuf *pixbuf;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  area =  gimp_preview_area_new ();
  g_signal_connect (area, "realize",
                    G_CALLBACK (area_realize), NULL);
  gtk_container_add (GTK_CONTAINER (align), area);
  pixbuf = load_image ("wilber-wizard.png");
  gtk_widget_set_size_request (area,
                               gdk_pixbuf_get_width (pixbuf),
                               gdk_pixbuf_get_height (pixbuf));
  g_object_unref (pixbuf);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("Preview Area"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-preview-area", vbox, MEDIUM);
}

static WidgetInfo *
create_string_combo_box (void)
{
  GtkWidget    *vbox;
  GtkWidget    *combo;
  GtkWidget    *align;
  GtkListStore *store;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.0);
  store = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_list_store_insert_with_values (store, NULL, 0, 0, "Foo", -1);
  gtk_list_store_insert_with_values (store, NULL, 1, 0, "Bar", -1);
  combo = gimp_string_combo_box_new (GTK_TREE_MODEL (store), 0, 0);
  g_object_unref (store);
  gimp_string_combo_box_set_active (GIMP_STRING_COMBO_BOX (combo), "Foo");

  gtk_container_add (GTK_CONTAINER (align), combo);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new ("String Combo Box"), FALSE, FALSE, 0);

  return new_widget_info ("gimp-widget-string-combo-box", vbox, SMALL);
}

GList *
get_all_widgets (void)
{
  GList *retval = NULL;

  retval = g_list_append (retval, create_browser ());
  retval = g_list_append (retval, create_button ());
  retval = g_list_append (retval, create_chain_button ());
  retval = g_list_append (retval, create_color_area ());
  retval = g_list_append (retval, create_color_button ());
  retval = g_list_append (retval, create_color_hex_entry ());
  retval = g_list_append (retval, create_color_profile_combo_box ());
  retval = g_list_append (retval, create_color_scale ());
  retval = g_list_append (retval, create_color_selection ());
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
  retval = g_list_append (retval, create_string_combo_box ());

  return retval;
}
