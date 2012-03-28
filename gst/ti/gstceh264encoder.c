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

#include <string.h>
#include <gst/gst.h>
#include <gstceh264encoder.h>

GST_DEBUG_CATEGORY_STATIC (ceenc_h264);
#define GST_CAT_DEFAULT ceenc_h264

/* Especification of the statics caps for h264 encoder */
static GstStaticPadTemplate gst_ce_h264_encoder_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
#ifdef H264_ENCODER_ACCEPTS_NV12
      "video/x-raw, "
      "  format = (string) NV12,"
      "  width = (int) [ 1, 2147483647 ],"
      "  height = (int) [ 1, 2147483647 ],"
      "  framerate = (fraction)[ 0/1, 2147483647/1 ];"
#endif
      )
    );

static GstStaticPadTemplate gst_ce_h264_encoder_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
      "video/x-h264,"
      "  stream-format = (string) { avc, byte-stream }, "
      "  alignment = (string) { nal, au },"
      "  width = (int) [ 1, 4096 ],"
      "  height = (int) [ 1, 4096 ],"
      "  framerate = (fraction)[ 0/1, 2147483647/1 ];"
      )
    );

enum
{
  PROP_0,
  PROP_SINGLE_NAL
};

/* base_init of the class */
static void
gst_ce_h264_encoder_base_init (GstCEH264EncoderClass * klass)
{
}

/* base finalize for the class */
static void
gst_ce_h264_encoder_base_finalize (GstCEH264EncoderClass * klass)
{
}

static void
gst_ce_h264_encoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_SINGLE_NAL:
      break;
    default:
      break;
  }
}

static void
gst_ce_h264_encoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_SINGLE_NAL:
      break;
    default:
      break;
  }
}

/* Implementation of fix_src_caps depending of template src caps 
 * and src_peer caps */
static gboolean 
gst_ce_h264_fixate_src_caps (GstCEBaseVideoEncoder * base_video_encoder) {
  
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER(base_video_encoder);
  GstCEH264Encoder *h264_encoder = GST_CE_H264_ENCODER(base_video_encoder);
  GstCaps *caps, *othercaps;
  GstCaps *filter;
  GstStructure *structure;
  GstStructure *other_structure;
  GstStructure *actual_structure;
  const gchar *stream_format;
  const gchar *alignment;
  gboolean ret;

  GST_DEBUG_OBJECT(h264_encoder,"Enter fixate_src_caps h264 encoder");
  
  /* Filter based on our template caps */
  filter = gst_pad_get_pad_template_caps (base_encoder->src_pad);
  othercaps = gst_pad_peer_query_caps (base_encoder->src_pad, filter);  
  other_structure = gst_caps_get_structure (othercaps, 0);
  
  if (othercaps == NULL ||
      gst_caps_is_empty (othercaps) || gst_caps_is_any (othercaps)) {
    /* If we got nothing useful, user our template caps */
    caps = gst_caps_copy (
      gst_pad_get_pad_template_caps (base_encoder->src_pad));
  } else {
    /* We got something useful */
    caps = othercaps;
  }
  
  /* Check that the caps are fixated */
  if (!gst_caps_is_fixed(caps)) {
    gst_caps_fixate(caps);
  }
  
  structure = gst_caps_get_structure (caps, 0);
  if (structure == NULL) {
    GST_ERROR_OBJECT(base_video_encoder, "Failed to get src caps structure");
    return FALSE;
  }
  /* Force to use avc and nal in case of null */
  stream_format = gst_structure_get_string(structure,"stream-format");
  if (stream_format == NULL){
    stream_format = "avc";
    gst_structure_set(structure,"stream-format",G_TYPE_STRING,stream_format,NULL);
  }
  alignment = gst_structure_get_string(structure,"alignment");
  if (alignment == NULL){
    alignment = "nal";
    gst_structure_set(structure,"alignment",G_TYPE_STRING,alignment,NULL);
  }
  
  /* Save the specific decision for future use */
  h264_encoder->generate_bytestream = !strcmp(stream_format,"byte-stream") ? TRUE : FALSE;
  h264_encoder->generate_aud = !strcmp(alignment,"aud") ? TRUE : FALSE;
  
  /* Set the src caps and check for errors */
  ret = gst_pad_set_caps(base_encoder->src_pad,caps);
  if(ret == FALSE) {
    GST_ERROR_OBJECT (h264_encoder, "Caps can't be set", ret);
  }

  GST_DEBUG_OBJECT(h264_encoder,"Leave fixate_src_caps h264 encoder");
  return ret;
  
}

