/* ----------------------------------------------------------------------
 * p_plug_in_sinus_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_sinus_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef gdouble colRGBA[4];

    typedef struct t_plug_in_sinus_Vals 
    {
      gdouble   xscale;
      gdouble   yscale;
      gdouble   complex;
      gdouble   blend_power;
      gint      seed;
      gint      tiling;
      gint      perturb;
      gint      colorization;
      gint      colors;
      colRGBA   alpha1;
      colRGBA   alpha2;
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

    p_delta_gdouble(&buf.xscale, buf_from->xscale, buf_to->xscale, total_steps, current_step);
    p_delta_gdouble(&buf.yscale, buf_from->yscale, buf_to->yscale, total_steps, current_step);
    p_delta_gdouble(&buf.complex, buf_from->complex, buf_to->complex, total_steps, current_step);
    p_delta_gint(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);
    p_delta_gint(&buf.tiling, buf_from->tiling, buf_to->tiling, total_steps, current_step);
    p_delta_gint(&buf.perturb, buf_from->perturb, buf_to->perturb, total_steps, current_step);
    p_delta_gint(&buf.colorization, buf_from->colorization, buf_to->colorization, total_steps, current_step);
    p_delta_gint(&buf.colors, buf_from->colors, buf_to->colors, total_steps, current_step);

    p_delta_gdouble(&buf.alpha1[0], buf_from->alpha1[0], buf_to->alpha1[0], total_steps, current_step);
    p_delta_gdouble(&buf.alpha1[1], buf_from->alpha1[1], buf_to->alpha1[1], total_steps, current_step);
    p_delta_gdouble(&buf.alpha1[2], buf_from->alpha1[2], buf_to->alpha1[2], total_steps, current_step);

    p_delta_gdouble(&buf.alpha2[0], buf_from->alpha2[0], buf_to->alpha2[0], total_steps, current_step);
    p_delta_gdouble(&buf.alpha2[1], buf_from->alpha2[1], buf_to->alpha2[1], total_steps, current_step);
    p_delta_gdouble(&buf.alpha2[2], buf_from->alpha2[2], buf_to->alpha2[2], total_steps, current_step);

    p_delta_gdouble(&buf.blend_power, buf_from->blend_power, buf_to->blend_power, total_steps, current_step);

    gimp_set_data("plug_in_sinus", &buf, sizeof(buf)); 

    return 0; /* OK */
}
