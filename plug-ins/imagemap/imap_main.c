/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2006 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h> /* for keyboard values */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_about.h"
#include "imap_circle.h"
#include "imap_commands.h"
#include "imap_default_dialog.h"
#include "imap_edit_area_info.h"
#include "imap_file.h"
#include "imap_grid.h"
#include "imap_icons.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_misc.h"
#include "imap_object.h"
#include "imap_polygon.h"
#include "imap_preview.h"
#include "imap_rectangle.h"
#include "imap_selection.h"
#include "imap_settings.h"
#include "imap_source.h"
#include "imap_statusbar.h"
#include "imap_string.h"

#include "libgimp/stdplugins-intl.h"


#define MAX_ZOOM_FACTOR 8
#define MIN_ZOOM_FACTOR -6
#define ZOOMED(x) (_zoom_ratio * (x))
#define GET_REAL_COORD(x) ((x) / _zoom_ratio)


GType                   imap_get_type         (void) G_GNUC_CONST;

static void             gimp_imap_finalize    (GObject              *object);

static GList          * imap_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * imap_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * imap_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static gint             dialog                (GimpImap             *imap);
static gint             zoom_in               (gpointer              data);
static gint             zoom_out              (gpointer              data);

static void             on_app_activate       (GApplication         *gapp,
                                               gpointer              user_data);
static void             window_destroy        (GtkWidget            *widget,
                                               gpointer              data);




G_DEFINE_TYPE (GimpImap, gimp_imap, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_TYPE_IMAP)
DEFINE_STD_SET_I18N


/* Global variables */
static MapInfo_t   _map_info;
static PreferencesData_t _preferences = {CSIM, TRUE, FALSE, TRUE, TRUE, FALSE,
FALSE, TRUE, DEFAULT_UNDO_LEVELS, DEFAULT_MRU_SIZE};
static MRU_t *_mru;

static GimpDrawable *_drawable;
static GdkCursorType _cursor = GDK_TOP_LEFT_ARROW;
static gboolean     _show_url = TRUE;
static gchar       *_filename = NULL;
static gchar       *_image_name;
static gint        _image_width;
static gint        _image_height;
static GtkWidget   *_dlg;
static Preview_t   *_preview;
static Selection_t *_selection;
static StatusBar_t *_statusbar;
static ObjectList_t *_shapes;
static gint         _zoom_factor = 1;
static gfloat       _zoom_ratio  = 1;
static gboolean (*_button_press_func)(GtkWidget*, GdkEventButton*, gpointer);
static gpointer _button_press_param;

static int      run_flag = 0;

static const GActionEntry ACTIONS[] =
{
  /* Sub-menu options */
  { "open", do_file_open_dialog },
  { "save", save },
  { "save-as", do_file_save_as_dialog },
  { "close", do_close },
  { "quit", do_quit },

  { "cut", do_cut },
  { "copy", do_copy },
  { "paste", do_paste },
  { "select-all", do_select_all },
  { "deselect-all", do_deselect_all },
  { "clear", do_clear },
  { "undo", do_undo },
  { "redo", do_redo },
  { "zoom-in", do_zoom_in },
  { "zoom-out", do_zoom_out },
  { "move-to-front", do_move_to_front },
  { "send-to-back", do_send_to_back },
  { "move-up", do_move_up },
  { "move-down", do_move_down },
  { "insert-point", polygon_insert_point },
  { "delete-point", polygon_delete_point },

  { "preferences", do_preferences_dialog },
  { "source", do_source_dialog },
  { "edit-area-info", do_edit_selected_shape },
  { "edit-map-info", do_settings_dialog },
  { "grid-settings", do_grid_settings_dialog },
  { "use-gimp-guides", do_use_gimp_guides_dialog },
  { "create-guides", do_create_guides_dialog },

  { "contents", imap_help },
  { "about", do_about_dialog },

  /* Toggle Options */
  { "grid", NULL, NULL, "false", toggle_grid },
  { "area-list", NULL, NULL, "true", toggle_area_list },

  /* Radio Options */
  { "zoom", set_zoom_factor, "s", "'1'", NULL },
  { "colormode", set_preview_color, "s", "'color'", NULL },
  { "shape", set_func, "s", "'arrow'", NULL },
};

static void
gimp_imap_class_init (GimpImapClass *klass)
{
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);
  GObjectClass    *object_class   = G_OBJECT_CLASS (klass);

  object_class->finalize          = gimp_imap_finalize;

  plug_in_class->query_procedures = imap_query_procedures;
  plug_in_class->create_procedure = imap_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_imap_init (GimpImap *imap)
{
}

static void
gimp_imap_finalize (GObject *object)
{
  GimpImap *imap = GIMP_IMAP (object);

  G_OBJECT_CLASS (gimp_imap_parent_class)->finalize (object);

  g_clear_object (&imap->builder);
}

static GList *
imap_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
imap_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            imap_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Image Map..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Web");

      gimp_procedure_set_documentation (procedure,
                                        _("Create a clickable imagemap"),
                                        NULL,
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Maurits Rijk",
                                      "Maurits Rijk",
                                      "1998-2005");
    }

  return procedure;
}

