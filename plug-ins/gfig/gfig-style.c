/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "gfig.h"
#include "gfig-dobject.h"
#include "gfig-style.h"


static void gfig_read_parameter_string   (gchar       **text,
                                          gint          nitems,
                                          const gchar  *name,
                                          gchar       **style_entry);

static void gfig_read_parameter_int      (gchar       **text,
                                          gint          nitems,
                                          const gchar  *name,
                                          gint         *style_entry);

static void gfig_read_parameter_double   (gchar        **text,
                                          gint           nitems,
                                          const gchar   *name,
                                          gdouble       *style_entry);

static void gfig_read_parameter_gimp_rgb (gchar        **text,
                                          gint           nitems,
                                          const gchar   *name,
                                          GimpRGB       *style_entry);

static void
gfig_read_parameter_string (gchar       **text,
                            gint          nitems,
                            const gchar  *name,
                            gchar        **style_entry)
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
gfig_read_parameter_int (gchar       **text,
                         gint          nitems,
                         const gchar  *name,
                         gint         *style_entry)
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
gfig_read_parameter_double (gchar        **text,
                            gint           nitems,
                            const gchar   *name,
                            gdouble       *style_entry)
{
  gint   n = 0;
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
              *style_entry = g_ascii_strtod (g_strchug (ptr), &endptr);
              g_free (tmpstr);
              return;
            }
          g_free (tmpstr);
        }
      ++n;
    }
}

static void
gfig_read_parameter_gimp_rgb (gchar        **text,
                              gint           nitems,
                              const gchar   *name,
                              GimpRGB       *style_entry)
{
  gint   n = 0;
  gchar *ptr;
  gchar *tmpstr;
  gchar *endptr;
  gchar  fmt_str[32];
  gchar  colorstr_r[G_ASCII_DTOSTR_BUF_SIZE];
  gchar  colorstr_g[G_ASCII_DTOSTR_BUF_SIZE];
  gchar  colorstr_b[G_ASCII_DTOSTR_BUF_SIZE];
  gchar  colorstr_a[G_ASCII_DTOSTR_BUF_SIZE];

  style_entry->r = style_entry->g = style_entry->b = style_entry->a = 0.;

  snprintf (fmt_str, sizeof (fmt_str),
            "%%%" G_GSIZE_FORMAT "s"
            " %%%" G_GSIZE_FORMAT "s"
            " %%%" G_GSIZE_FORMAT "s"
            " %%%" G_GSIZE_FORMAT "s",
            sizeof (colorstr_r) - 1, sizeof (colorstr_g) - 1,
            sizeof (colorstr_b) - 1, sizeof (colorstr_a) - 1);

  while (n < nitems)
    {
      ptr = strchr (text[n], ':');
      if (ptr)
        {
          tmpstr = g_strndup (text[n], ptr - text[n]);
          ptr++;
          if (!strcmp (tmpstr, name))
            {
              sscanf (ptr, fmt_str,
                      colorstr_r, colorstr_g, colorstr_b, colorstr_a);
              style_entry->r = g_ascii_strtod (colorstr_r, &endptr);
              style_entry->g = g_ascii_strtod (colorstr_g, &endptr);
              style_entry->b = g_ascii_strtod (colorstr_b, &endptr);
              style_entry->a = g_ascii_strtod (colorstr_a, &endptr);
              g_free (tmpstr);
              return;
            }
          g_free (tmpstr);
        }
      ++n;
    }
}

#define MAX_STYLE_TEXT_ENTRIES 100

