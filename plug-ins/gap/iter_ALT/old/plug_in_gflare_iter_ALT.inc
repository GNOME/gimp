/* ----------------------------------------------------------------------
 * p_plug_in_gflare_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_gflare_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
#define GFLARE_NAME_MAX 256
    typedef struct t_plug_in_gflare_Vals 
    {
      gint      xcenter;
      gint      ycenter;
      gdouble   radius;
      gdouble   rotation;
      gdouble   hue;
      gdouble   vangle;
      gdouble   vlength;
      gint      use_asupsample;
      gint      asupsample_max_depth;
      gdouble   asupsample_threshold;
      char      gflare_name[GFLARE_NAME_MAX];
    } t_plug_in_gflare_Vals; 

    t_plug_in_gflare_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_gflare_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_gflare_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_gflare_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_gflare_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_gflare_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_gflare_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_gflare_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.xcenter, buf_from->xcenter, buf_to->xcenter, total_steps, current_step);
    p_delta_gint(&buf.ycenter, buf_from->ycenter, buf_to->ycenter, total_steps, current_step);
    p_delta_gdouble(&buf.radius, buf_from->radius, buf_to->radius, total_steps, current_step);
    p_delta_gdouble(&buf.rotation, buf_from->rotation, buf_to->rotation, total_steps, current_step);
    p_delta_gdouble(&buf.hue, buf_from->hue, buf_to->hue, total_steps, current_step);
    p_delta_gdouble(&buf.vangle, buf_from->vangle, buf_to->vangle, total_steps, current_step);
    p_delta_gdouble(&buf.vlength, buf_from->vlength, buf_to->vlength, total_steps, current_step);
    p_delta_gint(&buf.use_asupsample, buf_from->use_asupsample, buf_to->use_asupsample, total_steps, current_step);
    p_delta_gint(&buf.asupsample_max_depth, buf_from->asupsample_max_depth, buf_to->asupsample_max_depth, total_steps, current_step);
    p_delta_gdouble(&buf.asupsample_threshold, buf_from->asupsample_threshold, buf_to->asupsample_threshold, total_steps, current_step);

    gimp_set_data("plug_in_gflare", &buf, sizeof(buf)); 

    return 0; /* OK */
}
