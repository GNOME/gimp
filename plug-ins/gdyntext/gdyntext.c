/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999 Marco Lamberto <lm@geocities.com>
 * Web page: http://www.geocities.com/Tokyo/1474/gimp/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <libgimp/gimp.h>
#include "gdyntext.h"
#include "gdyntext_ui.h"


char *strunescape(char *text);
char *strescape(char *text);
GArray *params_split(const char *text);
GArray *text_split(const char *text);
static void gdt_query(void);
static void gdt_run(char *name, int nparams, GParam *param, int *nreturn_vals,
	GParam **return_vals);
void gdt_get_values(GdtVals *data);
void gdt_set_values(GdtVals *data);
void gdt_render_text(GdtVals *data);


#define GDT_MAGIC_REV(lname)	(atoi(lname + 3))
#define GDT_REV()							((int)(atof(GDYNTEXT_VERSION) * 10))


GPlugInInfo PLUG_IN_INFO = 
{
	NULL,
	NULL,
	gdt_query,
	gdt_run,
};

typedef enum
{
	TEXT				= 0,
	FONT_FAMILY	= 1,
	FONT_SIZE		= 2,
	FONT_SIZE_T	= 3,
	FONT_COLOR	= 4,
	ANTIALIAS		= 5,
	ALIGNMENT		= 6,
	ROTATION		= 7,
	FONT_STYLE	= 8,
	SPACING			= 9,
} GDTBlock;


GdtVals gdtvals;


#ifndef DEBUG_UI
MAIN()
#endif


/* substitution of '\' escaped sequences */
char *strunescape(char *text)
{
	char *str;

	str = text;
	while ((str = strstr(str, "\\"))) {
		memcpy(str, str + 1, strlen(str) + 1);
		/* escape sequences recognition */
		switch (*str) {
			case 'n': *str = '\n'; break;
			case 't': *str = '\t'; break;
		}
		str++;
	}
	return text;
}


/* escapes by prepending a '\' to the following chars: '{' '}' '\n' '\t' '\' */
char *strescape(char *text)
{
	char *str, *str_esc, buff[MAX_TEXT_SIZE];

	strncpy(buff, text, sizeof(buff));
	for (str = text, str_esc = buff; *str; str++, str_esc++) {
		switch (*str) {
			case '{':
			case '}':
			case '\\':
			case '\t':
			case '\n':
				strcpy(str_esc + 1, str);
				*str_esc = '\\';
				str_esc++;
				switch (*str) {
					case '\t': *str_esc = 't'; break;
					case '\n': *str_esc = 'n'; break;
				}
		}
	}
	strncpy(text, buff, sizeof(buff));
	return text;
}


/* converts string to lower case */
char *strtolower(char *text)
{
	char *c;

	for (c = text; *c; c++)
		*c = tolower(*c);
	return text;
}


/* splits the layer name for retrieving the saved parameters
 *
 * GDT PARAMS FORMAT:
 *   {param0}{param1}{...}{paramn}
 */
GArray *params_split(const char *text)
{
	GArray *params;
	gchar *tok_begin, *tok_end, *text_copy, *str;
	
#ifdef GTK_HAVE_FEATURES_1_1_0
	params = g_array_new(TRUE, TRUE, sizeof(char *));
#else
	params = g_array_new(TRUE);
#endif
	tok_begin = text_copy = g_strdup(text);
	do {
		do {
	 		tok_begin = strstr(tok_begin + 1, "{");
		} while (tok_begin && *(tok_begin - 1) == '\\');
		if (!tok_begin) break;
		tok_end = tok_begin;
		do {
 			tok_end = strstr(tok_end + 1, "}");
		} while (tok_end && *(tok_end - 1) == '\\');
		if (!tok_end) break;
		*tok_end = 0;
		tok_begin++;
		str = g_strdup(tok_begin);
#ifdef GTK_HAVE_FEATURES_1_1_0
		g_array_append_val(params, str);
#else
		g_array_append_val(params, char *, str);
#endif
		tok_begin = tok_end;
	} while (tok_end);
	g_free(text_copy);
	return params;
}


/* splits text into lines while looking for \n */
GArray *text_split(const char *text)
{
	return strsplit(text, '\n');
}