gboolean
gfig_load_style (Style *style,
                 FILE  *fp)
{
  gulong  offset;
  gchar   load_buf2[MAX_LOAD_LINE];
  gchar  *style_text[MAX_STYLE_TEXT_ENTRIES];
  gint    nitems = 0;
  gint    value;
  gint    k;
  gchar   name[100];

  offset = ftell (fp);

  get_line (load_buf2, MAX_LOAD_LINE, fp, 0);
  /* nuke final > and preserve spaces in name */
  if (1 != sscanf (load_buf2, "<Style %99[^>]>", name))
    {
      /* no style data, copy default style and fail silently */
      gfig_style_copy (style, &gfig_context->default_style, "default style");
      fseek (fp, offset, SEEK_SET);
      return TRUE;
    }

  if (gfig_context->debug_styles)
    g_printerr ("Loading style '%s' -- ", name);

  style->name = g_strdup (name);

  while (TRUE)
    {
      get_line (load_buf2, MAX_LOAD_LINE, fp, 0);
      if (!strcmp (load_buf2, "</Style>") || feof (fp))
        break;

      style_text[nitems] = g_strdup (load_buf2);
      nitems++;

      if (nitems >= MAX_STYLE_TEXT_ENTRIES)
        break;
    }

  if (feof (fp) || (nitems >= MAX_STYLE_TEXT_ENTRIES))
    {
      g_message ("Error reading style data");
      return TRUE;
    }

  gfig_read_parameter_string (style_text, nitems, "BrushName",
                              &style->brush_name);

  if (style->brush_name == NULL)
    g_message ("Error loading style: got NULL for brush name.");

  gfig_read_parameter_string (style_text, nitems, "Pattern", &style->pattern);
  gfig_read_parameter_string (style_text, nitems, "Gradient", &style->gradient);

  gfig_read_parameter_gimp_rgb (style_text, nitems, "Foreground",
                                &style->foreground);
  gfig_read_parameter_gimp_rgb (style_text, nitems, "Background",
                                &style->background);

  gfig_read_parameter_int (style_text, nitems, "FillType", &value);
  style->fill_type = value;

  gfig_read_parameter_int (style_text, nitems, "PaintType", &value);
  style->paint_type = value;

  gfig_read_parameter_double (style_text, nitems, "FillOpacity",
                              &style->fill_opacity);

  for (k = 0; k < nitems; k++)
    {
      g_free (style_text[k]);
    }

  if (gfig_context->debug_styles)
    g_printerr ("done\n");

  return FALSE;
}


gboolean
gfig_skip_style (Style *style,
                 FILE  *fp)
{
  gulong offset;
  gchar  load_buf2[MAX_LOAD_LINE];

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
                  FILE    *fp)
{
  if (gfig_context->debug_styles)
    g_printerr ("Loading global styles -- ");

  /* currently we only have the default style */
  gfig_load_style (&gfig_context->default_style, fp);

  if (gfig_context->debug_styles)
    g_printerr ("done\n");

  return FALSE;
}

