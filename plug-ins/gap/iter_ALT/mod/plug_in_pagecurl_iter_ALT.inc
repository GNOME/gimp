/* ----------------------------------------------------------------------
 * p_plug_in_pagecurl_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_pagecurl_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_pagecurl_Vals 
    {
       gint do_curl_shade;
       gint do_curl_gradient;
       gint do_curl_warp;  /* Not yet supported... */

       double do_curl_opacity;
       gint do_shade_under;

       gint do_upper_left;
       gint do_upper_right;
       gint do_lower_left;
       gint do_lower_right;

       gint do_vertical;
       gint do_horizontal;
    } t_plug_in_pagecurl_Vals; 

    t_plug_in_pagecurl_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_pagecurl_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_pagecurl_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_pagecurl_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_pagecurl_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_pagecurl_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_pagecurl_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_pagecurl_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.do_curl_shade, buf_from->do_curl_shade, buf_to->do_curl_shade, total_steps, current_step);
    p_delta_gint(&buf.do_curl_gradient, buf_from->do_curl_gradient, buf_to->do_curl_gradient, total_steps, current_step);
    p_delta_gint(&buf.do_curl_warp, buf_from->do_curl_warp, buf_to->do_curl_warp, total_steps, current_step);
    p_delta_gdouble(&buf.do_curl_opacity, buf_from->do_curl_opacity, buf_to->do_curl_opacity, total_steps, current_step);
    p_delta_gint(&buf.do_shade_under, buf_from->do_shade_under, buf_to->do_shade_under, total_steps, current_step);
    p_delta_gint(&buf.do_upper_left, buf_from->do_upper_left, buf_to->do_upper_left, total_steps, current_step);
    p_delta_gint(&buf.do_upper_right, buf_from->do_upper_right, buf_to->do_upper_right, total_steps, current_step);
    p_delta_gint(&buf.do_lower_left, buf_from->do_lower_left, buf_to->do_lower_left, total_steps, current_step);
    p_delta_gint(&buf.do_lower_right, buf_from->do_lower_right, buf_to->do_lower_right, total_steps, current_step);
    p_delta_gint(&buf.do_vertical, buf_from->do_vertical, buf_to->do_vertical, total_steps, current_step);
    p_delta_gint(&buf.do_horizontal, buf_from->do_horizontal, buf_to->do_horizontal, total_steps, current_step);

    gimp_set_data("plug_in_pagecurl", &buf, sizeof(buf)); 

    return 0; /* OK */
}