GArray *strsplit(const char *text, const char sep)
{
	GArray *text_lines;
	char *tok_begin, *tok_end, *text_copy, *str;

#ifdef GTK_HAVE_FEATURES_1_1_0
	text_lines = g_array_new(TRUE, TRUE, sizeof(char *));
#else
	text_lines = g_array_new(TRUE);
#endif
	tok_begin = tok_end = text_copy = g_strdup(text);
	do {
		if ((tok_end = strchr(tok_begin, sep)))
			*tok_end = 0;
		str = g_strdup(tok_begin);
#ifdef GTK_HAVE_FEATURES_1_1_0
		g_array_append_val(text_lines, str);
#else
		g_array_append_val(text_lines, char *, str);
#endif
		tok_begin = tok_end + 1;
	} while (tok_end);
	free(text_copy);
	str = NULL;
#ifdef GTK_HAVE_FEATURES_1_1_0
	g_array_append_val(text_lines, str);
#else
	g_array_append_val(text_lines, char *, str);
#endif
	return text_lines;
}


static void gdt_query(void)
{
#if defined(ENABLE_NLS) & defined(LOCALEDIR)
#ifdef HAVE_LC_MESSAGES
	setlocale(LC_MESSAGES, "");
#endif
	bindtextdomain(TEXT_DOMAIN, LOCALEDIR);
	textdomain(TEXT_DOMAIN);
#endif

	{
		static GParamDef gdt_args[] = {
			/* standard params */
			{ PARAM_INT32,		_("run_mode"),			_("Interactive, non-interactive")},
			{ PARAM_IMAGE,		_("image"),					_("Input image") },
			{ PARAM_DRAWABLE,	_("drawable"),			_("Input drawable")},
			/* gdyntext params */
			{ PARAM_STRING,		_("text"),					_("Text to render")},
			{ PARAM_STRING,		_("font_family"),		_("Font family")},
			{ PARAM_STRING,		_("font_weight"),		_("Font weight or \"*\"")},
			{ PARAM_STRING,		_("font_slant"),		_("Font slant or \"*\"")},
			{ PARAM_STRING,		_("font_spacing"),	_("Font spacing or \"*\"")},
			{ PARAM_INT32,		_("font_size"),			_("Font size")},
			{ PARAM_INT32,		_("font_metric"),		_("Font size metric: { PIXELS(0), POINTS(1) } ")},
			{ PARAM_INT32,		_("antialias"),			_("Generate antialiased text")},
			{ PARAM_INT32,		_("alignment"),			_("Text alignment: { LEFT = 0, CENTER = 1, RIGHT = 2 }")},
			{ PARAM_INT32,		_("rotation"),			_("Text rotation (degrees)")},
			{ PARAM_INT32,		_("spacing"),				_("Line spacing")},
			{ PARAM_COLOR,		_("color"),					_("Text color")},
		};
		static GParamDef gdt_rets[] = {
			{ PARAM_LAYER,		"layer",				_("The text layer")},
		};
		static int ngdt_args = sizeof(gdt_args) / sizeof(gdt_args[0]);
		static int ngdt_rets = sizeof(gdt_rets) / sizeof(gdt_rets[0]);

		gimp_install_procedure("plug_in_dynamic_text",
			"GIMP Dynamic Text",
			"",
			"Marco Lamberto <lm@geocities.com>",
			"Marco Lamberto",
			"Jan 1999",
			"<Image>/Filters/Render/Dynamic Text",
			"RGB*,GRAY*,INDEXED*",
			PROC_PLUG_IN,
			ngdt_args, ngdt_rets, gdt_args, gdt_rets);
	}
}


