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


#define SAVE_PROC      "file-html-table-save"
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
#define HTML (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HTML_TYPE, Html))

GType                   html_get_type          (void) G_GNUC_CONST;

static GList          * html_query_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * html_create_procedure  (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * html_save              (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                gint                  n_drawables,
                                                GimpDrawable        **drawables,
                                                GFile                *file,
                                                const GimpValueArray *args,
                                                gpointer              run_data);

static gboolean         save_image             (GFile                *file,
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


static void
html_class_init (HtmlClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = html_query_procedures;
  plug_in_class->create_procedure = html_create_procedure;
}

static void
html_init (Html *html)
{
}

static GList *
html_query_procedures (GimpPlugIn *plug_in)
{
  return  g_list_append (NULL, g_strdup (SAVE_PROC));
}

static GimpProcedure *
html_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           html_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("HTML table"));

      gimp_procedure_set_documentation (procedure,
                                        "GIMP Table Magic",
                                        "Allows you to draw an HTML table "
                                        "in GIMP. See help for more info.",
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

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "use-caption",
                                 "Use caption",
                                 _("Enable if you would like to have the table "
                                   "captioned."),
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_STRING (procedure, "caption-text",
                                "Caption text",
                                _("The text for the table caption."),
                                "Made with GIMP Table Magic",
                                GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_STRING (procedure, "cell-content",
                                "Cell content",
                                _("The text to go into each cell."),
                                "&nbsp;",
                                GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_STRING (procedure, "cell-width",
                                "Cell width",
                                _("The width for each table cell. "
                                  "Can be a number or a percent."),
                                "",
                                GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_STRING (procedure, "cell-height",
                                "Cell height",
                                _("The height for each table cell. "
                                  "Can be a number or a percent."),
                                "",
                                GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "full-document",
                                 "Full document",
                                 _("If enabled GTM will output a full HTML "
                                   "document with <HTML>, <BODY>, etc. tags "
                                   "instead of just the table html."),
                                 TRUE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_INT (procedure, "border",
                             "Border",
                             _("The number of pixels in the table border."),
                             0, 1000, 2,
                             GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "span-tags",
                                 "Span tags",
                                 _("If enabled GTM will replace any "
                                   "rectangular sections of identically "
                                   "colored blocks with one large cell with "
                                   "ROWSPAN and COLSPAN values."),
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "compress-td-tags",
                                 "Compress td tags",
                                 _("Enabling this will cause GTM to "
                                   "leave no whitespace between the TD "
                                   "tags and the cell content. This is only "
                                   "necessary for pixel level positioning "
                                   "control."),
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_INT (procedure, "cell-padding",
                             "Cell padding",
                             _("The amount of cell padding."),
                             0, 1000, 4,
                             GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_INT (procedure, "cell-spacing",
                             "Cell spacing",
                             _("The amount of cell spacing."),
                             0, 1000, 0,
                             GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
html_save (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GFile                *file,
           const GimpValueArray *args,
           gpointer              run_data)
{
  GimpProcedureConfig *config;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  GeglBuffer          *buffer;
  GError              *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (run_mode != GIMP_RUN_INTERACTIVE)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_run (config, image, run_mode, args);

  if (! save_dialog (image, procedure, G_OBJECT (config)))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("HTML table plug-in does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  buffer = gimp_drawable_get_buffer (drawables[0]);

  if (! save_image (file, buffer, G_OBJECT (config),
                    &error))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  g_object_unref (buffer);

  gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_image (GFile       *file,
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
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *spinbutton;
  GtkWidget *entry;
  GtkWidget *toggle;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as HTML Table"));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  if (gimp_image_width (image) * gimp_image_height (image) > 4096)
    {
      GtkWidget *eek;
      GtkWidget *label;
      GtkWidget *hbox;

      frame = gimp_frame_new (_("Warning"));
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_container_add (GTK_CONTAINER (frame), hbox);

      eek = gtk_image_new_from_icon_name (GIMP_ICON_WILBER_EEK,
                                          GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), eek, FALSE, FALSE, 0);

      label = gtk_label_new (_("You are about to create a huge\n"
                               "HTML file which will most likely\n"
                               "crash your browser."));
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

      gtk_widget_show_all (frame);
    }

  /* HTML Page Options */
  frame = gimp_frame_new (_("HTML Page Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gimp_prop_check_button_new (config, "full-document",
                                       _("_Generate full HTML document"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* HTML Table Creation Options */
  frame = gimp_frame_new (_("Table Creation Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  toggle = gimp_prop_check_button_new (config, "span-tags",
                                       _("_Use cellspan"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 0, 2, 1);

  toggle = gimp_prop_check_button_new (config, "compress-td-tags",
                                       _("Co_mpress TD tags"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 1, 2, 1);

  toggle = gimp_prop_check_button_new (config, "use-caption",
                                       _("C_aption"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 2, 1, 1);

  entry = gimp_prop_entry_new (config, "caption-text", -1);
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);

  g_object_bind_property (config, "use-caption",
                          entry,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  entry = gimp_prop_entry_new (config, "cell-content", -1);
  gtk_widget_set_size_request (entry, 200, -1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("C_ell content:"), 0.0, 0.5,
                            entry, 1);

  /* HTML Table Options */
  frame = gimp_frame_new (_("Table Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  spinbutton = gimp_prop_spin_button_new (config, "border",
                                          1, 10, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Border:"), 0.0, 0.5,
                            spinbutton, 1);

  entry = gimp_prop_entry_new (config, "cell-width", -1);
  gtk_widget_set_size_request (entry, 60, -1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Width:"), 0.0, 0.5,
                            entry, 1);

  entry = gimp_prop_entry_new (config, "cell-height", -1);
  gtk_widget_set_size_request (entry, 60, -1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Height:"), 0.0, 0.5,
                            entry, 1);

  spinbutton = gimp_prop_spin_button_new (config, "cell-padding",
                                          1, 10, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("Cell-_padding:"), 0.0, 0.5,
                            spinbutton, 1);

  spinbutton = gimp_prop_spin_button_new (config, "cell-spacing",
                                          1, 10, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Cell-_spacing:"), 0.0, 0.5,
                            spinbutton, 1);

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
