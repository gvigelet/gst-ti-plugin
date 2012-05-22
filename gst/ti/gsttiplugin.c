/*
 * gsttiplugin.c
 *
 * Authors:
 *      Diego Dompe, ddompe@gmail.com
 *      Luis Arce, luis.arce@ridgerun.com
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

#include "gsttiplugin.h"
#include <gstceh264encoder.h>
#include <gstcempeg4encoder.h>
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/video1/videnc1.h>
#include <ti/sdo/ce/CERuntime.h>

GST_DEBUG_CATEGORY_STATIC (tiplugin);
#define GST_CAT_DEFAULT tiplugin

void gst_cmem_allocator_initialize (void);

/* Register of all the elements of the plugin */
static gboolean
TIPlugin_init (GstPlugin * plugin)
{

  gboolean ret;
  GST_DEBUG_CATEGORY_INIT (tiplugin, "ti", 0,
      "TI plugin for CodecEngine debugging");

  /* Init the CMEM allocator */
  gst_cmem_allocator_initialize ();

  /* Init the CMEM meta data */
  gst_cmem_meta_register ();

  /* Initialize the codec engine run time */
  CERuntime_init ();

  /* Register the encoders */
  ret =
      gst_element_register (plugin, "ceenc_h264", GST_RANK_PRIMARY,
      GST_TYPE_CE_H264_ENCODER);
  if (!ret) {
    g_warning ("Failed to register h264 encoder element");
  }

  ret =
      gst_element_register (plugin, "ceenc_mpeg4", GST_RANK_PRIMARY,
      GST_TYPE_CE_MPEG4_ENCODER);
  if (!ret) {
    g_warning ("Failed to register mpeg4 encoder element");
  }

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "tiplugin",
    "GStreamer Plugin for codecs based on CodecEngine API for Texas Instruments SoC",
    TIPlugin_init, VERSION, "LGPL", "gst-ti-plugin", "RidgeRun")