static void gdt_run(char *name, int nparams, GParam *param, int *nreturn_vals,
	GParam **return_vals)
{
	static GParam values[2];
	GRunModeType run_mode;
	GdtVals oldvals;

#if defined(ENABLE_NLS) & defined(LOCALEDIR)
#ifdef HAVE_LC_MESSAGES
	setlocale(LC_MESSAGES, "");
#endif
	bindtextdomain(TEXT_DOMAIN, LOCALEDIR);
	textdomain(TEXT_DOMAIN);
#endif

	gdtvals.valid							= TRUE;
#ifdef GIMP_HAVE_PARASITES
	gdtvals.change_layer_name	= FALSE;
#endif
	run_mode									= param[0].data.d_int32;
	gdtvals.image_id					= param[1].data.d_image;
	gdtvals.drawable_id				= param[2].data.d_drawable;
	gdtvals.layer_id					= param[2].data.d_layer;
	gdtvals.messages					= NULL;
	gdtvals.preview						= TRUE;
	
	*nreturn_vals = 2;
	*return_vals = values;
	
	values[0].type = PARAM_STATUS;
	values[0].data.d_status = STATUS_SUCCESS;
	values[1].type = PARAM_LAYER;
	values[1].data.d_int32 = -1;

	switch(run_mode) {
		case RUN_INTERACTIVE:
			memset(&oldvals, 0, sizeof(GdtVals));
			gimp_get_data("plug_in_gdyntext", &oldvals);
			if (oldvals.valid)
				gdtvals.preview = oldvals.preview;
			gdt_get_values(&gdtvals);
			if (!gdt_create_ui(&gdtvals)) return;
			break;
		case RUN_NONINTERACTIVE:
			/*printf("%d\n", nparams);*/
			if (nparams != 15) {
				values[0].data.d_status = STATUS_CALLING_ERROR;
				return;
			} else {
				gdtvals.new_layer	= !gimp_drawable_has_alpha(gdtvals.drawable_id);
				strncpy(gdtvals.text, param[3].data.d_string, sizeof(gdtvals.text));
				strncpy(gdtvals.font_family, param[4].data.d_string,
					sizeof(gdtvals.font_family));
				snprintf(gdtvals.font_style, sizeof(gdtvals.font_style), "%s-%s-%s",
					param[5].data.d_string,
					param[6].data.d_string,
					param[7].data.d_string);
				gdtvals.font_size = param[8].data.d_int32;
				gdtvals.font_metric = param[9].data.d_int32;
				gdtvals.antialias = param[10].data.d_int32;
				gdtvals.alignment = param[11].data.d_int32;
				gdtvals.rotation = param[12].data.d_int32;
				gdtvals.spacing = param[13].data.d_int32;
				gdtvals.font_color = (param[14].data.d_color.red << 16) +
					(param[14].data.d_color.green << 8) + param[14].data.d_color.blue;
			}
			break;
		case RUN_WITH_LAST_VALS:
			gimp_get_data("plug_in_gdyntext", &gdtvals);
			gdtvals.image_id		= param[1].data.d_image;
			gdtvals.drawable_id	= param[2].data.d_drawable;
			gdtvals.layer_id		= param[2].data.d_layer;
			gdtvals.new_layer		= !gimp_drawable_has_alpha(gdtvals.drawable_id);
			break;
	}
	gdt_render_text(&gdtvals);
	gdt_set_values(&gdtvals);
	if (run_mode == RUN_INTERACTIVE)
		gimp_set_data("plug_in_gdyntext", &gdtvals, sizeof(GdtVals));
	values[1].data.d_int32 = gdtvals.layer_id; 
}


void gdt_get_values(GdtVals *data)
{
	char *gdtparams = NULL;
	GArray *params = NULL;

#ifdef GIMP_HAVE_PARASITES
	Parasite *parasite = NULL;

	if ((parasite = gimp_drawable_find_parasite(data->drawable_id,
		GDYNTEXT_PARASITE)) != NULL) {
		/* GDynText 1.3.1 uses one parasite */
		gdtparams = strdup(parasite_data(parasite));
		parasite_free(parasite);
	} else if ((parasite = gimp_drawable_find_parasite(data->drawable_id,
		GDYNTEXT_PARASITE_MAGIC)) != NULL) {
		/* GDynText 1.3.0 uses too parasites!! */
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_TEXT);
		strncpy(data->text, parasite_data(parasite), parasite_data_size(parasite));
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_FONT_FAMILY);
		strncpy(data->font_family, parasite_data(parasite), parasite_data_size(parasite));
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_FONT_STYLE);
		strncpy(data->font_style, parasite_data(parasite), parasite_data_size(parasite));
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_FONT_SIZE);
		data->font_size = *(gint32*)parasite_data(parasite);
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_FONT_METRIC);
		data->font_metric = *(gint*)parasite_data(parasite);
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_FONT_COLOR);
		data->font_color = *(gint32*)parasite_data(parasite);
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_ANTIALIAS);
		data->antialias = *(gboolean*)parasite_data(parasite);
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_ALIGNMENT);
		data->alignment = *(GdtAlign*)parasite_data(parasite);
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_ROTATION);
		data->rotation = *(gint*)parasite_data(parasite);
		parasite_free(parasite);
		parasite = gimp_drawable_find_parasite(data->drawable_id,
			GDYNTEXT_PARASITE_PREVIEW);
		data->preview = *(gboolean*)parasite_data(parasite);
		parasite_free(parasite);
		return;
	}
