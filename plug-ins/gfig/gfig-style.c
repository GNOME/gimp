#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#  include <io.h>
#  ifndef W_OK
#    define W_OK 2
#  endif
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "libgimp/stdplugins-intl.h"

#include "gfig.h"
#include "gfig-style.h"

static void gfig_read_parameter_string (gchar **text,
                                        gint    nitems,
                                        gchar  *name,
                                        gchar  **style_entry);


static void gfig_read_parameter_int    (gchar **text,
                                        gint    nitems,
                                        gchar  *name,
                                        gint   *style_entry);

static void gfig_read_parameter_double (gchar  **text,
                                        gint     nitems,
                                        gchar   *name,
                                        gdouble *style_entry);

static void gfig_read_parameter_gimp_rgb (gchar  **text,
                                          gint     nitems,
                                          gchar   *name,
                                          GimpRGB *style_entry);

static void
gfig_read_parameter_string (gchar **text,
                            gint    nitems,
                            gchar  *name,
                            gchar  **style_entry)
{
  gint  n = 0;
  gchar *ptr;
  gchar *tmpstr;

  *style_entry = NULL;

  while (n < nitems)
    {
      ptr = strchr (text[n], ':');
      if (ptr)
        {
          tmpstr = g_strndup (text[n], ptr - text[n]);
          ptr++;
          if (!strcmp (tmpstr, name))
            {
              *style_entry = g_strdup (g_strchug (ptr));
              g_free (tmpstr);
              return;
            }
          g_free (tmpstr);
        }
      ++n;
    }

  g_message ("Parameter '%s' not found", name);
}


static void
gfig_read_parameter_int (gchar **text,
                         gint    nitems,
                         gchar  *name,
                         gint   *style_entry)
{
  gint  n = 0;
  gchar *ptr;
  gchar *tmpstr;

  *style_entry = 0;

  while (n < nitems)
    {
      ptr = strchr (text[n], ':');
      if (ptr)
        {
          tmpstr = g_strndup (text[n], ptr - text[n]);
          ptr++;
          if (!strcmp (tmpstr, name))
            {
              *style_entry = atoi (g_strchug (ptr));
              g_free (tmpstr);
              return;
            }
          g_free (tmpstr);
        }
      ++n;
    }
}

static void
gfig_read_parameter_double (gchar  **text,
                            gint     nitems,
                            gchar   *name,
                            gdouble *style_entry)
{
  gint  n = 0;
  gchar *ptr;
  gchar *endptr;
  gchar *tmpstr;

  *style_entry = 0.;

  while (n < nitems)
    {
      ptr = strchr (text[n], ':');
      if (ptr)
        {
          tmpstr = g_strndup (text[n], ptr - text[n]);
          ptr++;
          if (!strcmp (tmpstr, name))
            {
              *style_entry = g_strtod (g_strchug (ptr), &endptr);
              g_free (tmpstr);
              return;
            }
          g_free (tmpstr);
        }
      ++n;
    }
}

static void
gfig_read_parameter_gimp_rgb (gchar  **text,
                              gint     nitems,
                              gchar   *name,
                              GimpRGB *style_entry)
{
  gint  n = 0;
  gchar *ptr;
  gchar *tmpstr;

  style_entry->r = style_entry->g = style_entry->b = style_entry->a = 0.;

  while (n < nitems)
    {
      ptr = strchr (text[n], ':');
      if (ptr)
        {
          tmpstr = g_strndup (text[n], ptr - text[n]);
          ptr++;
          if (!strcmp (tmpstr, name))
            {
              sscanf (ptr, "%lf %lf %lf %lf", &style_entry->r, &style_entry->g,
                      &style_entry->b, &style_entry->a);
              g_free (tmpstr);
              return;
            }
          g_free (tmpstr);
        }
      ++n;
    }
}



gboolean 
gfig_load_style (Style *style,
                 FILE    *fp)
{
  gulong offset;
  gchar load_buf2[MAX_LOAD_LINE];
  gchar *style_text[100];
  gint  nitems = 0;
  gint  k;
  gchar name[100];
  offset = ftell (fp);

  get_line (load_buf2, MAX_LOAD_LINE, fp, 0);
  if (1 != sscanf (load_buf2, "<Style %s>", name))
    {
      /* no style data */
      fprintf (stderr, "No style data\n");
      fseek (fp, offset, SEEK_SET);
      return TRUE;
    }

  if (gfig_context->debug_styles)
    fprintf (stderr, "Loading style '%s' -- ", name);

  /* nuke final > in name */
  *strrchr (name, '>') = '\0';

  style->name = g_strdup (name);

  while (TRUE)
    {
      get_line (load_buf2, MAX_LOAD_LINE, fp, 0);
      if (!strcmp (load_buf2, "</Style>") || feof (fp))
        break;

      style_text[nitems] = (gchar *) g_malloc (MAX_LOAD_LINE);
      strcpy (style_text[nitems], load_buf2);
      nitems++;
    }

  if (feof (fp))
    {
      g_message ("Error reading style data");
      return TRUE;
    }

  gfig_read_parameter_string   (style_text, nitems, "BrushName",  &style->brush_name);
  gfig_read_parameter_string   (style_text, nitems, "Pattern",    &style->pattern);
  gfig_read_parameter_string   (style_text, nitems, "Gradient",   &style->gradient);
  gfig_read_parameter_gimp_rgb (style_text, nitems, "Foreground", &style->foreground);
  gfig_read_parameter_gimp_rgb (style_text, nitems, "Background", &style->background);

  for (k = 0; k < nitems; k++)
    {
      g_free (style_text[k]);
    }

  if (gfig_context->debug_styles)
    fprintf (stderr, "done\n");

  return FALSE;
}