static GimpValueArray *
imap_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpImap *imap;

  gegl_init (NULL, NULL);

  imap            = GIMP_IMAP (gimp_procedure_get_plug_in (procedure));
#if GLIB_CHECK_VERSION(2,74,0)
  imap->app       = gtk_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
#else
  imap->app       = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
#endif
  imap->success   = FALSE;

  imap->builder = gtk_builder_new_from_resource ("/org/gimp/imagemap/imap-menu.ui");

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      _drawable = drawables[0];
    }

  _image_name   = gimp_image_get_name (image);
  _image_width  = gimp_image_get_width (image);
  _image_height = gimp_image_get_height (image);

  _map_info.color = gimp_drawable_is_rgb (_drawable);
  imap->drawable  = _drawable;

  if (run_mode != GIMP_RUN_INTERACTIVE)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             NULL);

  g_signal_connect (imap->app, "activate", G_CALLBACK (on_app_activate), imap);
  g_application_run (G_APPLICATION (imap->app), 0, NULL);
  g_clear_object (&imap->app);

  if (! imap->success)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  return gimp_procedure_new_return_values (procedure,
                                           GIMP_PDB_SUCCESS,
                                           NULL);
}

GtkWidget*
get_dialog (void)
{
  return _dlg;
}

MRU_t*
get_mru (void)
{
  if (!_mru)
    _mru = mru_create();
  return _mru;
}

MapInfo_t*
get_map_info (void)
{
  return &_map_info;
}

PreferencesData_t*
get_preferences (void)
{
  return &_preferences;
}

static void
init_preferences (void)
{
  ColorSelData_t *colors = &_preferences.colors;

  colors->normal_fg.red = 0;
  colors->normal_fg.green = 0xFFFF;
  colors->normal_fg.blue = 0;

  colors->normal_bg.red = 0;
  colors->normal_bg.green = 0;
  colors->normal_bg.blue = 0xFFFF;

  colors->selected_fg.red = 0xFFFF;
  colors->selected_fg.green = 0;
  colors->selected_fg.blue = 0;

  colors->selected_bg.red = 0;
  colors->selected_bg.green = 0;
  colors->selected_bg.blue = 0xFFFF;

  colors->interactive_fg.red = 0xFFFF;
  colors->interactive_fg.green = 0;
  colors->interactive_fg.blue = 0xFFFF;

  colors->interactive_bg.red = 0xFFFF;
  colors->interactive_bg.green = 0xFFFF;
  colors->interactive_bg.blue = 0;

  preferences_load (&_preferences);

  mru_set_size (_mru, _preferences.mru_size);
  command_list_set_undo_level (_preferences.undo_levels);
}

gint
get_image_width (void)
{
  return _image_width;
}

gint
get_image_height (void)
{
  return _image_height;
}

void
set_busy_cursor (void)
{
  preview_set_cursor(_preview, GDK_WATCH);
}

void
remove_busy_cursor (void)
{
  gdk_window_set_cursor(gtk_widget_get_window (_dlg), NULL);
}

static gint
zoom_in (gpointer data)
{
   if (_zoom_factor < MAX_ZOOM_FACTOR)
     {
       set_zoom (_zoom_factor + 1);
       menu_set_zoom (data, _zoom_factor);
     }
   return _zoom_factor;
}

static gint
zoom_out (gpointer data)
{
   if (_zoom_factor > MIN_ZOOM_FACTOR)
     {
       set_zoom (_zoom_factor - 1);
       menu_set_zoom (data, _zoom_factor);
     }
   return _zoom_factor;
}

void
set_zoom (gint zoom_factor)
{
  set_busy_cursor();
  _zoom_factor = zoom_factor;

  if (zoom_factor > 0)
    _zoom_ratio = zoom_factor;
  else
    _zoom_ratio = 1.0f / ((zoom_factor - 2.0f) * -1.0f);

  preview_zoom (_preview, _zoom_ratio);
  statusbar_set_zoom (_statusbar, zoom_factor);
  remove_busy_cursor ();
}

gint
get_real_coord(gint coord)
{
   return GET_REAL_COORD(coord);
}

void
draw_line(cairo_t *cr, gint x1, gint y1, gint x2, gint y2)
{
   cairo_move_to (cr, ZOOMED (x1) + .5, ZOOMED (y1) + .5);
   cairo_line_to (cr, ZOOMED (x2) + .5, ZOOMED (y2) + .5);
   cairo_stroke (cr);
}

void
draw_rectangle(cairo_t *cr, gboolean filled, gint x, gint y,
               gint width, gint height)
{
   cairo_rectangle (cr, ZOOMED (x) + (filled ? 0. : .5),
                        ZOOMED (y) + (filled ? 0. : .5),
                    ZOOMED (width), ZOOMED (height));
   if (filled)
      cairo_fill (cr);
   else
      cairo_stroke (cr);
}

void
draw_circle(cairo_t *cr, gint x, gint y, gint r)
{
   cairo_arc (cr, ZOOMED (x), ZOOMED (y), ZOOMED (r), 0., 2 * G_PI);
   cairo_stroke (cr);
}

