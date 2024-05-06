/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GTM plug-in --- GIMP Table Magic
 * Allows images to be exported as HTML tables with different colored cells.
 * It doesn't  have very much practical use other than being able to
 * easily design a table by "painting" it in GIMP, or to make small HTML
 * table images/icons.
 *
 * Copyright (C) 1997 Daniel Dunbar
 * Email: ddunbar@diads.com
 * WWW:   http://millennium.diads.com/gimp/
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

/* Version 1.0:
 * Once I first found out that it was possible to have pixel level control
 * of HTML tables I instantly realized that it would be possible, however
 * pointless, to save an image as a, albeit huge, HTML table.
 *
 * One night when I was feeling in an adventourously stupid programming mood
 * I decided to write a program to do it.
 *
 * At first I just wrote a really ugly hack to do it, which I then planned
 * on using once just to see how it worked, and then posting a URL and
 * laughing about it on #gimp.  As it turns out, tigert thought it actually
 * had potential to be a useful plugin, so I started adding features and
 * and making a nice UI.
 *
 * It's still not very useful, but I did manage to significantly improve my
 * C programming skills in the process, so it was worth it.
 *
 * If you happen to find it useful I would appreciate any email about it.
 *                                     - Daniel Dunbar
 *                                       ddunbar@diads.com
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define EXPORT_PROC    "file-html-table-export"
#define PLUG_IN_BINARY "file-html-table"


typedef struct _Html      Html;
typedef struct _HtmlClass HtmlClass;

struct _Html
{
  GimpPlugIn      parent_instance;
};

struct _HtmlClass
{
  GimpPlugInClass parent_class;
};


#define HTML_TYPE  (html_get_type ())
#define HTML(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HTML_TYPE, Html))

GType                   html_get_type          (void) G_GNUC_CONST;

static GList          * html_query_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * html_create_procedure  (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * html_export            (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                GFile                *file,
                                                GimpExportOptions    *options,
                                                GimpMetadata         *metadata,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);

static gboolean         export_image           (GFile                *file,
                                                GeglBuffer           *buffer,
                                                GObject              *config,
                                                GError              **error);
static gboolean         save_dialog            (GimpImage            *image,
                                                GimpProcedure        *procedure,
                                                GObject              *config);

static gboolean         print                  (GOutputStream        *output,
                                                GError              **error,
                                                const gchar          *format,
                                                ...) G_GNUC_PRINTF (3, 0);
static gboolean         color_comp             (guchar               *buffer,
                                                guchar               *buf2);


G_DEFINE_TYPE (Html, html, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (HTML_TYPE)
DEFINE_STD_SET_I18N


static void
html_class_init (HtmlClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = html_query_procedures;
  plug_in_class->create_procedure = html_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
html_init (Html *html)
{
}

static GList *
html_query_procedures (GimpPlugIn *plug_in)
{
  return  g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static GimpProcedure *
html_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, html_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("HTML table"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("HTML Table"));

      gimp_procedure_set_documentation (procedure,
                                        _("GIMP Table Magic"),
                                        _("Allows you to draw an HTML table "
                                          "in GIMP. See help for more info."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Dunbar",
                                      "Daniel Dunbar",
                                      "1998");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "text/html");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "html,htm");

      gimp_procedure_add_boolean_aux_argument (procedure, "use-caption",
                                               _("Use c_aption"),
                                               _("Enable if you would like to have the table "
                                                 "captioned."),
                                               FALSE,
                                               GIMP_PARAM_READWRITE);

      gimp_procedure_add_string_aux_argument (procedure, "caption-text",
                                              _("Capt_ion"),
                                              _("The text for the table caption."),
                                              "Made with GIMP Table Magic",
                                              GIMP_PARAM_READWRITE);

      gimp_procedure_add_string_aux_argument (procedure, "cell-content",
                                              _("Cell con_tent"),
                                              _("The text to go into each cell."),
                                              "&nbsp;",
                                              GIMP_PARAM_READWRITE);

      gimp_procedure_add_string_aux_argument (procedure, "cell-width",
                                              _("_Width"),
                                              _("The width for each table cell. "
                                                "Can be a number or a percent."),
                                              "",
                                              GIMP_PARAM_READWRITE);

      gimp_procedure_add_string_aux_argument (procedure, "cell-height",
                                              _("_Height"),
                                              _("The height for each table cell. "
                                                "Can be a number or a percent."),
                                              "",
                                              GIMP_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "full-document",
                                               _("_Generate full HTML document"),
                                               _("If enabled GTM will output a full HTML "
                                                 "document with <HTML>, <BODY>, etc. tags "
                                                 "instead of just the table html."),
                                               TRUE,
                                               GIMP_PARAM_READWRITE);

      gimp_procedure_add_int_aux_argument (procedure, "border",
                                           _("_Border"),
                                           _("The number of pixels in the table border."),
                                           0, 1000, 2,
                                           GIMP_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "span-tags",
                                               _("_Use cellspan"),
                                               _("If enabled GTM will replace any "
                                                 "rectangular sections of identically "
                                                 "colored blocks with one large cell with "
                                                 "ROWSPAN and COLSPAN values."),
                                               FALSE,
                                               GIMP_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "compress-td-tags",
                                               _("Co_mpress TD tags"),
                                               _("Enabling this will cause GTM to "
                                                 "leave no whitespace between the TD "
                                                 "tags and the cell content. This is only "
                                                 "necessary for pixel level positioning "
                                                 "control."),
                                               FALSE,
                                               GIMP_PARAM_READWRITE);

      gimp_procedure_add_int_aux_argument (procedure, "cell-padding",
                                           _("Cell-pa_dding"),
                                           _("The amount of cell padding."),
                                           0, 1000, 4,
                                           GIMP_PARAM_READWRITE);

      gimp_procedure_add_int_aux_argument (procedure, "cell-spacing",
                                           _("Cell spaci_ng"),
                                           _("The amount of cell spacing."),
                                           0, 1000, 0,
                                           GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
html_export (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GFile                *file,
             GimpExportOptions    *options,
             GimpMetadata         *metadata,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpPDBStatusType  status    = GIMP_PDB_SUCCESS;
  GList             *drawables = gimp_image_list_layers (image);
  GeglBuffer        *buffer;
  GError            *error     = NULL;

  gegl_init (NULL, NULL);

  if (run_mode != GIMP_RUN_INTERACTIVE)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             NULL);

  if (! save_dialog (image, procedure, G_OBJECT (config)))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  buffer = gimp_drawable_get_buffer (drawables->data);

  if (! export_image (file, buffer, G_OBJECT (config),
                      &error))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  g_object_unref (buffer);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
export_image (GFile       *file,
              GeglBuffer  *buffer,
              GObject     *config,
              GError     **error)
{
  const Babl    *format = babl_format ("R'G'B'A u8");
  GeglSampler   *sampler;
  GCancellable  *cancellable;
  GOutputStream *output;
  gint           row, col;
  gint           cols, rows;
  gint           x, y;
  gint           colcount, colspan, rowspan;
  gint          *palloc;
  guchar        *buf, *buf2;
  gchar         *width  = NULL;
  gchar         *height = NULL;
  gboolean       config_use_caption;
  gchar         *config_caption_text;
  gchar         *config_cell_content;
  gchar         *config_cell_width;
  gchar         *config_cell_height;
  gboolean       config_full_document;
  gint           config_border;
  gboolean       config_span_tags;
  gboolean       config_compress_td_tags;
  gint           config_cell_padding;
  gint           config_cell_spacing;

  g_object_get (config,
                "use-caption",      &config_use_caption,
                "caption-text",     &config_caption_text,
                "cell-content",     &config_cell_content,
                "cell-width",       &config_cell_width,
                "cell-height",      &config_cell_height,
                "full-document",    &config_full_document,
                "border",           &config_border,
                "span-tags",        &config_span_tags,
                "compress-td-tags", &config_compress_td_tags,
                "cell-padding",     &config_cell_padding,
                "cell-spacing",     &config_cell_spacing,
                NULL);

  if (! config_caption_text) config_caption_text = g_strdup ("");
  if (! config_cell_content) config_cell_content = g_strdup ("");
  if (! config_cell_width)   config_cell_width   = g_strdup ("");
  if (! config_cell_height)  config_cell_height  = g_strdup ("");

  cols = gegl_buffer_get_width  (buffer);
  rows = gegl_buffer_get_height (buffer);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GOutputStream *buffered;

      buffered = g_buffered_output_stream_new (output);
      g_object_unref (output);

      output = buffered;
    }
  else
    {
      return FALSE;
    }

  sampler = gegl_buffer_sampler_new (buffer, format, GEGL_SAMPLER_NEAREST);

  palloc = g_new (int, rows * cols);

  if (config_full_document)
    {
      if (! print (output, error,
                   "<HTML>\n<HEAD><TITLE>%s</TITLE></HEAD>\n<BODY>\n",
                   gimp_file_get_utf8_name (file)) ||
          ! print (output, error, "<H1>%s</H1>\n",
                   gimp_file_get_utf8_name (file)))
        {
          goto fail;
        }
    }

  if (! print (output, error,
               "<TABLE BORDER=%d CELLPADDING=%d CELLSPACING=%d>\n",
               config_border, config_cell_padding, config_cell_spacing))
    goto fail;

  if (config_use_caption)
    {
      if (! print (output, error, "<CAPTION>%s</CAPTION>\n",
                   config_caption_text))
        goto fail;
    }

  buf  = g_newa (guchar, babl_format_get_bytes_per_pixel (format));
  buf2 = g_newa (guchar, babl_format_get_bytes_per_pixel (format));

  if (strcmp (config_cell_width, "") != 0)
    {
      width = g_strdup_printf (" WIDTH=\"%s\"", config_cell_width);
    }

  if (strcmp (config_cell_height, "") != 0)
    {
      height = g_strdup_printf (" HEIGHT=\"%s\" ", config_cell_height);
    }

  if (! width)
    width = g_strdup (" ");

  if (! height)
    height = g_strdup (" ");

  /* Initialize array to hold ROWSPAN and COLSPAN cell allocation table */

  for (row = 0; row < rows; row++)
    for (col = 0; col < cols; col++)
      palloc[cols * row + col] = 1;

  colspan = 0;
  rowspan = 0;

  for (y = 0; y < rows; y++)
    {
      if (! print (output, error, "   <TR>\n"))
        goto fail;

      for (x = 0; x < cols; x++)
        {
          gegl_sampler_get (sampler, x, y, NULL, buf, GEGL_ABYSS_NONE);

          /* Determine ROWSPAN and COLSPAN */

          if (config_span_tags)
            {
              col      = x;
              row      = y;
              colcount = 0;
              colspan  = 0;
              rowspan  = 0;

              gegl_sampler_get (sampler, col, row, NULL, buf2, GEGL_ABYSS_NONE);

              while (color_comp (buf, buf2) &&
                     palloc[cols * row + col] == 1 &&
                     row < rows)
                {
                  while (color_comp (buf, buf2) &&
                         palloc[cols * row + col] == 1 &&
                         col < cols)
                    {
                      colcount++;
                      col++;

                      gegl_sampler_get (sampler,
                                        col, row, NULL, buf2, GEGL_ABYSS_NONE);
                    }

                  if (colcount != 0)
                    {
                      row++;
                      rowspan++;
                    }

                  if (colcount < colspan || colspan == 0)
                    colspan = colcount;

                  col = x;
                  colcount = 0;

                  gegl_sampler_get (sampler,
                                    col, row, NULL, buf2, GEGL_ABYSS_NONE);
                }

              if (colspan > 1 || rowspan > 1)
                {
                  for (row = 0; row < rowspan; row++)
                    for (col = 0; col < colspan; col++)
                      palloc[cols * (row + y) + (col + x)] = 0;

                  palloc[cols * y + x] = 2;
                }
            }

          if (palloc[cols * y + x] == 1)
            {
              if (! print (output, error,
                           "      <TD%s%sBGCOLOR=#%02x%02x%02x>",
                           width, height, buf[0], buf[1], buf[2]))
                goto fail;
            }

          if (palloc[cols * y + x] == 2)
            {
              if (! print (output, error,
                           "      <TD ROWSPAN=\"%d\" COLSPAN=\"%d\"%s%sBGCOLOR=#%02x%02x%02x>",
                           rowspan, colspan, width, height,
                           buf[0], buf[1], buf[2]))
                goto fail;
            }

          if (palloc[cols * y + x] != 0)
            {
              if (config_compress_td_tags)
                {
                  if (! print (output, error,
                               "%s</TD>\n", config_cell_content))
                    goto fail;
                }
              else
                {
                  if (! print (output, error,
                               "\n      %s\n      </TD>\n", config_cell_content))
                    goto fail;
                }
            }
        }

      if (! print (output, error, "   </TR>\n"))
        goto fail;

      gimp_progress_update ((double) y / (double) rows);
    }

  if (config_full_document)
    {
      if (! print (output, error, "</TABLE></BODY></HTML>\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error, "</TABLE>\n"))
        goto fail;
    }

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_object_unref (output);
  g_object_unref (sampler);
  g_free (width);
  g_free (height);
  g_free (palloc);

  g_free (config_caption_text);
  g_free (config_cell_content);
  g_free (config_cell_width);
  g_free (config_cell_height);

  gimp_progress_update (1.0);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);
  g_object_unref (cancellable);

  g_object_unref (output);
  g_object_unref (sampler);
  g_free (width);
  g_free (height);
  g_free (palloc);

  g_free (config_caption_text);
  g_free (config_cell_content);
  g_free (config_cell_width);
  g_free (config_cell_height);

  return FALSE;
}

