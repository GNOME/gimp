/* ----------------------------------------------------------------------
 * p_plug_in_sinus_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_sinus_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_sinus_Vals 
    {
      gdouble   scalex;
      gdouble   scaley;
      gdouble   cmplx;
      gdouble   blend_power;
      gint      seed;
      gint      tiling;
      gint      perturbation;
      gint      colorization;
      gint      colors;
      t_color   col1;
      guchar    col1_alpha;
      t_color   col2;
      guchar    col2_alpha;
     } t_plug_in_sinus_Vals;

    t_plug_in_sinus_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_sinus_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_sinus_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_sinus_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_sinus_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_sinus_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_sinus_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_sinus_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.scalex, buf_from->scalex, buf_to->scalex, total_steps, current_step);
    p_delta_gdouble(&buf.scaley, buf_from->scaley, buf_to->scaley, total_steps, current_step);
    p_delta_gdouble(&buf.cmplx, buf_from->cmplx, buf_to->cmplx, total_steps, current_step);
    p_delta_gdouble(&buf.blend_power, buf_from->blend_power, buf_to->blend_power, total_steps, current_step);
    p_delta_gint(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);
    p_delta_gint(&buf.tiling, buf_from->tiling, buf_to->tiling, total_steps, current_step);
    p_delta_gint(&buf.perturbation, buf_from->perturbation, buf_to->perturbation, total_steps, current_step);
    p_delta_gint(&buf.colorization, buf_from->colorization, buf_to->colorization, total_steps, current_step);
    p_delta_gint(&buf.colors, buf_from->colors, buf_to->colors, total_steps, current_step);
    p_delta_color(&buf.col1, &buf_from->col1, &buf_to->col1, total_steps, current_step);
    p_delta_char(&buf.col1_alpha, buf_from->col1_alpha, buf_to->col1_alpha, total_steps, current_step);
    p_delta_color(&buf.col2, &buf_from->col2, &buf_to->col2, total_steps, current_step);
    p_delta_char(&buf.col2_alpha, buf_from->col2_alpha, buf_to->col2_alpha, total_steps, current_step);

    gimp_set_data("plug_in_sinus", &buf, sizeof(buf)); 

    return 0; /* OK */
}