void
draw_polygon(cairo_t *cr, GList *list)
{
   GList     *p;

   for (p = list; p; p = p->next) {
      GdkPoint *src = (GdkPoint*) p->data;
      cairo_line_to (cr, ZOOMED (src->x) + .5, ZOOMED (src->y) + .5);
   }
   cairo_close_path (cr);
   cairo_stroke (cr);
}

void
set_preview_color (GSimpleAction  *action,
                   GVariant       *new_state,
                   gpointer        user_data)
{
  gchar *str;

  str = g_strdup_printf ("%s", g_variant_get_string (new_state, NULL));

  if (! strcmp (str, "gray"))
    _map_info.show_gray = 1;
  else
    _map_info.show_gray = 0;

  g_free (str);
  g_simple_action_set_state (action, new_state);

  set_zoom (_zoom_factor);
}

void
preview_redraw(void)
{
  gtk_widget_queue_draw(_preview->preview);
}

void
set_zoom_factor (GSimpleAction *action,
                 GVariant      *new_state,
                 gpointer       user_data)
{
  gchar *str;
  gint   factor = 1;

  str = g_strdup_printf ("%s", g_variant_get_string (new_state, NULL));
  factor = atoi (str);
  g_free (str);

  set_zoom (factor);

  g_simple_action_set_state (action, new_state);
}

const gchar *
get_image_name(void)
{
   return _image_name;
}

const char*
get_filename(void)
{
   return _filename;
}

static gboolean
arrow_on_button_press (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      do_popup_menu (event, data);
    }
  else if (event->button == 1)
    {
      if (event->type == GDK_2BUTTON_PRESS)
        edit_shape ((gint) event->x, (gint) event->y);
      else
        select_shape (widget, event);
    }

  return FALSE;
}

static void
set_arrow_func (gpointer data)
{
   _button_press_func  = arrow_on_button_press;
   _button_press_param = data;
   _cursor             = GDK_TOP_LEFT_ARROW;
}

static void
set_object_func (gboolean (*func)(GtkWidget*, GdkEventButton*,
                                  gpointer), gpointer param)
{
   _button_press_func = func;
   _button_press_param = param;
   _cursor = GDK_CROSSHAIR;
}

void
set_func (GSimpleAction  *action,
          GVariant       *new_state,
          gpointer        user_data)
{
  gchar *str;
  gint   value = 0;

  str = g_strdup_printf ("%s", g_variant_get_string (new_state, NULL));
  if (! strcmp (str, "arrow"))
    value = 0;
  else if (! strcmp (str, "rectangle"))
    value = 1;
  else if (! strcmp (str, "circle"))
    value = 2;
  else if (! strcmp (str, "polygon"))
    value = 3;

  g_free (str);

  switch (value)
    {
    case 0:
      set_arrow_func (user_data);
      break;
    case 1:
      set_object_func (object_on_button_press, get_rectangle_factory);
      break;
    case 2:
      set_object_func (object_on_button_press, get_circle_factory);
      break;
    case 3:
      set_object_func (object_on_button_press, get_polygon_factory);
      break;
    default:
      break;
    }

  g_simple_action_set_state (action, new_state);
}

void
add_shape(Object_t *obj)
{
   object_list_append(_shapes, obj);
}

ObjectList_t*
get_shapes(void)
{
   return _shapes;
}

void
update_shape(Object_t *obj)
{
   object_list_update(_shapes, obj);
}

void
do_edit_selected_shape (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
   object_list_edit_selected(_shapes);
}

void
do_popup_menu (GdkEventButton *event,
               gpointer        data)
{
  gint x = GET_REAL_COORD((gint) event->x);
  gint y = GET_REAL_COORD((gint) event->y);
  Object_t *obj = object_list_find (_shapes, x, y);

  if (obj)
    obj->class->do_popup (obj, event, data);
  else
    do_main_popup_menu (event, data);
}

static void
set_all_sensitivities (gpointer data)
{
   gint count = object_list_nr_selected (_shapes);
   menu_shapes_selected (count, data);
}

static void
main_set_title (const char *filename)
{
   char *title, *p;

   g_strreplace(&_filename, filename);
   p = filename ? g_filename_display_basename (filename) : (gchar *) _("<Untitled>");
   title = g_strdup_printf("%s - Image Map", p);
   if (filename)
     g_free (p);
   gtk_window_set_title(GTK_WINDOW(_dlg), title);
   g_free(title);
}

void
main_set_dimension(gint width, gint height)
{
   statusbar_set_dimension(_statusbar,
                           width / _zoom_factor, height / _zoom_factor);
}

void
main_clear_dimension (void)
{
   statusbar_clear_dimension (_statusbar);
}

void
show_url (void)
{
   _show_url = TRUE;
}

void
hide_url (void)
{
   _show_url = FALSE;
   statusbar_clear_status(_statusbar);
}