void
gfig_save_style (Style   *style,
                 GString *string)
{
  gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_r[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_g[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_b[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_a[G_ASCII_DTOSTR_BUF_SIZE];
  gint  blen = G_ASCII_DTOSTR_BUF_SIZE;

  if (gfig_context->debug_styles)
    g_printerr ("Saving style %s, brush name '%s'\n", style->name, style->brush_name);

  g_string_append_printf (string, "<Style %s>\n", style->name);
  g_string_append_printf (string, "BrushName:      %s\n",          style->brush_name);
  if (!style->brush_name)
    g_message ("Error saving style %s: saving NULL for brush name", style->name);

  g_string_append_printf (string, "PaintType:       %d\n",          style->paint_type);

  g_string_append_printf (string, "FillType:       %d\n",          style->fill_type);

  g_string_append_printf (string, "FillOpacity:    %s\n",
                          g_ascii_dtostr (buffer, blen, style->fill_opacity));

  g_string_append_printf (string, "Pattern:        %s\n",          style->pattern);

  g_string_append_printf (string, "Gradient:       %s\n",          style->gradient);

  g_string_append_printf (string, "Foreground: %s %s %s %s\n",
                          g_ascii_dtostr (buffer_r, blen, style->foreground.r),
                          g_ascii_dtostr (buffer_g, blen, style->foreground.g),
                          g_ascii_dtostr (buffer_b, blen, style->foreground.b),
                          g_ascii_dtostr (buffer_a, blen, style->foreground.a));

  g_string_append_printf (string, "Background: %s %s %s %s\n",
                          g_ascii_dtostr (buffer_r, blen, style->background.r),
                          g_ascii_dtostr (buffer_g, blen, style->background.g),
                          g_ascii_dtostr (buffer_b, blen, style->background.b),
                          g_ascii_dtostr (buffer_a, blen, style->background.a));

  g_string_append_printf (string, "</Style>\n");
}

void
gfig_style_save_as_attributes (Style   *style,
                               GString *string)
{
  gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_r[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_g[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_b[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buffer_a[G_ASCII_DTOSTR_BUF_SIZE];
  gint  blen = G_ASCII_DTOSTR_BUF_SIZE;

  if (gfig_context->debug_styles)
    g_printerr ("Saving style %s as attributes\n", style->name);
  g_string_append_printf (string, "BrushName=\"%s\" ",  style->brush_name);

  g_string_append_printf (string, "Foreground=\"%s %s %s %s\" ",
                          g_ascii_dtostr (buffer_r, blen, style->foreground.r),
                          g_ascii_dtostr (buffer_g, blen, style->foreground.g),
                          g_ascii_dtostr (buffer_b, blen, style->foreground.b),
                          g_ascii_dtostr (buffer_a, blen, style->foreground.a));

  g_string_append_printf (string, "Background=\"%s %s %s %s\" ",
                          g_ascii_dtostr (buffer_r, blen, style->background.r),
                          g_ascii_dtostr (buffer_g, blen, style->background.g),
                          g_ascii_dtostr (buffer_b, blen, style->background.b),
                          g_ascii_dtostr (buffer_a, blen, style->background.a));

  g_string_append_printf (string, "FillType=%d ", style->fill_type);

  g_string_append_printf (string, "PaintType=%d ", style->paint_type);

  g_string_append_printf (string, "FillOpacity=%s ",
                          g_ascii_dtostr (buffer, blen, style->fill_opacity));


}

void
gfig_save_styles (GString *string)
{
  if (gfig_context->debug_styles)
    g_printerr ("Saving global styles.\n");

  gfig_save_style (&gfig_context->default_style, string);
}

/*
 * set_foreground_callback() is the callback for the Foreground color select
 * widget.  It reads the color from the widget, and applies this color to the
 * current style.  It then produces a repaint (which will be suppressed if
 * gfig_context->enable_repaint is FALSE).
 */
void
set_foreground_callback (GimpColorButton *button,
                         gpointer         data)
{
  GimpRGB  color2;
  Style   *current_style;

  if (gfig_context->debug_styles)
    g_printerr ("Setting foreground color from color selector\n");

  current_style = gfig_context_get_current_style ();

  gimp_color_button_get_color (button, &color2);

  gimp_rgba_set (&current_style->foreground,
                 color2.r, color2.g, color2.b, color2.a);

  gfig_paint_callback ();
}

void
set_background_callback (GimpColorButton *button,
                         gpointer         data)
{
  GimpRGB  color2;
  Style   *current_style;

  if (gfig_context->debug_styles)
    g_printerr ("Setting background color from color selector\n");

  current_style = gfig_context_get_current_style ();

  gimp_color_button_get_color (button, &color2);
  gimp_rgba_set (&current_style->background,
                 color2.r, color2.g, color2.b, color2.a);

  gfig_paint_callback ();
}

void
set_paint_type_callback (GtkToggleButton *toggle,
                         gpointer         data)
{
  gboolean  paint_type;
  Style    *current_style;

  current_style = gfig_context_get_current_style ();
  paint_type = gtk_toggle_button_get_active (toggle);
  current_style->paint_type = paint_type;
  gfig_paint_callback ();

  gtk_widget_set_sensitive (GTK_WIDGET (data), paint_type);
}

/*
 * gfig_brush_changed_callback() is the callback for the brush
 * selector widget.  It reads the brush name from the widget, and
 * applies this to the current style, as well as the gfig_context->bdesc
 * values.  It then produces a repaint (which will be suppressed if
 * gfig_context->enable_repaint is FALSE).
 */
void
gfig_brush_changed_callback (GimpBrushSelectButton *button,
                             const gchar           *brush_name,
                             gdouble                opacity,
                             gint                   spacing,
                             GimpLayerMode          paint_mode,
                             gint                   width,
                             gint                   height,
                             const guchar          *mask_data,
                             gboolean               dialog_closing,
                             gpointer               user_data)
{
  Style *current_style;

  current_style = gfig_context_get_current_style ();
  current_style->brush_name = g_strdup (brush_name);

  /* this will soon be unneeded. How soon? */
  gfig_context->bdesc.name = g_strdup (brush_name);
  gfig_context->bdesc.width = width;
  gfig_context->bdesc.height = height;
  gimp_context_set_brush (brush_name);
  gimp_context_set_brush_default_size ();

  gfig_paint_callback ();
}

void
gfig_pattern_changed_callback (GimpPatternSelectButton *button,
                               const gchar             *pattern_name,
                               gint                     width,
                               gint                     height,
                               gint                     bpp,
                               const guchar            *mask_data,
                               gboolean                 dialog_closing,
                               gpointer                 user_data)
{
  Style *current_style;

  current_style = gfig_context_get_current_style ();
  current_style->pattern = g_strdup (pattern_name);

  gfig_paint_callback ();
}

void
gfig_gradient_changed_callback (GimpGradientSelectButton *button,
                                const gchar              *gradient_name,
                                gint                      width,
                                const gdouble            *grad_data,
                                gboolean                  dialog_closing,
                                gpointer                  user_data)
{
  Style *current_style;

  current_style = gfig_context_get_current_style ();
  current_style->gradient = g_strdup (gradient_name);

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
gfig_style_copy (Style       *style1,
                 Style       *style0,
                 const gchar *name)
{
  if (name)
    style1->name = g_strdup (name);
  else
    g_message ("Error: name is NULL in gfig_style_copy.");

  if (gfig_context->debug_styles)
    g_printerr ("Copying style %s as style %s\n", style0->name, name);

  gfig_rgba_copy (&style1->foreground, &style0->foreground);
  gfig_rgba_copy (&style1->background, &style0->background);

  if (!style0->brush_name)
    g_message ("Error copying style %s: brush name is NULL.", style0->name);

  style1->brush_name    = g_strdup (style0->brush_name);
  style1->gradient      = g_strdup (style0->gradient);
  style1->pattern       = g_strdup (style0->pattern);
  style1->fill_type    = style0->fill_type;
  style1->fill_opacity = style0->fill_opacity;
  style1->paint_type   = style0->paint_type;
}

/*
 * gfig_style_apply() applies the settings from the specified style to
 * the GIMP core.  It does not change any widgets, and does not cause
 * a repaint.
 */
void
gfig_style_apply (Style *style)
{
  if (gfig_context->debug_styles)
    g_printerr ("Applying style '%s' -- ", style->name);

  gimp_context_set_foreground (&style->foreground);

  gimp_context_set_background (&style->background);

  if (! gimp_context_set_brush (style->brush_name))
    g_message ("Style apply: Failed to set brush to '%s' in style '%s'",
               style->brush_name, style->name);

  gimp_context_set_brush_default_size ();

  gimp_context_set_pattern (style->pattern);

  gimp_context_set_gradient (style->gradient);

  if (gfig_context->debug_styles)
    g_printerr ("done.\n");
}

/*
 * gfig_read_gimp_style() reads the style settings from the Gimp core,
 * and applies them to the specified style, giving that style the
 * specified name.  This is mainly useful as a way of initializing
 * a style.  The function does not cause a repaint.
 */
void
gfig_read_gimp_style (Style       *style,
                      const gchar *name)
{
  gint dummy;

  if (!name)
    g_message ("Error: name is NULL in gfig_read_gimp_style.");

  if (gfig_context->debug_styles)
    g_printerr ("Reading Gimp settings as style %s\n", name);
  style->name = g_strdup (name);

  gimp_context_get_foreground (&style->foreground);
  gimp_context_get_background (&style->background);

  style->brush_name = gimp_context_get_brush ();
  gimp_brush_get_info (style->brush_name,
                       &style->brush_width, &style->brush_height,
                       &dummy, &dummy);
  gimp_brush_get_spacing (style->brush_name, &style->brush_spacing);

  style->gradient = gimp_context_get_gradient ();
  style->pattern  = gimp_context_get_pattern ();

  style->fill_opacity = 100.;

  gfig_context->bdesc.name   = style->brush_name;
  gfig_context->bdesc.width  = style->brush_width;
  gfig_context->bdesc.height = style->brush_height;
}

/*
 * gfig_style_set_content_from_style() sets all of the style control widgets
 * to values from the specified style.  This in turn sets the Gimp core's
 * values to the same things.  Repainting is suppressed while this happens,
 * so calling this function will not produce a repaint.
 *
 */
void
gfig_style_set_context_from_style (Style *style)
{
  gboolean enable_repaint;

  if (gfig_context->debug_styles)
    g_printerr ("Setting context from style '%s' -- ", style->name);

  enable_repaint = gfig_context->enable_repaint;
  gfig_context->enable_repaint = FALSE;

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (gfig_context->fg_color_button),
                               &style->foreground);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (gfig_context->bg_color_button),
                               &style->background);
  if (! gimp_context_set_brush (style->brush_name))
    g_message ("Style from context: Failed to set brush to '%s'",
               style->brush_name);

  gimp_context_set_brush_default_size ();

  gimp_brush_select_button_set_brush (GIMP_BRUSH_SELECT_BUTTON (gfig_context->brush_select),
                                      style->brush_name, -1.0, -1, -1);  /* FIXME */

  gimp_pattern_select_button_set_pattern (GIMP_PATTERN_SELECT_BUTTON (gfig_context->pattern_select),
                                          style->pattern);

  gimp_gradient_select_button_set_gradient (GIMP_GRADIENT_SELECT_BUTTON (gfig_context->gradient_select),
                                            style->gradient);

  gfig_context->bdesc.name = style->brush_name;
  if (gfig_context->debug_styles)
    g_printerr ("done.\n");

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (gfig_context->fillstyle_combo),
                                 (gint) style->fill_type);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfig_context->paint_type_toggle),
                                style->paint_type);
  gfig_context->enable_repaint = enable_repaint;
}

/*
 * gfig_style_set_style_from_context() sets the values in the specified
 * style to those that appear in the style control widgets f
 */
void
gfig_style_set_style_from_context (Style *style)
{
  Style   *current_style;
  GimpRGB  color;
  gint     value;

  style->name = "object";
  current_style = gfig_context_get_current_style ();

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (gfig_context->fg_color_button),
                               &color);
  if (gfig_context->debug_styles)
    g_printerr ("Setting foreground color to %lg %lg %lg\n",
                color.r, color.g, color.b);

  gfig_rgba_copy (&style->foreground, &color);
  gimp_color_button_get_color (GIMP_COLOR_BUTTON (gfig_context->bg_color_button),
                               &color);
  gfig_rgba_copy (&style->background, &color);

  style->brush_name = current_style->brush_name;

  if (!style->pattern || strcmp (style->pattern, current_style->pattern))
    {
      style->pattern = g_strdup (current_style->pattern); /* why strduping? */
    }

  style->gradient = current_style->gradient;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (gfig_context->fillstyle_combo), &value))
    style->fill_type = value;

  /* FIXME when there is an opacity control widget to read */
  style->fill_opacity = 100.;

  style->paint_type = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gfig_context->paint_type_toggle));
}

void
mygimp_brush_info (gint *width,
                   gint *height)
{
  gchar *name = gimp_context_get_brush ();
  gint   dummy;

  if (name && gimp_brush_get_info (name, width, height, &dummy, &dummy))
    {
      *width  = MAX (*width, 32);
      *height = MAX (*height, 32);
    }
  else
    {
      g_message ("Failed to get brush info");
      *width = *height = 48;
    }

  g_free (name);
}

Style *
gfig_context_get_current_style (void)
{
  if (gfig_context->selected_obj)
    return &gfig_context->selected_obj->style;
  else
    return &gfig_context->default_style;
}