#endif

	if (gdtparams == NULL)
		gdtparams = gimp_layer_get_name(data->layer_id);

	if (!gimp_drawable_has_alpha(data->drawable_id) ||
		strncmp(gdtparams, "GDT", 3) != 0) {
		data->messages = g_list_append(data->messages,
			_("Current layer isn't a GDynText layer or it has no alpha channel.\n"
			"  Forcing new layer creation.\n"));
		data->new_layer = TRUE;
		strcpy(data->text, "");
		strcpy(data->font_family, "");
		strcpy(data->font_style, "");
		data->font_size = 50;
		data->font_metric = 0;
		{
			GParam *ret_vals;
			gint nret_vals;

			ret_vals = gimp_run_procedure("gimp_palette_get_foreground", &nret_vals,
				PARAM_END);
			data->font_color = (ret_vals[1].data.d_color.red << 16) +
				(ret_vals[1].data.d_color.green << 8) + ret_vals[1].data.d_color.blue;
			gimp_destroy_params(ret_vals, nret_vals);
		}
		data->antialias = TRUE;
		data->alignment = LEFT;
		data->rotation = 0;
		data->spacing = 0;
		return;
	}
	params = params_split(gdtparams);
	data->new_layer		= FALSE;
	data->font_size		= atoi(g_array_index(params, char *, FONT_SIZE));
	data->font_metric	= atoi(g_array_index(params, char *, FONT_SIZE_T));
	data->font_color	= strtol(g_array_index(params, char *, FONT_COLOR),
		(char **)NULL, 16);
	data->antialias		= atoi(g_array_index(params, char *, ANTIALIAS));
	/* older GDT < 0.6 formats don't have alignment */
	data->alignment = GDT_MAGIC_REV(gdtparams) < 6 ? LEFT :
		atoi(g_array_index(params, char *, ALIGNMENT));
	/* older GDT < 0.7 formats don't have rotation */
	data->rotation = GDT_MAGIC_REV(gdtparams) < 7 ? 0 :
		atoi(g_array_index(params, char *, ROTATION));
	strncpy(data->text, g_array_index(params, char *, TEXT), sizeof(data->text));
	strunescape(data->text);
	strncpy(data->font_family, strtolower(g_array_index(params, char *,
		FONT_FAMILY)), sizeof(data->font_family));
	/* older GDT < 0.8 formats don't have font style */
	strncpy(data->font_style, (GDT_MAGIC_REV(gdtparams) < 8 ? "" :
		strtolower(g_array_index(params, char *, FONT_STYLE))),
		sizeof(data->font_style));
	/* older GDT < 0.9 formats don't have spacing */
	data->spacing = GDT_MAGIC_REV(gdtparams) < 9 ? 0 :
		atoi(g_array_index(params, char *, SPACING));

	if (GDT_MAGIC_REV(gdtparams) < GDT_MAGIC_REV(GDYNTEXT_MAGIC))
		data->messages = g_list_append(data->messages,
			_("Upgrading old GDynText layer to "GDYNTEXT_MAGIC".\n"));
	else if (GDT_MAGIC_REV(gdtparams) > GDT_MAGIC_REV(GDYNTEXT_MAGIC))
		data->messages = g_list_append(data->messages, _(
			"WARNING: GDynText is too old!\n"
			"  You may loose some data by changing this text.\n"
			"  A newer version is reqired to handle this layer.\n"
			"  Get it from "GDYNTEXT_WEB_PAGE"\n"));
}