void
select_shape (GtkWidget      *widget,
              GdkEventButton *event)
{
   Object_t *obj;
   gint x = GET_REAL_COORD ((gint) event->x);
   gint y = GET_REAL_COORD ((gint) event->y);
   MoveSashFunc_t sash_func;

   obj = object_list_near_sash (_shapes, x, y, &sash_func);
   if (obj) {                   /* Start resizing */
      Command_t *command = move_sash_command_new(widget, obj, x, y, sash_func);
      command_execute(command);
   } else {
      Command_t *command;

      obj = object_list_find(_shapes, x, y);
      if (obj) {
         if (event->state & GDK_SHIFT_MASK) {
            if (obj->selected)
               command = unselect_command_new(obj);
            else
               command = select_command_new(obj);
         } else {               /* No Shift key pressed */
            if (obj->selected) {
               command = unselect_all_command_new(_shapes, obj);
            } else {
               Command_t *sub_command;

               command = subcommand_start(NULL);
               sub_command = unselect_all_command_new(_shapes, NULL);
               command_add_subcommand(command, sub_command);
               sub_command = select_command_new(obj);
               command_add_subcommand(command, sub_command);
               command_set_name(command, sub_command->name);
               subcommand_end();
            }
         }
         command_execute(command);

         command = move_command_new(_preview, obj, x, y);
         command_execute(command);
      } else { /* Start selection rectangle */
         command = select_region_command_new(widget, _shapes, x, y);
         command_execute(command);
      }
   }
}

void
edit_shape(gint x, gint y)
{
   Object_t *obj;

   x = GET_REAL_COORD(x);
   y = GET_REAL_COORD(y);

   obj = object_list_find(_shapes, x, y);
   if (obj) {
      object_select(obj);
      object_edit(obj, TRUE);
   }
}

void
do_zoom_in (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
   gint factor = zoom_in (user_data);
   menu_set_zoom_sensitivity (user_data, factor);
}

void
do_zoom_out (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
   gint factor = zoom_out (user_data);
   menu_set_zoom_sensitivity (user_data, factor);
}

void
draw_shapes(cairo_t *cr)
{
   object_list_draw(_shapes, cr);
}

static void
clear_map_info (void)
{
   const gchar *author = g_get_real_name();

   if (!*author)
      author = g_get_user_name();
   g_strreplace(&_map_info.image_name, _image_name);
   g_strreplace(&_map_info.title, "map");
   g_strreplace(&_map_info.author, author);
   g_strreplace(&_map_info.default_url, "");
   g_strreplace(&_map_info.description, "");

   _map_info.map_format = _preferences.default_map_type;
   _map_info.show_gray = FALSE;
}

static void
do_data_changed_dialog (void (*continue_cb)(gpointer),
                        gpointer param)
{
  GtkWidget *dialog = gtk_message_dialog_new
    (NULL,
     GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_YES_NO,
     _("Some data has been changed!"));
  gtk_message_dialog_format_secondary_text
    (GTK_MESSAGE_DIALOG (dialog),
     _("Do you really want to discard your changes?"));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    continue_cb (param);

  gtk_widget_destroy (dialog);
}

static void
check_if_changed (void (*func)(gpointer),
                  gpointer param)
{
   if (object_list_get_changed (_shapes))
     do_data_changed_dialog (func, param);
   else
     func (param);
}

static void
close_current (gpointer data)
{
   selection_freeze (_selection);
   object_list_remove_all (_shapes);
   selection_thaw (_selection);
   clear_map_info ();
   main_set_title (NULL);
   set_all_sensitivities (data);
   preview_redraw ();
   object_list_clear_changed (_shapes);
   command_list_remove_all ();
}

static void
really_close (gpointer data)
{
  close_current (data);
}

void
do_close (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  check_if_changed (really_close, user_data);
}

static void
really_quit (gpointer data)
{
  GimpImap *imap = NULL;

  if (data)
    {
      imap = GIMP_IMAP (data);
      imap->success = TRUE;
    }

  preferences_save (&_preferences);
  run_flag = 1;

  window_destroy (_dlg, imap);
}

void
do_quit (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
  check_if_changed (really_quit, user_data);
}

void
do_undo (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
  selection_freeze (_selection);
  last_command_undo ();
  selection_thaw (_selection);
  preview_redraw ();
}

void
do_redo (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
  selection_freeze (_selection);
  last_command_redo ();
  selection_thaw (_selection);
  preview_redraw ();
}

void
save (GSimpleAction *action,
      GVariant      *parameter,
      gpointer       user_data)
{
  if (_filename)
    save_as (_filename);
  else
    do_file_save_as_dialog (NULL, NULL, NULL);
}

static void
write_cern_comment (gpointer     param,
                    OutputFunc_t output)
{
  output(param, "rect (4096,4096) (4096,4096) imap:#$");
}

static void
save_as_cern(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;
   gchar *next_token;

   write_cern_comment(param, output);
   output(param, "-:Image map file created by GIMP Image Map plug-in\n");
   write_cern_comment(param, output);
   output(param, "-:GIMP Image Map plug-in by Maurits Rijk\n");
   write_cern_comment(param, output);
   output(param, "-:Please do not edit lines starting with \"#$\"\n");
   write_cern_comment(param, output);
   output(param, "VERSION:2.3\n");
   write_cern_comment(param, output);
   output(param, "TITLE:%s\n", _map_info.title);
   write_cern_comment(param, output);
   output(param, "AUTHOR:%s\n", _map_info.author);
   write_cern_comment(param, output);
   output(param, "FORMAT:cern\n");

   description = g_strdup(_map_info.description);
   next_token = description;
   for (p = strtok (next_token, "\n"); p; p = strtok(NULL, "\n")) {
      write_cern_comment(param, output);
      output(param, "DESCRIPTION:%s\n", p);
   }
   g_free(description);

   if (*_map_info.default_url)
      output(param, "default %s\n", _map_info.default_url);
   object_list_write_cern(_shapes, param, output);
}