gboolean 
gfig_skip_style (Style *style,
                 FILE    *fp)
{
  gulong offset;
  gchar load_buf2[MAX_LOAD_LINE];

  offset = ftell (fp);

  get_line (load_buf2, MAX_LOAD_LINE, fp, 0);
  if (strncmp (load_buf2, "<Style ", 7))
    {
      /* no style data */
      fseek (fp, offset, SEEK_SET);
      return TRUE;
    }

  while (TRUE)
    {
      get_line (load_buf2, MAX_LOAD_LINE, fp, 0);
      if (!strcmp (load_buf2, "</Style>") || feof (fp))
        break;
    }

  if (feof (fp))
    {
      g_message ("Error trying to skip style data");
      return TRUE;
    }

  return FALSE;
}

/*
 * FIXME: need to make this load a list of styles if there are more than one.
 */
gboolean
gfig_load_styles (GFigObj *gfig,
                  FILE *fp)
{
  if (gfig_context->debug_styles)
    fprintf (stderr, "Loading global styles -- ");

  /* currently we only have the default style */
  gfig_load_style (&gfig_context->default_style, fp);

  if (gfig_context->debug_styles)
    fprintf (stderr, "done\n");

  return FALSE;
}

void 
gfig_save_style (Style *style, 
                 GString *string)
{
  g_string_append_printf (string, "<Style %s>\n", style->name);
  g_string_append_printf (string, "BrushName:      %s\n",          style->brush_name);
  g_string_append_printf (string, "BrushSource:    %d\n",          style->brush_source);
  g_string_append_printf (string, "FillType:       %d\n",          style->fill_type);
  g_string_append_printf (string, "FillTypeSource: %d\n",          style->fill_type_source);
  g_string_append_printf (string, "Pattern:        %s\n",          style->pattern);
  g_string_append_printf (string, "PatternSource:  %d\n",          style->pattern_source);
  g_string_append_printf (string, "Gradient:       %s\n",          style->gradient);
  g_string_append_printf (string, "GradientSource: %d\n",          style->gradient_source);
  g_string_append_printf (string, "Foreground: %lg %lg %lg %lg\n", style->foreground.r,
                          style->foreground.g, style->foreground.b, style->foreground.a);
  g_string_append_printf (string, "ForegroundSource: %d\n",  style->background_source);
  g_string_append_printf (string, "Background: %lg %lg %lg %lg\n", style->background.r,
                          style->background.g, style->background.b, style->background.a);
  g_string_append_printf (string, "BackgroundSource: %d\n",        style->background_source);
  g_string_append_printf (string, "</Style>\n");
}

void
gfig_save_styles (GString *string)
{
  gint k;

  for (k = 1; k < gfig_context->num_styles; k++) 
    gfig_save_style (gfig_context->style[k], string);
}

void
set_foreground_callback (GimpColorButton *button,
                         gpointer         data)
{
  GimpRGB color2;

  gimp_color_button_get_color (button, &color2);
  gimp_rgba_set (&gfig_context->default_style.foreground,
                 color2.r, color2.g, color2.b, color2.a);

  gimp_rgba_set (&gfig_context->current_style->foreground,
                 color2.r, color2.g, color2.b, color2.a);

  gfig_paint_callback ();
}

void
set_background_callback (GimpColorButton *button,
                         gpointer         data)
{
  GimpRGB color2;

  gimp_color_button_get_color (button, &color2);
  gimp_rgba_set (&gfig_context->default_style.background,
                 color2.r, color2.g, color2.b, color2.a);
  gimp_rgba_set (&gfig_context->current_style->background,
                 color2.r, color2.g, color2.b, color2.a);

  gfig_paint_callback ();
}

void 
gfig_brush_changed_callback (const gchar *brush_name,
                             gdouble opacity,
                             gint spacing,
                             GimpLayerModeEffects paint_mode,
                             gint width,
                             gint height,
                             const guchar *mask_data,
                             gboolean dialog_closing,
                             gpointer user_data)
{
  gfig_context->current_style->brush_name = (gchar *) brush_name;
  gfig_context->default_style.brush_name = (gchar *) brush_name;

  /* this will soon be unneeded */
  gfig_context->bdesc.name = (gchar *) brush_name;
  gfig_context->bdesc.width = width;
  gfig_context->bdesc.height = height;
  gimp_brushes_set_brush (brush_name);

  gfig_paint_callback ();
}

