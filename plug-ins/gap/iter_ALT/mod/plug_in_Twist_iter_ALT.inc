/* ----------------------------------------------------------------------
 * p_plug_in_Twist_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_Twist_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_Twist_Vals 
    {
      float   a1;
      float   a2;
      float   a3;
      float   a4;
      float   a5;
      float   a6;
      float   a7;
      float   a8;
      gchar lettering01[30];
      gchar lettering02[30];
      gchar lettering03[30];
      gchar lettering04[30];
      gchar lettering05[30];
      gchar lettering06[30];
      gchar lettering07[30];
      gchar lettering08[30];

      gint    use_cutoff;
      float   r;
      float   dr;
      gint    current_function;
    } t_plug_in_Twist_Vals; 

    t_plug_in_Twist_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_Twist_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_Twist_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_Twist_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_Twist_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_Twist_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_Twist_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_Twist_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_float(&buf.a1, buf_from->a1, buf_to->a1, total_steps, current_step);
    p_delta_float(&buf.a2, buf_from->a2, buf_to->a2, total_steps, current_step);
    p_delta_float(&buf.a3, buf_from->a3, buf_to->a3, total_steps, current_step);
    p_delta_float(&buf.a4, buf_from->a4, buf_to->a4, total_steps, current_step);
    p_delta_float(&buf.a5, buf_from->a5, buf_to->a5, total_steps, current_step);
    p_delta_float(&buf.a6, buf_from->a6, buf_to->a6, total_steps, current_step);
    p_delta_float(&buf.a7, buf_from->a7, buf_to->a7, total_steps, current_step);
    p_delta_float(&buf.a8, buf_from->a8, buf_to->a8, total_steps, current_step);
    p_delta_gint(&buf.use_cutoff, buf_from->use_cutoff, buf_to->use_cutoff, total_steps, current_step);
    p_delta_float(&buf.r, buf_from->r, buf_to->r, total_steps, current_step);
    p_delta_float(&buf.dr, buf_from->dr, buf_to->dr, total_steps, current_step);
    p_delta_gint(&buf.current_function, buf_from->current_function, buf_to->current_function, total_steps, current_step);

    gimp_set_data("plug_in_Twist", &buf, sizeof(buf)); 

    return 0; /* OK */
}
