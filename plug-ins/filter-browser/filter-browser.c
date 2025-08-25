/*
 * GIMP plug-in for browsing available GEGL filters.
 *
 * Copyright (C) 2025 Ondřej Míchal <harrymichal@seznam.cz>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl-plugin.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <operation/gegl-operation.h>
#include <string.h>

#include "libgimp/stdplugins-intl.h"
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "libgimpbase/gimpbase.h"

#include "gegl-filter-info.h"

#define PLUG_IN_GEGL_FILTER_BROWSER "plug-in-gegl-filter-browser"
#define PLUG_IN_BINARY              "gegl-filter-browser"
#define PLUG_IN_ROLE                "gegl-filter-browser"

#define FILTER_BROWSER_WIDTH  800
#define FILTER_BROWSER_HEIGHT 500

typedef enum
{
  SEARCH_NAME,
  SEARCH_TITLE,
  SEARCH_DESCRIPTION,
  SEARCH_CATEGORY,
} FilterBrowserSearchType;

typedef struct
{
  GtkWidget *dialog;

  GtkWidget  *browser;
  GtkListBox *filter_list;

  GList *filters;

  FilterBrowserSearchType search_type;
} FilterBrowserPrivate;

typedef struct _FilterBrowser      FilterBrowser;
typedef struct _FilterBrowserClass FilterBrowserClass;

struct _FilterBrowser
{
  GimpPlugIn parent_instance;
};

struct _FilterBrowserClass
{
  GimpPlugInClass parent_class;
};

#define FILTER_BROWSER_TYPE (filter_browser_get_type ())
#define FILTER_BROWSER(obj) \
  (g_type_check_instance_cast ((obj), browser_type, FilterBrowser))

GType filter_browser_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (FilterBrowser, filter_browser, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FILTER_BROWSER_TYPE)
DEFINE_STD_SET_I18N

static GtkWidget *
create_filter_param_details (GParamSpec   *pspec,
                             GtkSizeGroup *sg_label,
                             GtkSizeGroup *sg_type,
                             GtkSizeGroup *sg_blurb)
{
  GtkWidget *param_box   = NULL;
  GtkWidget *hbox        = NULL;
  GtkWidget *label       = NULL;
  gchar     *name        = NULL;
  gchar     *type        = NULL;
  GString   *blurb       = g_string_new (NULL);
  gboolean   is_unknown  = FALSE;
  GType      gtype       = G_PARAM_SPEC_VALUE_TYPE (pspec);

  /* Some param specs are not supported by the wire. Thankfully, the
   * wire is well-behaved and returns NULL in such cases for each
   * param spec. */
  if (pspec == NULL)
    {
      label = gtk_label_new ("unknown");
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_widget_show (label);

      return label;
    }

  param_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start (GTK_BOX (param_box), hbox, FALSE, FALSE, 0);

  /* Some types might not be understood by the wire. In such a case the
   * Gimp_drawable_filter_operation_get_pspecs procedure encodes the
   * parameter information in the blurb of a placeholder GParamSpec.
   * See the definition of the procedure for more details.
   */
  is_unknown = G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_PARAM &&
               g_strcmp0 (pspec->name, "unknown") == 0;
  if (is_unknown)
    {
      gchar **parts = NULL;

      parts = g_strsplit (g_param_spec_get_blurb (pspec), ":", 4);

      name  = g_strdup (parts[1]);
      type  = g_strdup (parts[2]);
      g_string_append (blurb, parts[3] ? parts[3] : "");

      g_strfreev (parts);
    }
  else
    {
      gchar *blurb_s = NULL;

      name    = (gchar *) g_param_spec_get_name (pspec);
      type    = (gchar *) g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec));
      blurb_s = (gchar *) g_param_spec_get_blurb (pspec);
      g_string_append (blurb, blurb_s ? blurb_s : "");
    }

  label = gtk_label_new (name);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.0);
  gtk_size_group_add_widget (sg_label, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (type);
  gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_FAMILY, "monospace",
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC, -1);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.0);
  gtk_size_group_add_widget (sg_type, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (g_type_is_a (gtype, G_TYPE_DOUBLE))
    {
      GStrvBuilder *builder       = g_strv_builder_new ();
      gchar        *text          = NULL;
      GStrv         parts         = NULL;
      gdouble       default_value = G_PARAM_SPEC_DOUBLE (pspec)->default_value;
      gdouble       min           = G_PARAM_SPEC_DOUBLE (pspec)->minimum;
      gdouble       max           = G_PARAM_SPEC_DOUBLE (pspec)->maximum;

      if (default_value <= -G_MAXDOUBLE || default_value == G_MINFLOAT)
        g_strv_builder_add (builder, "default: -inf");
      else if (default_value >= G_MAXDOUBLE || default_value == G_MAXFLOAT)
        g_strv_builder_add (builder, "default: +inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("default: %2.2f", default_value));

      if (min <= -G_MAXDOUBLE || min == G_MINFLOAT)
        g_strv_builder_add (builder, "minimum: -inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("minimum: %2.2f", min));

      if (max >= G_MAXDOUBLE || max == G_MAXFLOAT)
        g_strv_builder_add (builder, "maximum: +inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("maximum: %2.2f", max));

      parts = g_strv_builder_end (builder);
      text  = g_strjoinv (", ", parts);

      g_string_append_printf (blurb, "\n<tt><i>%s</i></tt>", text);

      g_free (text);
      g_strfreev (parts);
      g_strv_builder_unref (builder);
    }

  if (g_type_is_a (gtype, G_TYPE_INT))
    {
      GStrvBuilder *builder       = g_strv_builder_new ();
      gchar        *text          = NULL;
      GStrv         parts         = NULL;
      gint          default_value = G_PARAM_SPEC_INT (pspec)->default_value;
      gint          min           = G_PARAM_SPEC_INT (pspec)->minimum;
      gint          max           = G_PARAM_SPEC_INT (pspec)->maximum;

      if (default_value < G_MININT)
        g_strv_builder_add (builder, "default: -inf");
      else if (default_value > G_MAXINT)
        g_strv_builder_add (builder, "default: +inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("default: %d", default_value));

      if (min <= -G_MAXINT)
        g_strv_builder_add (builder, "minimum: -inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("minimum: %d", min));

      if (max >= G_MAXINT)
        g_strv_builder_add (builder, "maximum: +inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("maximum: %d", max));

      parts = g_strv_builder_end (builder);
      text  = g_strjoinv (", ", parts);

      g_string_append_printf (blurb, "\n<tt><i>%s</i></tt>", text);

      g_free (text);
      g_strfreev (parts);
      g_strv_builder_unref (builder);
    }

  if (g_type_is_a (gtype, G_TYPE_UINT))
    {
      GStrvBuilder *builder       = g_strv_builder_new ();
      gchar        *text          = NULL;
      GStrv         parts         = NULL;
      guint         default_value = G_PARAM_SPEC_UINT (pspec)->default_value;
      guint         max           = G_PARAM_SPEC_UINT (pspec)->maximum;

      if (default_value > G_MAXUINT)
        g_strv_builder_add (builder, "default: +inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("default: %d", default_value));

      g_strv_builder_add (builder, "minimum: 0");

      if (max >= G_MAXUINT)
        g_strv_builder_add (builder, "maximum: +inf");
      else
        g_strv_builder_add (builder, g_strdup_printf ("maximum: %d", max));

      parts = g_strv_builder_end (builder);
      text  = g_strjoinv (", ", parts);

      g_string_append_printf (blurb, "\n<tt><i>%s</i></tt>", text);

      g_free (text);
      g_strfreev (parts);
      g_strv_builder_unref (builder);
    }

  if (g_type_is_a (gtype, G_TYPE_BOOLEAN))
    {
      gint          default_value = G_PARAM_SPEC_BOOLEAN (pspec)->default_value;

      g_string_append_printf (blurb, "\n<tt><i>default: %s</i></tt>", default_value ? "TRUE" : "FALSE");
    }

  if (GIMP_IS_PARAM_SPEC_CHOICE (pspec))
    {
      GimpChoice *choice  = NULL;
      GList      *choices = NULL;
      GString    *desc    = NULL;

      choice  = gimp_param_spec_choice_get_choice (pspec);
      choices = gimp_choice_list_nicks (choice);
      desc    = g_string_new ("\n");

      g_string_append_printf (desc, "<i>%s</i>", _("Allowed values:"));

      for (GList *iter = choices; iter != NULL; iter = iter->next)
        {
          gchar *nick  = iter->data;
          gchar *label = NULL;
          gchar *help  = NULL;

          gimp_choice_get_documentation (choice, (const gchar *) nick, (const gchar **) &label, (const gchar **) &help);
          nick  = g_markup_escape_text (nick, -1);
          label = g_markup_escape_text (label, -1);
          help  = (help != NULL ? g_markup_escape_text (help, -1) : NULL);
          /* \xe2\x80\xa2 is the UTF-8 for the bullet point. */
          if (help != NULL)
            g_string_append_printf (desc, "\n\xe2\x80\xa2 <tt>%s</tt>: %s\n\t%s", nick, label, help);
          else
            g_string_append_printf (desc, "\n\xe2\x80\xa2 <tt>%s</tt>: %s", nick, label);

          g_free (nick);
          g_free (label);
          g_free (help);
        }

      g_string_append (blurb, desc->str);
      g_string_free (desc, TRUE);
    }

  if (g_type_is_a (gtype, G_TYPE_ENUM))
    {
      GEnumClass   *eclass   = NULL;
      GType         etype    = 0;

      etype =  G_VALUE_TYPE (g_param_spec_get_default_value (pspec));
      eclass = g_type_class_ref (etype);

      g_string_append_printf (blurb, "\n<i>%s</i>", _("Allowed values:"));

      for (guint i = 0; i < eclass->n_values; i++ )
        {
          GEnumValue value = eclass->values[i];

          /* \xe2\x80\xa2 is the UTF-8 for the bullet point. */
          g_string_append_printf (blurb, "\n\xe2\x80\xa2 <tt>%s</tt>: %s", value.value_name, value.value_nick);
        }

      g_type_class_unref (eclass);
    }

  if (g_type_is_a (gtype, GEGL_TYPE_COLOR))
    {
      GeglColor *color = gegl_param_spec_color_get_default (pspec);
      gdouble    vals[4] = {};

      gegl_color_get_rgba (color, &vals[0], &vals[1], &vals[2], &vals[3]);

      g_string_append_printf (blurb, "\n<tt><i>default: (%1.1f, %1.1f, %1.1f, %1.1f)</i></tt>",
                              vals[0], vals[1], vals[2], vals[3]);
    }

  label = gtk_label_new (g_strstrip (blurb->str));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_size_group_add_widget (sg_blurb, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (is_unknown)
    {
      g_free (name);
      g_free (type);
    }

  g_string_free (blurb, TRUE);

  return param_box;
}

static GtkWidget *
create_filter_info_view (GimpGeglFilterInfo *filter_info)
{
  GtkWidget            *frame       = NULL;
  GtkWidget            *label       = NULL;
  GtkWidget            *vbox        = NULL;
  GtkWidget            *view        = NULL;
  gchar                *buf         = NULL;
  const gchar          *name        = NULL;
  const gchar          *title       = NULL;
  const gchar          *description = NULL;
  const gchar          *categories  = NULL;
  const gchar          *license     = NULL;
  const GimpValueArray *pspecs      = NULL;

  g_object_get (G_OBJECT (filter_info),
                "name",        &name,
                "title",       &title,
                "description", &description,
                "categories",  &categories,
                "license",     &license,
                "pspecs",      &pspecs,
                NULL);

  /*
   * TODO: Information to include
   *   - what is the are of effect? (Area, Point,..)
   *   - Special requirements and attributes (needs alpha, is position dependent?,..)
   */

  if (description && strlen (description) < 2)
    description = NULL;
  if (categories && strlen (categories) < 2)
    categories = NULL;
  if (license && strlen (license) < 2)
    license = NULL;

  view = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  /*
   * Main information
   */

  if (title != NULL)
    frame = gimp_frame_new (title);
  else
    frame = gimp_frame_new (name);
  gtk_label_set_selectable (GTK_LABEL (gtk_frame_get_label_widget (GTK_FRAME (frame))), TRUE);
  gtk_box_pack_start (GTK_BOX (view), frame, FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  buf   = g_strdup_printf (_("Operation name: %s"), name);
  label = gtk_label_new (buf);
  g_free (buf);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  if (categories != NULL)
    {
      gchar *categories_buf = NULL;

      categories_buf = g_strdelimit (g_strdup (categories), ":", ' ');
      buf   = g_strdup_printf (_("Categories: %s"), categories_buf);
      g_free (categories_buf);
      label = gtk_label_new (buf);
      g_free (buf);

      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    }

  if (description != NULL)
    {
      label = gtk_label_new (description);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    }

  /*
   * Parameters
   */

  if (pspecs != NULL && gimp_value_array_length (pspecs) != 0)
    {
      GtkSizeGroup *sg_label = NULL;
      GtkSizeGroup *sg_type  = NULL;
      GtkSizeGroup *sg_blurb = NULL;

      frame = gimp_frame_new (_ ("Parameters"));
      gtk_box_pack_start (GTK_BOX (view), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      sg_label = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
      sg_type  = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
      sg_blurb = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      for (gint i = 0; i < gimp_value_array_length (pspecs); i++)
        {
          GParamSpec *pspec = g_value_get_param (gimp_value_array_index (pspecs, i));

          gtk_box_pack_start (GTK_BOX (vbox),
                              create_filter_param_details (pspec, sg_label, sg_type, sg_blurb),
                              FALSE, FALSE, 0);
        }

      g_object_unref (sg_label);
      g_object_unref (sg_type);
      g_object_unref (sg_blurb);
    }

  /*
   * Additional information
   */

  if (license != NULL)
    {
      frame = gimp_frame_new (_("Additional Information"));
      gtk_box_pack_start (GTK_BOX (view), frame, FALSE, FALSE, 0);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      buf   = g_strdup_printf (_("License: %s"), license);
      label = gtk_label_new (buf);
      g_free (buf);

      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    }

  gtk_widget_show_all (view);

  return view;
}

static void
filter_list_row_selected (GtkListBox    *filter_list,
                          GtkListBoxRow *row,
                          gpointer       data)
{
  FilterBrowserPrivate *browser     = NULL;
  GimpGeglFilterInfo   *filter_info = NULL;

  browser = (FilterBrowserPrivate *) data;

  g_return_if_fail (GTK_IS_LIST_BOX (filter_list));
  if (row == NULL)
    return;

  filter_info = g_object_get_data (G_OBJECT (row), "filter-info");

  gimp_browser_set_widget (GIMP_BROWSER (browser->browser),
                           create_filter_info_view (filter_info));
}

static gint
gegl_filter_info_sort_func (gconstpointer a,
                            gconstpointer b)
{
  GimpGeglFilterInfo *info_a = (GimpGeglFilterInfo *) a;
  GimpGeglFilterInfo *info_b = (GimpGeglFilterInfo *) b;
  gchar              *name_a = NULL;
  gchar              *name_b = NULL;

  g_assert_true (GIMP_IS_GEGL_FILTER_INFO (info_a));
  g_assert_true (GIMP_IS_GEGL_FILTER_INFO (info_b));

  g_object_get (G_OBJECT (info_a), "name", &name_a, NULL);
  g_object_get (G_OBJECT (info_b), "name", &name_b, NULL);

  return g_strcmp0 (name_a, name_b);
}

static GtkWidget *
create_filter_list_row (gpointer item,
                        gpointer user_data)
{
  GimpGeglFilterInfo *filter_info = item;
  GtkWidget          *row         = NULL;
  GtkWidget          *box         = NULL;
  GtkWidget          *name_label  = NULL;
  const gchar        *name        = NULL;
  const gchar        *categories  = NULL;

  row = gtk_list_box_row_new ();

  g_object_set_data (G_OBJECT (row), "filter-info", filter_info);
  g_object_get (G_OBJECT (filter_info), "name", &name, "categories", &categories, NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_focus_on_click (box, FALSE);
  gtk_container_add (GTK_CONTAINER (row), box);

  name_label = gtk_label_new (name);
  gtk_box_pack_start (GTK_BOX (box), name_label, FALSE, FALSE, 0);

  gtk_widget_show_all (GTK_WIDGET (row));

  return row;
}

static void
insert_gegl_filter_info_into_list_store (gpointer data,
                                         gpointer user_data)
{
  GimpGeglFilterInfo *filter_info = (GimpGeglFilterInfo *) data;
  GListStore         *model       = (GListStore *) (user_data);

  g_assert_true (GIMP_IS_GEGL_FILTER_INFO (filter_info));
  g_assert_true (G_IS_LIST_STORE (model));

  g_list_store_append (model, filter_info);
}

static void
filter_browser_search (GimpBrowser            *gimp_browser,
                       const gchar            *search_text,
                       FilterBrowserSearchType search_type,
                       FilterBrowserPrivate   *browser)
{
  GListStore *new_model      = NULL;
  gchar      *search_msg     = NULL;
  gchar      *search_summary = NULL;
  guint       matches;

  switch (search_type)
    {
    case SEARCH_NAME:
      search_msg = _("Searching by name");
      break;
    case SEARCH_TITLE:
      search_msg = _("Searching by title");
      break;
    case SEARCH_DESCRIPTION:
      search_msg = _("Searching by description");
      break;
    case SEARCH_CATEGORY:
      search_msg = _("Searching by category");
    default:
      return;
    }
  gimp_browser_show_message (GIMP_BROWSER (browser->browser), search_msg);

  new_model = g_list_store_new (GIMP_GEGL_FILTER_TYPE_INFO);

  if (strlen (search_text) != 0)
    {
      for (GList *iter = browser->filters; iter != NULL; iter = iter->next)
        {
          GimpGeglFilterInfo *filter_info = NULL;
          const gchar        *value       = NULL;

          filter_info = (GimpGeglFilterInfo *) iter->data;

          switch (search_type)
            {
            case SEARCH_NAME:
              g_object_get (G_OBJECT (filter_info), "name", &value, NULL);
              break;
            case SEARCH_TITLE:
              g_object_get (G_OBJECT (filter_info), "title", &value, NULL);
              break;
            case SEARCH_DESCRIPTION:
              g_object_get (G_OBJECT (filter_info), "description", &value, NULL);
              break;
            case SEARCH_CATEGORY:
              g_object_get (G_OBJECT (filter_info), "categories", &value, NULL);
            default:
              return;
            }

          if (value == NULL)
            continue;

          if (! g_str_match_string (search_text, value, TRUE))
            continue;

          insert_gegl_filter_info_into_list_store (filter_info, new_model);
        }
    }
  else
    {
      g_list_foreach (browser->filters, insert_gegl_filter_info_into_list_store, new_model);
    }

  gtk_list_box_bind_model (browser->filter_list, G_LIST_MODEL (new_model),
                           create_filter_list_row, NULL, NULL);
  gtk_list_box_select_row (browser->filter_list,
                           gtk_list_box_get_row_at_index (browser->filter_list, 0));

  matches = g_list_model_get_n_items (G_LIST_MODEL (new_model));
  if (search_text != NULL && strlen (search_text) > 0)
    {
      if (matches == 0)
        {
          search_summary = g_strdup (_("No matches for your query"));
          gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                     _("No matches"));
        }
      else
        {
          search_summary = g_strdup_printf (
              ngettext ("%d filter matches your query", "%d filters match your query", matches),
              matches);
        }
    }
  else
    {
      search_summary = g_strdup_printf (ngettext ("%d filter", "%d filters", matches), matches);
    }
  gimp_browser_set_search_summary (gimp_browser, search_summary);
  g_free (search_summary);
}

static void
browser_dialog_response (GtkWidget            *widget,
                         gint                  response_id,
                         FilterBrowserPrivate *browser)
{
  gtk_widget_destroy (browser->dialog);
  g_list_free (browser->filters);
  g_free (browser);
  g_main_loop_quit (g_main_loop_new (NULL, TRUE));
}

static GimpValueArray *
filter_browser_run (GimpProcedure       *procedure,
                    GimpProcedureConfig *config,
                    gpointer             run_data)
{
  FilterBrowserPrivate  *browser         = NULL;
  GtkWidget             *scrolled_window = NULL;
  GtkStyleContext       *style_context   = NULL;
  GListStore            *model           = NULL;
  gchar                **filter_names    = NULL;

  gimp_ui_init (PLUG_IN_BINARY);

  browser              = g_new0 (FilterBrowserPrivate, 1);
  browser->search_type = SEARCH_NAME;

  browser->dialog = gimp_dialog_new (_("GEGL Filter Browser"),
                                     PLUG_IN_ROLE,
                                     NULL,
                                     0,
                                     gimp_standard_help_func,
                                     PLUG_IN_GEGL_FILTER_BROWSER,
                                     _("_Close"),
                                     GTK_RESPONSE_CLOSE,
                                     NULL);
  gtk_window_set_default_size (GTK_WINDOW (browser->dialog),
                               FILTER_BROWSER_WIDTH, FILTER_BROWSER_HEIGHT);

  g_signal_connect (browser->dialog, "response",
                    G_CALLBACK (browser_dialog_response), browser);

  browser->browser = gimp_browser_new ();
  gimp_browser_add_search_types (GIMP_BROWSER (browser->browser),
                                 _("by name"), SEARCH_NAME,
                                 _("by title"), SEARCH_TITLE,
                                 _("by description"), SEARCH_DESCRIPTION,
                                 NULL);
  gtk_container_set_border_width (GTK_CONTAINER (browser->browser), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (browser->dialog))),
                      browser->browser, TRUE, TRUE, 0);
  gtk_widget_show (browser->browser);

  g_signal_connect (browser->browser, "search", G_CALLBACK (filter_browser_search), browser);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (gimp_browser_get_left_vbox (GIMP_BROWSER (browser->browser))),
                      scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  /*
   * TODO: Extract the data preparation, so that it can run on idle and not
   * block the GUI.
   */
  filter_names = gimp_drawable_filter_operation_get_available ();
  for (gint i = 0; filter_names[i] != NULL; i++)
    {
      GimpGeglFilterInfo *info   = NULL;
      GimpValueArray     *pspecs = NULL;
      GimpValueArray     *values = NULL;
      GStrv               names  = NULL;

      gimp_drawable_filter_operation_get_details (filter_names[i],
                                                  &names, &values);
      pspecs = gimp_drawable_filter_operation_get_pspecs (filter_names[i]);

      info = g_object_new (GIMP_GEGL_FILTER_TYPE_INFO,
                           "name",        filter_names[i],
                           "title",       g_value_get_string (gimp_value_array_index (values, 0)),
                           "description", g_value_get_string (gimp_value_array_index (values, 1)),
                           "categories",  g_value_get_string (gimp_value_array_index (values, 2)),
                           "license",     g_value_get_string (gimp_value_array_index (values, 3)),
                           "pspecs",      pspecs,
                           NULL);
      browser->filters = g_list_append (browser->filters, info);

      g_strfreev (names);
      gimp_value_array_unref (values);
      gimp_value_array_unref (pspecs);
    }
  g_strfreev (filter_names);

  browser->filters = g_list_sort (browser->filters, gegl_filter_info_sort_func);

  model = g_list_store_new (GIMP_GEGL_FILTER_TYPE_INFO);
  g_list_foreach (browser->filters, insert_gegl_filter_info_into_list_store, model);

  browser->filter_list = GTK_LIST_BOX (gtk_list_box_new ());
  style_context = gtk_widget_get_style_context (GTK_WIDGET (browser->filter_list));
  gtk_style_context_add_class (style_context, "view");
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (browser->filter_list), GTK_SELECTION_BROWSE);
  gtk_list_box_bind_model (GTK_LIST_BOX (browser->filter_list),
                           G_LIST_MODEL (model), create_filter_list_row, NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (browser->filter_list));
  gtk_widget_show (GTK_WIDGET (browser->filter_list));

  g_signal_connect (browser->filter_list, "row-selected",
                    G_CALLBACK (filter_list_row_selected), browser);

  gtk_list_box_select_row (browser->filter_list,
                           gtk_list_box_get_row_at_index (browser->filter_list, 0));

  gtk_widget_show (GTK_WIDGET (browser->dialog));
  g_main_loop_run (g_main_loop_new (NULL, TRUE));

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static GimpProcedure *
filter_browser_create_procedure (GimpPlugIn  *plug_in,
                                 const gchar *procedure_name)
{
  GimpProcedure *procedure = NULL;

  if (strcmp (procedure_name, PLUG_IN_GEGL_FILTER_BROWSER))
    return procedure;

  procedure = gimp_procedure_new (plug_in, procedure_name, GIMP_PDB_PROC_TYPE_PLUGIN,
                                  filter_browser_run, NULL, NULL);

  gimp_procedure_set_menu_label (procedure, _ ("_GEGL Filter Browser"));
  gimp_procedure_set_icon_name (procedure, GIMP_ICON_PLUGIN);
  gimp_procedure_add_menu_path (procedure, "<Image>/Help/[Programming]");

  gimp_procedure_set_documentation (procedure,
                                    _("Display information about available"
                                      " GEGL operations (i.e., filters)."),
                                    _("Shows a list of all GEGL operations,"
                                      " their details and what parameters they"
                                      " are configured with. The list contains"
                                      " operations provided by GEGL itself,"
                                      " GIMP and plug-ins loaded by GEGL."),
                                    PLUG_IN_GEGL_FILTER_BROWSER);
  gimp_procedure_set_attribution (procedure, "Ondřej Míchal", "Ondřej Míchal", "2025");

  gimp_procedure_add_enum_argument (procedure, "run-mode", "Run mode",
                                    "The run mode", GIMP_TYPE_RUN_MODE,
                                    GIMP_RUN_INTERACTIVE, G_PARAM_READWRITE);

  return procedure;
}

static GList *
filter_browser_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_GEGL_FILTER_BROWSER));
}

static void
filter_browser_init (FilterBrowser *browser)
{
}

static void
filter_browser_class_init (FilterBrowserClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = filter_browser_query_procedures;
  plug_in_class->create_procedure = filter_browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}
