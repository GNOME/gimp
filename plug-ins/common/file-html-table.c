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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


#define SAVE_PROC      "file-gtm-save"
#define PLUG_IN_BINARY "file-html-table"
#define PLUG_IN_ROLE   "gimp-file-html-table"


/* Typedefs */

typedef struct
{
  gchar    captiontxt[256];
  gchar    cellcontent[256];
  gchar    clwidth[256];
  gchar    clheight[256];
  gboolean fulldoc;
  gboolean caption;
  gint     border;
  gboolean spantags;
  gboolean tdcomp;
  gint     cellpadding;
  gint     cellspacing;
} GTMValues;


/* Declare some local functions */

static void     query                    (void);
static void     run                      (const gchar      *name,
                                          gint              nparams,
                                          const GimpParam  *param,
                                          gint             *nreturn_vals,
                                          GimpParam       **return_vals);

static gboolean save_image               (GFile            *file,
                                          GeglBuffer       *buffer,
                                          GError          **error);
static gboolean save_dialog              (gint32            image_ID);

static gboolean print                    (GOutputStream    *output,
                                          GError          **error,
                                          const gchar      *format,
                                          ...) G_GNUC_PRINTF (3, 0);
static gboolean color_comp               (guchar           *buffer,
                                          guchar           *buf2);
static void     entry_changed_callback   (GtkEntry         *entry,
                                          gchar            *string);


/* Variables */

static GTMValues gtmvals =
{
  "Made with GIMP Table Magic",  /* caption text */
  "&nbsp;",  /* cellcontent text */
  "",        /* cell width text */
  "",        /* cell height text */
  TRUE,      /* fulldoc */
  FALSE,     /* caption */
  2,         /* border */
  FALSE,     /* spantags */
  FALSE,     /* tdcomp */
  4,         /* cellpadding */
  0          /* cellspacing */
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to export" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to export the image in" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "GIMP Table Magic",
                          "Allows you to draw an HTML table in GIMP. See help for more info.",
                          "Daniel Dunbar",
                          "Daniel Dunbar",
                          "1998",
                          _("HTML table"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "text/html");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "html,htm", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  gimp_get_data (SAVE_PROC, &gtmvals);

  if (save_dialog (param[1].data.d_int32))
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (param[2].data.d_int32);

      if (save_image (g_file_new_for_uri (param[3].data.d_string),
                      buffer, &error))
        {
          gimp_set_data (SAVE_PROC, &gtmvals, sizeof (GTMValues));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      g_object_unref (buffer);
    }
  else
    {
      status = GIMP_PDB_CANCEL;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static gboolean
save_image (GFile       *file,
            GeglBuffer  *buffer,
            GError     **error)
{
  const Babl    *format = babl_format ("R'G'B'A u8");
  GeglSampler   *sampler;
  GOutputStream *output;
  gint           row, col;
  gint           cols, rows;
  gint           x, y;
  gint           colcount, colspan, rowspan;
  gint          *palloc;
  guchar        *buf, *buf2;
  gchar         *width  = NULL;
  gchar         *height = NULL;

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

  if (gtmvals.fulldoc)
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
               gtmvals.border, gtmvals.cellpadding, gtmvals.cellspacing))
    goto fail;

  if (gtmvals.caption)
    {
      if (! print (output, error, "<CAPTION>%s</CAPTION>\n",
                   gtmvals.captiontxt))
        goto fail;
    }

  buf  = g_newa (guchar, babl_format_get_bytes_per_pixel (format));
  buf2 = g_newa (guchar, babl_format_get_bytes_per_pixel (format));

  if (strcmp (gtmvals.clwidth, "") != 0)
    {
      width = g_strdup_printf (" WIDTH=\"%s\"", gtmvals.clwidth);
    }

  if (strcmp (gtmvals.clheight, "") != 0)
    {
      height = g_strdup_printf (" HEIGHT=\"%s\" ", gtmvals.clheight);
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

          if (gtmvals.spantags)
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
              if (gtmvals.tdcomp)
                {
                  if (! print (output, error,
                               "%s</TD>\n", gtmvals.cellcontent))
                    goto fail;
                }
              else
                {
                  if (! print (output, error,
                               "\n      %s\n      </TD>\n", gtmvals.cellcontent))
                    goto fail;
                }
            }
        }

      if (! print (output, error, "   </TR>\n"))
        goto fail;

      gimp_progress_update ((double) y / (double) rows);
    }

  if (gtmvals.fulldoc)
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

  gimp_progress_update (1.0);

  return TRUE;

 fail:

  g_object_unref (output);
  g_object_unref (sampler);
  g_free (width);
  g_free (height);
  g_free (palloc);

  return FALSE;
}