void gdt_set_values(GdtVals *data)
{
	char *lname, text[MAX_TEXT_SIZE];
#ifdef GIMP_HAVE_PARASITES
	Parasite *parasite;
#endif

	strncpy(text, data->text, sizeof(text));
	strescape(text);
	if ((lname = calloc(MAX_TEXT_SIZE + 1024 * 4, sizeof(char))) == NULL) {
		puts(_("gdt_set_values: NOT ENOUGH MEMORY!"));
		exit(1);
	}
	sprintf(lname, GDYNTEXT_MAGIC"{%s}{%s}{%d}{%d}{%06X}{%d}{%d}{%d}{%s}{%d}",
		text,
		data->font_family, data->font_size, data->font_metric, data->font_color,
		data->antialias, data->alignment, data->rotation, data->font_style,
		data->spacing);

#ifdef GIMP_HAVE_PARASITES
	parasite = parasite_new(GDYNTEXT_PARASITE, PARASITE_PERSISTENT,
		strlen(lname), lname);
	gimp_drawable_attach_parasite(data->drawable_id, parasite);
	parasite_free(parasite);

	if (!data->change_layer_name) {
		gchar *lname = gimp_layer_get_name(data->layer_id);
		if (strncmp("GDT", lname, strlen("GDT")) == 0 && lname[5] == '{') {
			/* remove old layer name */
			gimp_layer_set_name(data->layer_id, _("GDynText Layer "));
			gimp_displays_flush();
		}
		g_free(lname);
		return;
	}
#endif

	gimp_layer_set_name(data->layer_id, lname);
	gimp_displays_flush();
	g_free(lname);
}