static gint
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  if (gimp_image_get_width (image) * gimp_image_get_height (image) > 4096)
    {
      GtkWidget *eek;
      GtkWidget *hbox;

      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "warning-label",
                                       _("You are about to create a huge\n"
                                        "HTML file which will most likely\n"
                                        "crash your browser."),
                                       FALSE, FALSE);
      hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                             "warning-hbox", "warning-label",
                                             NULL);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

      eek = gtk_image_new_from_icon_name (GIMP_ICON_WILBER_EEK,
                                          GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), eek, FALSE, FALSE, 0);
      gtk_widget_show (eek);
      gtk_box_reorder_child (GTK_BOX (hbox), eek, 0);
      gtk_widget_set_margin_end (eek, 24);

      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "warning-frame-label", _("Warning"),
                                       FALSE, FALSE);
      frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                                "warning-frame",
                                                "warning-frame-label",
                                                FALSE, "warning-hbox");
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "warning-frame", NULL);
    }

  /* HTML Page Options */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "page-label", _("HTML Page Options"),
                                   FALSE, FALSE);
  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "page-frame",
                                            "page-label",
                                            FALSE, "full-document");
  gtk_widget_set_margin_bottom (frame, 8);

  /* HTML Table Creation Options */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "creation-label",
                                   _("Table Creation Options"),
                                   FALSE, FALSE);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "creation-vbox", "span-tags",
                                  "compress-td-tags", "use-caption",
                                  "caption-text", "cell-content", NULL);

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "caption-text", TRUE,
                                       config, "use-caption",
                                       FALSE);

  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "creation-frame",
                                            "creation-label",
                                            FALSE, "creation-vbox");
  gtk_widget_set_margin_bottom (frame, 8);
  /* HTML Table Options */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "table-label", _("Table Options"),
                                   FALSE, FALSE);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "table-vbox", "border", "cell-width",
                                  "cell-height", "cell-padding",
                                  "cell-spacing", NULL);

  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "table-frame", "table-label",
                                            FALSE, "table-vbox");
  gtk_widget_set_margin_bottom (frame, 8);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "page-frame", "creation-frame",
                              "table-frame", NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
print (GOutputStream  *output,
       GError        **error,
       const gchar    *format,
       ...)
{
  va_list  args;
  gboolean success;

  va_start (args, format);
  success = g_output_stream_vprintf (output, NULL, NULL,
                                     error, format, args);
  va_end (args);

  return success;
}

static gboolean
color_comp (guchar *buf,
            guchar *buf2)
{
  return (buf[0] == buf2[0] &&
          buf[1] == buf2[1] &&
          buf[2] == buf2[2]);
}
