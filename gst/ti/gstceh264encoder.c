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
#define NO_PPS -1
#define NO_SPS -2
#define NO_PPS_SPS -3
#define NAL_TAG_LENGTH 4

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
    GST_STATIC_CAPS ("video/x-h264,"
        "  stream-format = (string) { avc, byte-stream }, "
        "  alignment = (string) { nal, au },"
        "  width = (int) [ 1, 4096 ],"
        "  height = (int) [ 1, 4096 ],"
        "  framerate = (fraction)[ 0/1, 2147483647/1 ];")
    );

enum
{
  PROP_0,
  PROP_FORCEINTRA
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


/* Implementation of fix_src_caps depending of template src caps 
 * and src_peer caps */
GstCaps *
gst_ce_h264_encoder_fixate_src_caps (GstCEBaseVideoEncoder * base_video_encoder,
    GstCaps * filter)
{

  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (base_video_encoder);
  GstCEBaseVideoEncoder *video_encoder =
      GST_CE_BASE_VIDEO_ENCODER (base_video_encoder);
  GstCEH264Encoder *h264_encoder = GST_CE_H264_ENCODER (base_video_encoder);

  GstCaps *caps, *othercaps;
  GstCaps *template_caps;
  GstStructure *structure;
  GstStructure *other_structure;
  const gchar *stream_format;
  const gchar *alignment;
  gboolean caps_subset;

  GST_DEBUG_OBJECT (h264_encoder, "Enter fixate_src_caps h264 encoder");

  /* Filter based on our template caps */
  template_caps = gst_pad_get_pad_template_caps (base_encoder->src_pad);
  othercaps = gst_pad_peer_query_caps (base_encoder->src_pad, template_caps);
  other_structure = gst_caps_get_structure (othercaps, 0);

  if (othercaps == NULL ||
      gst_caps_is_empty (othercaps) || gst_caps_is_any (othercaps)) {
    /* If we got nothing useful, user our template caps */
    caps =
        gst_caps_copy (gst_pad_get_pad_template_caps (base_encoder->src_pad));
  } else {
    /* We got something useful */
    caps = othercaps;
  }
  
  /* Check is the width, height and framerate from suggest caps are valid */
  caps_subset = gst_ce_base_video_encoder_is_valid_suggest_caps(filter, caps); 
  
  if (!gst_caps_is_writable (caps)) {
    caps = gst_caps_make_writable (caps);
  }

  /* Check that the caps are fixated */
  if (!gst_caps_is_fixed (caps)) {
    gst_caps_fixate (caps);
  }

  structure = gst_caps_get_structure (caps, 0);
  if (structure == NULL) {
    GST_ERROR_OBJECT (base_video_encoder, "Failed to get src caps structure");
    return NULL;
  }
  /* Force to use avc and nal in case of null */
  stream_format = gst_structure_get_string (structure, "stream-format");
  if (stream_format == NULL) {
    stream_format = "avc";
    gst_structure_set (structure, "stream-format", G_TYPE_STRING, stream_format,
        NULL);
  }
  alignment = gst_structure_get_string (structure, "alignment");
  if (alignment == NULL) {
    alignment = "nal";
    gst_structure_set (structure, "alignment", G_TYPE_STRING, alignment, NULL);
  }
  
  
  
  if(caps_subset == TRUE) {
    /* Set the width, height and framerate */
    gst_structure_set (structure, "width", G_TYPE_INT,
        GST_VIDEO_INFO_WIDTH (&video_encoder->video_info), NULL);
    gst_structure_set (structure, "height", G_TYPE_INT,
        GST_VIDEO_INFO_HEIGHT (&video_encoder->video_info), NULL);
    gst_structure_set (structure, "framerate", GST_TYPE_FRACTION,
        GST_VIDEO_INFO_FPS_N (&video_encoder->video_info),
        GST_VIDEO_INFO_FPS_D (&video_encoder->video_info), NULL);
  }
  else {
    GST_WARNING_OBJECT (h264_encoder, "Suggest width, height or framerate don't valid");
    return NULL;
  }
  
  

  /* Save the specific decision for future use */
  h264_encoder->generate_bytestream =
      !strcmp (stream_format, "byte-stream") ? TRUE : FALSE;
  h264_encoder->generate_aud = !strcmp (alignment, "aud") ? TRUE : FALSE;

  GST_DEBUG_OBJECT (h264_encoder, "Leave fixate_src_caps h264 encoder");
  return caps;

}

/* Init of the class */
static void
gst_ce_h264_encoder_init (GstCEH264Encoder * h264_encoder)
{

  GstCEH264EncoderClass *h264_encoder_class =
      CE_H264_ENCODER_GET_CLASS (h264_encoder);

  /* Obtain base class and instance */
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (h264_encoder);

  /* Obtain base video class */
  GstCEBaseVideoEncoderClass *video_encoder_class =
      GST_CE_BASE_VIDEO_ENCODER_CLASS (h264_encoder_class);

  GST_DEBUG_OBJECT (h264_encoder, "ENTER");

  /* Process the sinkpad */
  base_encoder->sink_pad =
      gst_pad_new_from_static_template (&gst_ce_h264_encoder_sink_factory,
      "sink");
  gst_pad_set_chain_function (base_encoder->sink_pad,
      GST_DEBUG_FUNCPTR (video_encoder_class->video_encoder_chain));
  gst_pad_set_event_function (base_encoder->sink_pad,
      GST_DEBUG_FUNCPTR (video_encoder_class->video_encoder_sink_event));
  gst_pad_set_query_function (base_encoder->sink_pad,
      GST_DEBUG_FUNCPTR (video_encoder_class->video_encoder_sink_query));
  gst_element_add_pad (GST_ELEMENT (base_encoder), base_encoder->sink_pad);

  /* Process the src pad */
  base_encoder->src_pad =
      gst_pad_new_from_static_template (&gst_ce_h264_encoder_src_factory,
      "src");
  gst_element_add_pad (GST_ELEMENT (base_encoder), base_encoder->src_pad);

  /* Setup codec name */
  base_encoder->codec_name = "h264enc";

  /* Init the engine handler */
  Engine_Error *engine_error = NULL;
  GST_CE_BASE_ENCODER (h264_encoder)->engine_handle =
      Engine_open ("codecServer", NULL, engine_error);

  if (engine_error != Engine_EOK) {
    GST_WARNING_OBJECT (h264_encoder, "Problems in Engine_open with code: %d",
        (int)engine_error);
  }

  GST_DEBUG_OBJECT (h264_encoder, "LEAVE");

}

/* Set properties own from the h264 format */
static void
gst_ce_h264_encoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{

  VIDENC1_DynamicParams *dynamic_params =
      GST_CE_BASE_ENCODER (object)->codec_dynamic_params;

  switch (prop_id) {
    case PROP_FORCEINTRA:
      dynamic_params->forceFrame =
          g_value_get_boolean (value) ? IVIDEO_IDR_FRAME : IVIDEO_NA_FRAME;
      break;
    default:
      break;
  }
}

/* Get properties own from the h264 format */
static void
gst_ce_h264_encoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  VIDENC1_DynamicParams *dynamic_params =
      GST_CE_BASE_ENCODER (object)->codec_dynamic_params;
  switch (prop_id) {
    case PROP_FORCEINTRA:
      g_value_set_boolean (value,
          dynamic_params->forceFrame == IVIDEO_NA_FRAME ? FALSE : TRUE);
      break;
    default:
      break;
  }
}

