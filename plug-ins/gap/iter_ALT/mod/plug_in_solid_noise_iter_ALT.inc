/* ----------------------------------------------------------------------
 * p_plug_in_solid_noise_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_solid_noise_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_solid_noise_Vals 
    {
      long      tilable;
      long      turbulent;
      long      seed;
      long      detail;
      gdouble   xsize;
      gdouble   ysize;

      gint    timeseed;
    } t_plug_in_solid_noise_Vals; 

    t_plug_in_solid_noise_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_solid_noise_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_solid_noise_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_solid_noise_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_solid_noise_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_solid_noise_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_solid_noise_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_solid_noise_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.tilable, buf_from->tilable, buf_to->tilable, total_steps, current_step);
    p_delta_long(&buf.turbulent, buf_from->turbulent, buf_to->turbulent, total_steps, current_step);
    p_delta_long(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);
    p_delta_long(&buf.detail, buf_from->detail, buf_to->detail, total_steps, current_step);
    p_delta_gdouble(&buf.xsize, buf_from->xsize, buf_to->xsize, total_steps, current_step);
    p_delta_gdouble(&buf.ysize, buf_from->ysize, buf_to->ysize, total_steps, current_step);

    gimp_set_data("plug_in_solid_noise", &buf, sizeof(buf)); 

    return 0; /* OK */
}