static gint
save_dialog (gint32 image_ID)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *grid;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  GtkWidget     *entry;
  GtkWidget     *toggle;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_export_dialog_new (_("HTML table"), PLUG_IN_BINARY, SAVE_PROC);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      main_vbox, TRUE, TRUE, 0);

  if (gimp_image_width (image_ID) * gimp_image_height (image_ID) > 4096)
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Generate full HTML document"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gtmvals.fulldoc);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("If checked GTM will output a full HTML document "
                             "with <HTML>, <BODY>, etc. tags instead of just "
                             "the table html."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gtmvals.fulldoc);

  gtk_widget_show (main_vbox);
  gtk_widget_show (frame);

  /* HTML Table Creation Options */
  frame = gimp_frame_new (_("Table Creation Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);

  toggle = gtk_check_button_new_with_mnemonic (_("_Use cellspan"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 0, 2, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gtmvals.spantags);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("If checked GTM will replace any rectangular "
                             "sections of identically colored blocks with one "
                             "large cell with ROWSPAN and COLSPAN values."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gtmvals.spantags);

  toggle = gtk_check_button_new_with_mnemonic (_("Co_mpress TD tags"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 1, 2, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gtmvals.tdcomp);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("Checking this tag will cause GTM to leave no "
                             "whitespace between the TD tags and the "
                             "cellcontent.  This is only necessary for pixel "
                             "level positioning control."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gtmvals.tdcomp);

  toggle = gtk_check_button_new_with_mnemonic (_("C_aption"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 2, 1, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gtmvals.caption);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("Check if you would like to have the table "
                             "captioned."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gtmvals.caption);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.captiontxt);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
  gtk_widget_show (entry);

  gimp_help_set_help_data (entry, _("The text for the table caption."), NULL);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_changed_callback),
                    gtmvals.captiontxt);

  g_object_bind_property (toggle, "active",
                          entry,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.cellcontent);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("C_ell content:"), 0.0, 0.5,
                            entry, 1);
  gtk_widget_show (entry);

  gimp_help_set_help_data (entry, _("The text to go into each cell."), NULL);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_changed_callback),
                    gtmvals.cellcontent);

  gtk_widget_show (grid);
  gtk_widget_show (frame);

  /* HTML Table Options */
  frame = gimp_frame_new (_("Table Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);

  adj = (GtkAdjustment *) gtk_adjustment_new (gtmvals.border,
                                              0, 1000, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Border:"), 0.0, 0.5,
                            spinbutton, 1);

  gimp_help_set_help_data (spinbutton,
                           _("The number of pixels in the table border."),
                           NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &gtmvals.border);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 60, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.clwidth);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Width:"), 0.0, 0.5,
                            entry, 1);

  gimp_help_set_help_data (entry,
                           _("The width for each table cell.  "
                             "Can be a number or a percent."),
                           NULL);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_changed_callback),
                    gtmvals.clwidth);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 60, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.clheight);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Height:"), 0.0, 0.5,
                            entry, 1);

  gimp_help_set_help_data (entry,
                           _("The height for each table cell.  "
                             "Can be a number or a percent."),
                           NULL);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_changed_callback),
                    gtmvals.clheight);

  adj = (GtkAdjustment *) gtk_adjustment_new (gtmvals.cellpadding,
                                              0, 1000, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("Cell-_padding:"), 0.0, 0.5,
                            spinbutton, 1);

  gimp_help_set_help_data (spinbutton,
                           _("The amount of cell padding."), NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &gtmvals.cellpadding);

  adj = (GtkAdjustment *) gtk_adjustment_new (gtmvals.cellspacing,
                                              0, 1000, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Cell-_spacing:"), 0.0, 0.5,
                            spinbutton, 1);

  gimp_help_set_help_data (spinbutton,
                           _("The amount of cell spacing."), NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &gtmvals.cellspacing);

  gtk_widget_show (grid);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

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

/*  Export interface functions  */

static void
entry_changed_callback (GtkEntry *entry,
                        gchar    *string)
{
  strncpy (string, gtk_entry_get_text (entry), 255);
}