/* Init of the class */
static void gst_ce_h264_encoder_init (GstCEH264Encoder *h264_encoder) {
  
  
  GstCEH264EncoderClass *h264_encoder_class = CE_H264_ENCODER_GET_CLASS(h264_encoder);
  
  /* Obtain base class and instance */
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER(h264_encoder);
  
  /* Obtain base video class */
  GstCEBaseVideoEncoderClass *video_encoder_class = GST_CE_BASE_VIDEO_ENCODER_CLASS(h264_encoder_class);

  GST_DEBUG_OBJECT(h264_encoder,"ENTER");

  /* Process the sinkpad */
  base_encoder->sink_pad =
      gst_pad_new_from_static_template (&gst_ce_h264_encoder_sink_factory, "sink");
  gst_pad_set_chain_function (base_encoder->sink_pad,
    GST_DEBUG_FUNCPTR (video_encoder_class->chain));
  gst_pad_set_event_function (base_encoder->sink_pad, 
    GST_DEBUG_FUNCPTR (video_encoder_class->sink_event));
  gst_pad_set_query_function (base_encoder->sink_pad, 
    GST_DEBUG_FUNCPTR (video_encoder_class->sink_query));
  gst_element_add_pad (GST_ELEMENT (base_encoder), base_encoder->sink_pad);

  /* Process the src pad */
  base_encoder->src_pad =
      gst_pad_new_from_static_template (&gst_ce_h264_encoder_src_factory, "src");
  gst_element_add_pad (GST_ELEMENT (base_encoder), base_encoder->src_pad);
  
  /* Setup codec name */
  base_encoder->codec_name = "h264enc";
  
  /* Init the engine handler */
  Engine_Error *engine_error;
  GST_CE_BASE_ENCODER(h264_encoder)->engine_handle = Engine_open("codecServer", NULL, engine_error);
  
  if(engine_error != Engine_EOK) {
    GST_WARNING_OBJECT(h264_encoder,"Problems in Engine_open with code: %d", engine_error);
  }
  
  GST_DEBUG_OBJECT(h264_encoder,"LEAVE");
  
}

/* class_init of the class */
static void
gst_ce_h264_encoder_class_init  (GstCEH264EncoderClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstCEBaseVideoEncoderClass *video_encoder_class = 
    GST_CE_BASE_VIDEO_ENCODER_CLASS(klass);

  GST_DEBUG_CATEGORY_INIT (ceenc_h264, "ceenc_h264", 0,
      "CodecEngine h264 encoder");

  GST_DEBUG("ENTER");
  gobject_class->set_property = gst_ce_h264_encoder_set_property;
  gobject_class->get_property = gst_ce_h264_encoder_get_property;

  video_encoder_class->fixate_src_caps = 
    gst_ce_h264_fixate_src_caps;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_sink_factory));

  g_object_class_install_property (gobject_class, PROP_SINGLE_NAL,
      g_param_spec_boolean ("single-nal", "Single NAL optimization", 
        "Assume encoder generates single NAL units per frame encoded to optimize avc stream generation", 
        FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  GST_DEBUG("LEAVE");
}

/* Obtain the type of the class */
GType
gst_ce_h264_encoder_get_type (void)
{
  static GType object_type = 0;
  if (object_type == 0) {
    static const GTypeInfo object_info = {
      sizeof (GstCEH264EncoderClass),
      (GBaseInitFunc) gst_ce_h264_encoder_base_init,
      (GBaseFinalizeFunc) gst_ce_h264_encoder_base_finalize,
      (GClassInitFunc) gst_ce_h264_encoder_class_init,
      NULL,
      NULL,
      sizeof (GstCEH264Encoder),
      0,
      (GInstanceInitFunc) gst_ce_h264_encoder_init,
      NULL
    };

    object_type = g_type_register_static (GST_TYPE_CE_VIDENC1,
        "GstCEH264Encoder", &object_info, (GTypeFlags) 0);
  }
  return object_type;
};