void 
gfig_pattern_changed_callback (const gchar *pattern_name,
                               gint width,
                               gint height,
                               gint bpp,
                               const guchar *mask_data,
                               gboolean dialog_closing,
                               gpointer user_data)
{
  gfig_context->current_style->pattern = (gchar *) pattern_name;
  gfig_context->default_style.pattern = (gchar *) pattern_name;

  gfig_paint_callback ();
}

void 
gfig_gradient_changed_callback (const gchar *gradient_name,
                                gint width,
                                const gdouble *grad_data,
                                gboolean dialog_closing,
                                gpointer user_data)
{
  gfig_context->current_style->gradient = (gchar *) gradient_name;
  gfig_context->default_style.gradient = (gchar *) gradient_name;

  gfig_paint_callback ();
}

void
gfig_rgba_copy (GimpRGB *color1,
                GimpRGB *color2)
{
  color1->r = color2->r;
  color1->g = color2->g;
  color1->b = color2->b;
  color1->a = color2->a;
}

void 
gfig_style_copy (Style *style1, 
                 Style *style0,
                 const gchar *name)
{
  if (name)
    style1->name = g_strdup (name);

  gfig_rgba_copy (&style1->foreground, &style0->foreground);
  gfig_rgba_copy (&style1->background, &style0->background);

  style1->brush_name = g_strdup (style0->brush_name);

  style1->gradient = g_strdup (style0->gradient);

  style1->pattern = g_strdup (style0->pattern);
}

void
gfig_style_apply (Style *style)
{
  if (gfig_context->debug_styles)
    fprintf (stderr, "Applying style '%s' -- ", style->name);
  gimp_palette_set_foreground (&style->foreground);
  gimp_palette_set_background (&style->background);
  if (!gimp_brushes_set_brush (style->brush_name))
    g_message ("Style apply: Failed to set brush to '%s' in style '%s'", 
               style->brush_name, style->name);
  if (gfig_context->debug_styles)
    fprintf (stderr, "done.\n");
}

void
gfig_style_set_all_sources (Style *style,
                            StyleSource source)
{
  style->foreground_source = source;
  style->background_source = source;
  style->brush_source = source;
  style->gradient_source = source;
  style->pattern_source = source;

}

void
gfig_style_append (Style *style)
{
  gfig_context->style[gfig_context->num_styles] = style;
  ++gfig_context->num_styles;
}

void
gfig_read_gimp_style (Style *style,
                      const gchar *name)
{
  gint w, h;

  style->name = g_strdup (name);

  gimp_palette_get_foreground (&style->foreground);
  gimp_palette_get_background (&style->background);
  style->brush_name = (gchar *) gimp_brushes_get_brush (&style->brush_width,
                                                        &style->brush_height,
                                                        &style->brush_spacing);
  style->gradient = gimp_gradients_get_gradient ();
  style->pattern = gimp_patterns_get_pattern (&w, &h);
}

void
gfig_style_set_context_from_style (Style *style)
{
  if (gfig_context->debug_styles)
    fprintf (stderr, "Setting context from style '%s' -- ", style->name);

  gfig_context->current_style = style;

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (gfig_context->fg_color_button),
                               &style->foreground);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (gfig_context->bg_color_button),
                               &style->background);
  if (!gimp_brushes_set_brush (style->brush_name))
    g_message ("Style from context: Failed to set brush to '%s'", style->brush_name);

  gimp_brush_select_widget_set (gfig_context->brush_select,
                                style->brush_name,
                                -1., -1, -1);  /* FIXME */

  gfig_context->bdesc.name = style->brush_name;
  if (gfig_context->debug_styles)
    fprintf (stderr, "done.\n");

}


void
gfig_style_set_style_from_context (Style *style)
{
  GimpRGB color;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (gfig_context->fg_color_button),
                               &color);
  gfig_rgba_copy (&style->foreground, &color);
  gimp_color_button_get_color (GIMP_COLOR_BUTTON (gfig_context->bg_color_button),
                               &color);
  gfig_rgba_copy (&style->background, &color);

  style->brush_name = gfig_context->current_style->brush_name;
  style->pattern = gfig_context->current_style->pattern;
  style->gradient = gfig_context->current_style->gradient;
}

char *
mygimp_brush_get (void)
{
  gint width, height, spacing;

  return gimp_brushes_get_brush (&width, &height, &spacing);
}

void
mygimp_brush_info (gint *width,
                   gint *height)
{
  char *name;
  gint spacing;

  name = gimp_brushes_get_brush (width, height, &spacing);
  if (name)
    {
      *width  = MAX (*width, 32);
      *height = MAX (*height, 32);
      g_free (name);
    }
  else
    {
      g_message ("Failed to get brush info");
      *width = *height = 48;
    }
}