static void
save_as_csim(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;

   output(param, "<img src=\"%s\" width=\"%d\" height=\"%d\" border=\"0\" "
          "usemap=\"#%s\" />\n\n", _map_info.image_name,
          _image_width, _image_height, _map_info.title);
   output(param, "<map name=\"%s\">\n", _map_info.title);
   output(param,
          "<!-- #$-:Image map file created by GIMP Image Map plug-in -->\n");
   output(param, "<!-- #$-:GIMP Image Map plug-in by Maurits Rijk -->\n");
   output(param,
          "<!-- #$-:Please do not edit lines starting with \"#$\" -->\n");
   output(param, "<!-- #$VERSION:2.3 -->\n");
   output(param, "<!-- #$AUTHOR:%s -->\n", _map_info.author);

   description = g_strdup(_map_info.description);
   for (p = strtok(description, "\n"); p; p = strtok(NULL, "\n"))
      output(param, "<!-- #$DESCRIPTION:%s -->\n", p);
   g_free(description);

   object_list_write_csim(_shapes, param, output);
   if (*_map_info.default_url)
      output(param, "<area shape=\"default\" href=\"%s\" />\n",
             _map_info.default_url);
   output(param, "</map>\n");
}

static void
save_as_ncsa(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;

   output(param, "#$-:Image map file created by GIMP Image Map plug-in\n");
   output(param, "#$-:GIMP Image Map plug-in by Maurits Rijk\n");
   output(param, "#$-:Please do not edit lines starting with \"#$\"\n");
   output(param, "#$VERSION:2.3\n");
   output(param, "#$TITLE:%s\n", _map_info.title);
   output(param, "#$AUTHOR:%s\n", _map_info.author);
   output(param, "#$FORMAT:ncsa\n");

   description = g_strdup(_map_info.description);
   for (p = strtok(description, "\n"); p; p = strtok(NULL, "\n"))
      output(param, "#$DESCRIPTION:%s\n", p);
   g_free(description);

   if (*_map_info.default_url)
      output(param, "default %s\n", _map_info.default_url);
   object_list_write_ncsa(_shapes, param, output);
}

static void   save_to_file (gpointer    param,
                            const char *format,
                            ...) G_GNUC_PRINTF(2,3);


static void
save_to_file (gpointer    param,
              const char* format,
              ...)
{
  va_list ap;

  va_start (ap, format);
  vfprintf ((FILE*) param, format, ap);
  va_end(ap);
}

void
dump_output (gpointer     param,
             OutputFunc_t output)
{
   if (_map_info.map_format == NCSA)
      save_as_ncsa(param, output);
   else if (_map_info.map_format == CERN)
      save_as_cern(param, output);
   else if (_map_info.map_format == CSIM)
      save_as_csim(param, output);
}

void
save_as (const gchar *filename)
{
   FILE *out = g_fopen(filename, "w");
   if (out) {
      dump_output(out, save_to_file);
      fclose(out);

      statusbar_set_status(_statusbar, _("File \"%s\" saved."), filename);
      main_set_title(filename);
      object_list_clear_changed(_shapes);
   } else {
      do_file_error_dialog( _("Couldn't save file:"), filename);
   }
}

static void
do_image_size_changed_dialog (void)
{
   GtkWidget *dialog = gtk_message_dialog_new_with_markup
     (NULL,
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_YES_NO,
      "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
      _("Image size has changed."),
      _("Resize area's?"));

   if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
     {
       gint per_x = _image_width * 100 / _map_info.old_image_width;
       gint per_y = _image_height * 100 / _map_info.old_image_height;
       object_list_resize(_shapes, per_x, per_y);
     }

   preview_redraw();
   gtk_widget_destroy (dialog);
}

static void
really_load (gpointer data)
{
  GimpImap *imap = GIMP_IMAP (data);
  gchar    *filename = imap->tmp_filename;
  close_current (imap);

  selection_freeze(_selection);
  _map_info.old_image_width = _image_width;
  _map_info.old_image_height = _image_height;
  if (load_csim(filename))
    {
      _map_info.map_format = CSIM;
      if (_image_width != _map_info.old_image_width ||
          _image_height != _map_info.old_image_height)
        {
          do_image_size_changed_dialog();
        }
     }
   else if (load_ncsa(filename))
     {
       _map_info.map_format = NCSA;
     }
   else if (load_cern(filename))
     {
       _map_info.map_format = CERN;
     }
   else
     {
       do_file_error_dialog ( _("Couldn't read file:"), filename);
       selection_thaw(_selection);
       close_current (imap);
       return;
     }
   mru_set_first(_mru, filename);

   selection_thaw (_selection);
   main_set_title (filename);
   object_list_clear_changed (_shapes);
   preview_redraw ();
}

void
load (const gchar *filename,
      gpointer     data)
{
  GimpImap *imap = GIMP_IMAP (data);

  g_strreplace (&imap->tmp_filename, filename);
  check_if_changed (really_load, imap);
}

