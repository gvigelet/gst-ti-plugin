/*
 * Authors:
 *   Diego Dompe <ddompe@gmail.com>
 *   Luis Arce <luis.arce@rigerun.com>
 *
 * Copyright (C) 2012 RidgeRun	
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation version 2.1 of the License.
 *
 * This program is distributed #as is# WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include <gstcevideoutils.h>

/* Set the chroma format param for the codec instance depends of the video mime type (format) */
XDAS_Int32
gst_ce_video_utils_gst_video_info_to_xdm_chroma_format(GstVideoFormat format){
  switch (format) {
    case GST_VIDEO_FORMAT_NV12:
      return XDM_YUV_420SP;
    case GST_VIDEO_FORMAT_UYVY:
      return XDM_YUV_422ILE;
    default:
      GST_ERROR("Failed to convert video format at function %s",__FUNCTION__);
      return XDM_CHROMA_NA;
  }
}

/* Set the content type param for the codec instance depends of the video mime type (format) */
XDAS_Int32
gst_ce_video_utils_gst_video_info_to_xdm_content_type(GstVideoFormat format){
  switch (format) {
    case GST_VIDEO_FORMAT_NV12:
      g_print("________________________________________\n");
      return IVIDEO_PROGRESSIVE;
    case GST_VIDEO_FORMAT_UYVY:
      g_print("----------------------------------------\n");
      return IVIDEO_PROGRESSIVE;
    default:
      GST_ERROR("Failed to figure out video content type at function %s",__FUNCTION__);
      return IVIDEO_CONTENTTYPE_DEFAULT;
  }
}