/* Install properties own from h264 format  */
void
gst_ce_h264_install_properties (GObjectClass * gobject_class)
{

  g_object_class_install_property (gobject_class, PROP_FORCEINTRA,
      g_param_spec_boolean ("forceintra",
          "Force next frame to be an intracodec frame",
          "Force next frame to be an intracodec frame",
          FALSE, G_PARAM_READWRITE));
}


/* Function that override the pre process method of the base class */
GstBuffer *
gst_ce_h264_encoder_pre_process (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, GList ** actual_free_slice)
{

  GST_DEBUG_OBJECT (GST_CE_H264_ENCODER (base_encoder), "Entry");

  GstBuffer *codec_data;
  GstCaps *caps;
  gboolean set_caps_ret;
  GstBuffer *output_buffer;

  if (base_encoder->first_buffer == FALSE) {
    /* fixate the caps */
    caps =
        gst_ce_h264_encoder_fixate_src_caps (GST_CE_BASE_VIDEO_ENCODER
        (base_encoder), GST_CE_BASE_VIDEO_ENCODER(base_encoder)->sink_caps);

    if (caps == NULL) {
      GST_WARNING_OBJECT (GST_CE_H264_ENCODER (base_encoder),
          "Problems for fixate src caps");
    }

    /* Generate the codec data */
    codec_data = gst_ce_videnc1_generate_header (GST_CE_VIDENC1 (base_encoder));

    /* Update the caps with the codec data */
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, codec_data, NULL);
    set_caps_ret = gst_pad_set_caps (base_encoder->src_pad, caps);
    if (set_caps_ret == FALSE) {
      GST_WARNING_OBJECT (GST_CE_H264_ENCODER (base_encoder),
          "Src caps can't be update");
    }

    base_encoder->first_buffer = TRUE;
  }

  /* Obtain the slice of the output buffer to use */
  output_buffer =
      gst_ce_base_encoder_get_output_buffer (base_encoder, actual_free_slice);

  GST_DEBUG_OBJECT (GST_CE_H264_ENCODER (base_encoder), "Leave");

  return output_buffer;
}

/* class_init of the class */
static void
gst_ce_h264_encoder_class_init (GstCEH264EncoderClass * klass)
{

  /* Obtain base class */
  GstCEBaseEncoderClass *base_encoder_class = GST_CE_BASE_ENCODER_CLASS (klass);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (ceenc_h264, "ceenc_h264", 0,
      "CodecEngine h264 encoder");

  GST_DEBUG ("ENTER");

  /* Override of heredity functions */
  base_encoder_class->base_encoder_pre_process =
      gst_ce_h264_encoder_pre_process;
  gobject_class->set_property = gst_ce_h264_encoder_set_property;
  gobject_class->get_property = gst_ce_h264_encoder_get_property;
  

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_sink_factory));

  /* Install properties for the class */
  gst_ce_h264_install_properties (gobject_class);

  /*g_object_class_install_property (gobject_class, PROP_SINGLE_NAL,
     g_param_spec_boolean ("single-nal", "Single NAL optimization",
     "Assume encoder generates single NAL units per frame encoded to optimize avc stream generation",
     FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)); */

  GST_DEBUG ("LEAVE");
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