void
toggle_area_list (GSimpleAction  *action,
                  GVariant       *new_state,
                  gpointer        user_data)
{
   selection_toggle_visibility(_selection);
   g_simple_action_set_state (action, new_state);
}

static gboolean
close_callback (GtkWidget *widget,
                gpointer   data)
{
  do_quit (NULL, NULL, NULL);
  return TRUE;
}

static gboolean
preview_move (GtkWidget      *widget,
              GdkEventMotion *event)
{
   gint x = GET_REAL_COORD((gint) event->x);
   gint y = GET_REAL_COORD((gint) event->y);
   static Object_t *prev_obj = NULL;
   Object_t *obj = object_list_find(_shapes, x, y);

   statusbar_set_xy(_statusbar, x, y);
   if (obj != prev_obj) {
      prev_obj = obj;
      if (obj && _show_url) {
         statusbar_set_status(_statusbar, _("URL: %s"), obj->url);
      } else {
         statusbar_clear_status(_statusbar);
      }
   }
#ifdef _NOT_READY_YET_
   if (!obj) {
      if (grid_near_x(x)) {
         preview_set_cursor(_preview, GDK_SB_H_DOUBLE_ARROW);
      } else if (grid_near_y(y)) {
         preview_set_cursor(_preview, GDK_SB_V_DOUBLE_ARROW);
      } else {
         preview_set_cursor(_preview, _cursor);
      }
   }
#endif
   return FALSE;
}

static void
preview_enter(GtkWidget *widget, GdkEventCrossing *event)
{
   preview_set_cursor(_preview, _cursor);
}

static void
preview_leave(GtkWidget *widget, GdkEventCrossing *event)
{
  gdk_window_set_cursor(gtk_widget_get_window (_dlg), NULL);
  statusbar_clear_xy(_statusbar);
}

static gboolean
button_press (GtkWidget*      widget,
              GdkEventButton* event,
              gpointer        data)
{
  if (_button_press_func)
    return _button_press_func (widget, event, _button_press_param);

  return FALSE;
}

/* A few global vars for key movement */

static guint _timeout;
static guint _keyval;
static gint _dx, _dy;

static void
move_sash_selected_objects (gint     dx,
                            gint     dy,
                            gboolean fast)
{
   if (fast) {
      dx *= 5;
      dy *= 5;
   }

   object_list_move_sash_selected (_shapes, dx, dy);

   preview_redraw ();
}

static void
move_selected_objects (gint     dx,
                       gint     dy,
                       gboolean fast)
{
   if (fast) {
      dx *= 5;
      dy *= 5;
   }
   _dx += dx;
   _dy += dy;

   object_list_move_selected(_shapes, dx, dy);

   preview_redraw ();
}

static gboolean
key_timeout_cb(gpointer data)
{
   switch (_keyval) {
   case GDK_KEY_Left:
   case GDK_KEY_Right:
   case GDK_KEY_Up:
   case GDK_KEY_Down:
      command_list_add(move_selected_command_new(_shapes, _dx, _dy));
      _dx = _dy = 0;
      break;
   }
   preview_redraw();

   _timeout = 0;
   return FALSE;
}