void gdt_render_text(GdtVals *data)
{
	gint layer_ox, layer_oy, i, nret_vals, xoffs;
	gint32 layer_f;
	gint32 text_width, text_height;
  gint32 text_ascent, text_descent;
  gint32 layer_width, layer_height;
	gint32 space_width;
	GArray *text_lines, *text_lines_w, *text_style;
	GParam *ret_vals;
	GParamColor old_color, text_color;

	gimp_progress_init("GIMP Dynamic Text");
	ret_vals = gimp_run_procedure("gimp_undo_push_group_start", &nret_vals,
		PARAM_IMAGE, data->image_id, PARAM_END);
	gimp_destroy_params(ret_vals, nret_vals);

	text_style = strsplit(data->font_style, '-');

	/* retrieve space char width */
	ret_vals = gimp_run_procedure("gimp_text_get_extents", &nret_vals,
		PARAM_STRING, "A A",
		PARAM_FLOAT, (float)data->font_size,
		PARAM_INT32, 0,
		PARAM_STRING, "*",																	/* foundry */
		PARAM_STRING, data->font_family,
		PARAM_STRING, g_array_index(text_style, char *, 0),	/* weight */
		PARAM_STRING, g_array_index(text_style, char *, 1),	/* slant */
		PARAM_STRING, g_array_index(text_style, char *, 2),	/* set_width */
		PARAM_STRING, "*",																	/* spacing */
#ifdef GIMP_HAVE_FEATURES_1_1_5
		PARAM_STRING, "*",																	/* registry */
		PARAM_STRING, "*",																	/* encoding */
#endif
		PARAM_END);
	space_width = ret_vals[1].data.d_int32;
	gimp_destroy_params(ret_vals, nret_vals);
	ret_vals = gimp_run_procedure("gimp_text_get_extents", &nret_vals,
		PARAM_STRING, "AA",
		PARAM_FLOAT, (float)data->font_size,
		PARAM_INT32, 0,
		PARAM_STRING, "*",																	/* foundry */
		PARAM_STRING, data->font_family,
		PARAM_STRING, g_array_index(text_style, char *, 0),	/* weight */
		PARAM_STRING, g_array_index(text_style, char *, 1),	/* slant */
		PARAM_STRING, g_array_index(text_style, char *, 2),	/* set_width */
		PARAM_STRING, "*",																	/* spacing */
#ifdef GIMP_HAVE_FEATURES_1_1_5
		PARAM_STRING, "*",																	/* registry */
		PARAM_STRING, "*",																	/* encoding */
#endif
		PARAM_END);
	space_width -= ret_vals[1].data.d_int32;
	gimp_destroy_params(ret_vals, nret_vals);
#ifdef DEBUG
	printf("GDT: space width = %d\n", space_width);
#endif
	
	/* setup text and compute layer size */
	text_lines = text_split(data->text);
#ifdef GTK_HAVE_FEATURES_1_1_0
	text_lines_w = g_array_new(TRUE, TRUE, sizeof(int));
#else
	text_lines_w = g_array_new(TRUE);
#endif
	layer_width = layer_height = 0;
	for (i = 0; g_array_index(text_lines, char *, i); i++) {
		ret_vals = gimp_run_procedure("gimp_text_get_extents", &nret_vals,
			PARAM_STRING, strlen(g_array_index(text_lines, char *, i)) > 0 ?
				g_array_index(text_lines, char *, i) : " ",
			PARAM_FLOAT, (float)data->font_size,
			PARAM_INT32, 0,
			PARAM_STRING, "*",																	/* foundry */
			PARAM_STRING, data->font_family,
			PARAM_STRING, g_array_index(text_style, char *, 0),	/* weight */
			PARAM_STRING, g_array_index(text_style, char *, 1),	/* slant */
			PARAM_STRING, g_array_index(text_style, char *, 2),	/* set_width */
			PARAM_STRING, "*",																	/* spacing */
#ifdef GIMP_HAVE_FEATURES_1_1_5
		PARAM_STRING, "*",																	/* registry */
		PARAM_STRING, "*",																	/* encoding */
#endif
			PARAM_END);
		text_width = ret_vals[1].data.d_int32;
		text_height = ret_vals[2].data.d_int32;
		text_ascent = ret_vals[3].data.d_int32;
		text_descent = ret_vals[4].data.d_int32;
#ifdef DEBUG
    printf("GDT: %4dx%4d A:%3d D:%3d [%s]\n", text_width, text_height, text_ascent, text_descent, g_array_index(text_lines, char *, i));
#endif
		gimp_destroy_params(ret_vals, nret_vals);
#ifdef GTK_HAVE_FEATURES_1_1_0
		g_array_append_val(text_lines_w, text_width);
#else
		g_array_append_val(text_lines_w, int, text_width);
#endif
		if (layer_width < text_width)
			layer_width = text_width;
		layer_height += text_height + data->spacing;
	}
	layer_height -= data->spacing;

	if (!data->new_layer) {
		/* resize the old layer */
		gimp_layer_resize(data->layer_id, layer_width, layer_height, 0, 0);
	} else {
		/* create a new layer */
		data->layer_id = data->drawable_id = gimp_layer_new(data->image_id,
			_("GDynText Layer "), layer_width, layer_height,
			(GDrawableType)(gimp_image_base_type(data->image_id) * 2 + 1),
			100.0, NORMAL_MODE);
		gimp_layer_add_alpha(data->layer_id);
		gimp_image_add_layer(data->image_id, data->layer_id, 0);
		gimp_image_set_active_layer(data->image_id, data->layer_id);
	}

	/* clear layer */
	gimp_layer_set_preserve_transparency(data->layer_id, 0);
	ret_vals = gimp_run_procedure("gimp_edit_clear", &nret_vals,
#ifndef GIMP_HAVE_PARASITES
		PARAM_IMAGE, data->image_id,
#endif
		PARAM_DRAWABLE, data->drawable_id,
		PARAM_END);
	gimp_destroy_params(ret_vals, nret_vals);

	/* get layer offsets */
	gimp_drawable_offsets(data->layer_id, &layer_ox, &layer_oy);

	/* get foreground color */
	ret_vals = gimp_run_procedure("gimp_palette_get_foreground", &nret_vals,
		PARAM_END);
	memcpy(&old_color, &ret_vals[1].data.d_color, sizeof(GParamColor));
	gimp_destroy_params(ret_vals, nret_vals);

	/* set foreground color to the wanted text color */
	text_color.red = (data->font_color & 0xff0000) >> 16;
	text_color.green = (data->font_color & 0xff00) >> 8;
	text_color.blue = data->font_color & 0xff;
	ret_vals = gimp_run_procedure("gimp_palette_set_foreground", &nret_vals,
		PARAM_COLOR, &text_color,
		PARAM_END);
	gimp_destroy_params(ret_vals, nret_vals);

	/* write text */
	for (i = 0; g_array_index(text_lines, char *, i); i++) {
		switch (data->alignment) {
			case LEFT:
				xoffs = 0;
				break;
			case RIGHT:
				xoffs = layer_width - g_array_index(text_lines_w, int, i);
				break;
			case CENTER:
				xoffs = (layer_width - g_array_index(text_lines_w, int, i)) / 2;
				break;
			default:
				xoffs = 0;
		}
		ret_vals = gimp_run_procedure("gimp_text", &nret_vals,
			PARAM_IMAGE, data->image_id,
			PARAM_DRAWABLE, data->drawable_id,
			PARAM_FLOAT, (gdouble)layer_ox + 
				strspn(g_array_index(text_lines, char *, i), " ") * space_width +
				xoffs,																						/* x */
			PARAM_FLOAT, (gdouble)layer_oy + i * (text_height + data->spacing),		/* y */
			PARAM_STRING, g_array_index(text_lines, char *, i),
			PARAM_INT32, 0,																			/* border */
			PARAM_INT32, data->antialias,
			PARAM_FLOAT, (float)data->font_size,
			PARAM_INT32, data->font_metric,
			PARAM_STRING, "*",																	/* foundry */
			PARAM_STRING, data->font_family,
			PARAM_STRING, g_array_index(text_style, char *, 0),	/* weight */
			PARAM_STRING, g_array_index(text_style, char *, 1),	/* slant */
			PARAM_STRING, g_array_index(text_style, char *, 2),	/* set_width */
			PARAM_STRING, "*",																	/* spacing */
#ifdef GIMP_HAVE_FEATURES_1_1_5
			PARAM_STRING, "*",																	/* registry */
			PARAM_STRING, "*",																	/* encoding */
#endif
			PARAM_END);
		layer_f = ret_vals[1].data.d_layer;
		gimp_destroy_params(ret_vals, nret_vals);
		
		/* FIXME: ascent/descent stuff, use gimp_layer_translate */
#ifdef DEBUG
		printf("GDT: MH:%d LH:%d\n", text_height, gimp_drawable_height(layer_f));
#endif

		ret_vals = gimp_run_procedure("gimp_floating_sel_anchor", &nret_vals,
			PARAM_LAYER, layer_f,
			PARAM_END);
		gimp_destroy_params(ret_vals, nret_vals);
		gimp_progress_update((double)(i + 2) * 100.0 * (double)text_height /
			(double)layer_height);
	}
	g_array_free(text_lines_w, TRUE);
	g_array_free(text_style, TRUE);

	/* set foreground color to the old one */
	ret_vals = gimp_run_procedure("gimp_palette_set_foreground", &nret_vals,
		PARAM_COLOR, &old_color,
		PARAM_END);
	gimp_destroy_params(ret_vals, nret_vals);

	/* apply rotation */
	if (data->rotation != 0 && abs(data->rotation) != 360) {
		ret_vals = gimp_run_procedure("gimp_rotate", &nret_vals,
#ifndef GIMP_HAVE_PARASITES
			PARAM_IMAGE, data->image_id,
#endif
			PARAM_DRAWABLE, data->drawable_id,
			PARAM_INT32, TRUE,
			PARAM_FLOAT, (double)data->rotation * M_PI / 180.0,
#ifndef GIMP_HAVE_PARASITES
			PARAM_COLOR, &old_color,
#endif
			PARAM_END);
		gimp_destroy_params(ret_vals, nret_vals);
		gimp_layer_set_offsets(data->layer_id, layer_ox, layer_oy);
	}

	gimp_layer_set_preserve_transparency(data->layer_id, 1);

	ret_vals = gimp_run_procedure("gimp_undo_push_group_end", &nret_vals,
		PARAM_IMAGE, data->image_id, PARAM_END);
	gimp_destroy_params(ret_vals, nret_vals);

	gimp_displays_flush();
	gimp_progress_update(100.0);
}

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