static gboolean
key_press_cb(GtkWidget *widget, GdkEventKey *event)
{
   gboolean handled = FALSE;
   gboolean shift = event->state & GDK_SHIFT_MASK;
   gboolean ctrl = event->state & GDK_CONTROL_MASK;
   Command_t *command;

   if (_timeout)
      g_source_remove(_timeout);
   _timeout = 0;

   switch (event->keyval) {
   case GDK_KEY_Left:
      if (ctrl)
         move_sash_selected_objects(-1, 0, shift);
      else
         move_selected_objects(-1, 0, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Right:
      if (ctrl)
         move_sash_selected_objects(1, 0, shift);
      else
         move_selected_objects(1, 0, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Up:
      if (ctrl)
         move_sash_selected_objects(0, -1, shift);
      else
         move_selected_objects(0, -1, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Down:
      if (ctrl)
         move_sash_selected_objects(0, 1, shift);
      else
         move_selected_objects(0, 1, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Tab:
      if (shift)
         command = select_prev_command_new(_shapes);
      else
         command = select_next_command_new(_shapes);
      command_execute(command);
      handled = TRUE;
      break;
   }
   if (handled)
      g_signal_stop_emission_by_name(widget, "key-press-event");

   return handled;
}

static gboolean
key_release_cb (GtkWidget *widget, GdkEventKey *event)
{
   _keyval = event->keyval;
   _timeout = g_timeout_add(250, key_timeout_cb, NULL);
   return FALSE;
}

static void
geometry_changed (Object_t *obj,
                  gpointer  data)
{
   preview_redraw();
}

static void
data_changed (Object_t *obj,
              gpointer  data)
{
   preview_redraw ();
   set_all_sensitivities (data);
}

static void
data_selected (Object_t *obj,
               gpointer  data)
{
   set_all_sensitivities (data);
}

void
imap_help (GSimpleAction *action,
           GVariant      *parameter,
           gpointer       user_data)
{
  gimp_standard_help_func ("plug-in-imagemap", NULL);
}

void
do_cut (GSimpleAction *action,
        GVariant      *parameter,
        gpointer       user_data)
{
  command_execute (cut_command_new (_shapes));
}

void
do_copy (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
  command_execute (copy_command_new (_shapes));
}

void
do_paste (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  command_execute (paste_command_new (_shapes));
}

void
do_select_all (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  command_execute (select_all_command_new (_shapes));
}

void
do_deselect_all (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  command_execute (unselect_all_command_new (_shapes, NULL));
}

void
do_clear (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  command_execute (clear_command_new(_shapes));
}

void
do_move_up (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  /* Fix me!
   Command_t *command = object_up_command_new(_current_obj->list,
                                              _current_obj);
   command_execute(command);
  */
}

void
do_move_down (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  /* Fix me!
   Command_t *command = object_down_command_new(_current_obj->list,
                                                _current_obj);
   command_execute(command);
  */
}

void
do_move_to_front (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  command_execute(move_to_front_command_new(_shapes));
}

void
do_send_to_back (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  command_execute(send_to_back_command_new(_shapes));
}

void
do_use_gimp_guides_dialog (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  command_execute (gimp_guides_command_new (_shapes, _drawable));
}

void
do_create_guides_dialog (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
  command_execute (guides_command_new (_shapes));
}

static Command_t*
factory_move_up(void)
{
   return move_up_command_new(_shapes);
}

static Command_t*
factory_move_down(void)
{
   return move_down_command_new(_shapes);
}

static gint
dialog (GimpImap *imap)
{
  GtkWidget  *hbox;
  GtkWidget  *menubar;
  GMenuModel *model;
  GtkWidget  *toolbar;
  GtkWidget  *main_vbox;
  GtkWidget  *tools;

  gimp_ui_init (PLUG_IN_BINARY);

  set_arrow_func (imap);

  _shapes = make_object_list();

  _dlg = imap->dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_resizable (GTK_WINDOW (imap->dlg), TRUE);

  main_set_title (NULL);
  gimp_help_connect (imap->dlg, NULL, gimp_standard_help_func, PLUG_IN_PROC, NULL, NULL);

  gtk_window_set_position (GTK_WINDOW (imap->dlg), GTK_WIN_POS_MOUSE);

  gtk_window_set_application (GTK_WINDOW (_dlg), imap->app);
  gimp_window_set_transient (GTK_WINDOW (imap->dlg));

  g_action_map_add_action_entries (G_ACTION_MAP (imap->app),
                                   ACTIONS, G_N_ELEMENTS (ACTIONS),
                                   imap);

  g_signal_connect (imap->dlg, "delete-event",
                    G_CALLBACK (close_callback), NULL);
  g_signal_connect (imap->dlg, "key-press-event",
                    G_CALLBACK (key_press_cb), NULL);
  g_signal_connect (imap->dlg, "key-release-event",
                    G_CALLBACK (key_release_cb), NULL);

  g_signal_connect (imap->dlg, "destroy",
                    G_CALLBACK (window_destroy),
                    NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (imap->dlg), main_vbox);
  gtk_widget_show (main_vbox);

  init_icons();

  /* Create menu */
  make_menu (imap);
  model = G_MENU_MODEL (gtk_builder_get_object (imap->builder, "imap-menubar"));
  menubar = gtk_menu_bar_new_from_model (model);

  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);

  /* Create toolbar */
  toolbar = gtk_toolbar_new ();

  add_tool_button (toolbar, "app.open", GIMP_ICON_DOCUMENT_OPEN,
                   _("Open"), _("Open"));
  add_tool_button (toolbar, "app.save", GIMP_ICON_DOCUMENT_SAVE,
                   _("Save"), _("Save"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.preferences", GIMP_ICON_PREFERENCES_SYSTEM,
                   _("Preferences"), _("Preferences"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.undo", GIMP_ICON_EDIT_UNDO,
                   _("Undo"), _("Undo"));
  add_tool_button (toolbar, "app.redo", GIMP_ICON_EDIT_REDO,
                   _("Redo"), _("Redo"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.cut", GIMP_ICON_EDIT_CUT,
                   _("Cut"), _("Cut"));
  add_tool_button (toolbar, "app.copy", GIMP_ICON_EDIT_COPY,
                   _("Copy"), _("Copy"));
  add_tool_button (toolbar, "app.paste", GIMP_ICON_EDIT_PASTE,
                   _("Paste"), _("Paste"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.zoom-in", GIMP_ICON_ZOOM_IN,
                   _("Zoom In"), _("Zoom In"));
  add_tool_button (toolbar, "app.zoom-out", GIMP_ICON_ZOOM_OUT,
                   _("Zoom Out"), _("Zoom Out"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.edit-map-info", GIMP_ICON_DIALOG_INFORMATION,
                   _("Edit Map Info"), _("Edit Map Info"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.move-to-front", IMAP_TO_FRONT,
                   _("Move Area to Front"), _("Move Area to Front"));
  add_tool_button (toolbar, "app.send-to-back", IMAP_TO_BACK,
                   _("Move Area to Bottom"), _("Move Area to Bottom"));
  add_tool_separator (toolbar, FALSE);
  imap->grid_toggle = add_toggle_button (toolbar, "app.grid", GIMP_ICON_GRID,
                                         _("Grid"), _("Grid"));
  add_tool_separator (toolbar, FALSE);

  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /*  Dialog area  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* Pointer tools area */
  tools = gtk_toolbar_new ();
  add_toggle_button (tools, "app.shape::arrow", GIMP_ICON_CURSOR,
                     _("Arrow"), _("Select Existing Area"));
  add_toggle_button (tools, "app.shape::rectangle", IMAP_RECTANGLE,
                     _("Rectangle"), _("Define Rectangle area"));
  add_toggle_button (tools, "app.shape::circle", IMAP_CIRCLE,
                     _("Circle"), _("Define Circle/Oval area"));
  add_toggle_button (tools, "app.shape::polygon", IMAP_POLYGON,
                     _("Polygon"), _("Define Polygon area"));
  add_tool_separator (tools, FALSE);
  add_tool_button (tools, "app.edit-area-info", GIMP_ICON_EDIT,
                   _("Edit Area Info..."), _("Edit selected area info"));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (tools),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (tools), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (tools), 0);
  gtk_widget_show (tools);
  gtk_box_pack_start (GTK_BOX (hbox), tools, FALSE, FALSE, 0);

  _preview = make_preview (imap->drawable, imap);

  g_signal_connect(_preview->preview, "motion-notify-event",
                   G_CALLBACK (preview_move), NULL);
  g_signal_connect(_preview->preview, "enter-notify-event",
                   G_CALLBACK (preview_enter), NULL);
  g_signal_connect(_preview->preview, "leave-notify-event",
                   G_CALLBACK (preview_leave), NULL);
  g_signal_connect(_preview->preview, "button-press-event",
                   G_CALLBACK (button_press), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), _preview->window, TRUE, TRUE, 0);

  object_list_add_geometry_cb (_shapes, geometry_changed, NULL);
  object_list_add_update_cb (_shapes, data_changed, imap);
  object_list_add_add_cb (_shapes, data_changed, imap);
  object_list_add_remove_cb (_shapes, data_changed, imap);
  object_list_add_move_cb (_shapes, data_changed, imap);
  object_list_add_select_cb (_shapes, data_selected, imap);

  /* Selection */
  _selection = make_selection (_shapes, imap);
  selection_set_move_up_command (_selection, factory_move_up);
  selection_set_move_down_command (_selection, factory_move_down);
  gtk_box_pack_start (GTK_BOX (hbox), _selection->container, FALSE, FALSE, 0);

  _statusbar = make_statusbar (main_vbox, imap->dlg);
  statusbar_set_zoom (_statusbar, 1);

  gtk_widget_show (imap->dlg);

  _mru = mru_create ();
  init_preferences ();

  clear_map_info ();

  imap->success = TRUE;

  return run_flag;
}

static void
on_app_activate (GApplication *gapp,
                 gpointer      user_data)
{
  GimpImap *imap = GIMP_IMAP (user_data);

  dialog (imap);

  gtk_application_set_accels_for_action (imap->app, "app.save-as", (const char*[]) { "<shift><control>S", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.close", (const char*[]) { "<primary>w", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.quit", (const char*[]) { "<primary>q", NULL });

  gtk_application_set_accels_for_action (imap->app, "app.cut", (const char*[]) { "<primary>x", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.copy", (const char*[]) { "<primary>c", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.paste", (const char*[]) { "<primary>p", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.select-all", (const char*[]) { "<primary>A", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.deselect-all", (const char*[]) { "<shift><primary>A", NULL });

  gtk_application_set_accels_for_action (imap->app, "app.zoom-in", (const char*[]) { "plus", NULL });
  gtk_application_set_accels_for_action (imap->app, "app.zoom-out", (const char*[]) { "minus", NULL });
}

static void
window_destroy (GtkWidget *widget,
                gpointer   data)
{
  if (data)
    {
      GimpImap *imap = GIMP_IMAP (data);

      gtk_application_remove_window (imap->app, GTK_WINDOW (imap->dlg));
    }
  else
    {
      gtk_widget_destroy (_dlg);
    }
}

GtkWidget *
add_tool_button (GtkWidget  *toolbar,
                 const char *action,
                 const char *icon,
                 const char *label,
                 const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));
  tool_button = gtk_tool_button_new (tool_icon, label);
  gtk_widget_show (GTK_WIDGET (tool_button));
  gtk_tool_item_set_tooltip_text (tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_button, -1);

  return GTK_WIDGET (tool_button);
}

GtkWidget *
add_toggle_button (GtkWidget  *toolbar,
                   const char *action,
                   const char *icon,
                   const char *label,
                   const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *toggle_tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));

  toggle_tool_button = gtk_toggle_tool_button_new ();
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (toggle_tool_button),
                                   tool_icon);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (toggle_tool_button), label);
  gtk_widget_show (GTK_WIDGET (toggle_tool_button));
  gtk_tool_item_set_tooltip_text (toggle_tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (toggle_tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toggle_tool_button, -1);

  return GTK_WIDGET (toggle_tool_button);
}

void
add_tool_separator (GtkWidget *toolbar,
                    gboolean   expand)
{
  GtkToolItem *item;

  item = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, expand);
  gtk_widget_show (GTK_WIDGET (item));
}
