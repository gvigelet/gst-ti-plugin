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
#include <gstcempeg4encoder.h>

GST_DEBUG_CATEGORY_STATIC (ceenc_mpeg4);
#define GST_CAT_DEFAULT ceenc_mpeg4

/* Especification of the statics caps for mpeg4 encoder */
static GstStaticPadTemplate gst_ce_mpeg4_encoder_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
#ifdef MPEG4_ENCODER_ACCEPTS_NV12
        "video/x-raw, "
        "  format = (string) NV12,"
        "  width = (int) [ 1, 2147483647 ],"
        "  height = (int) [ 1, 2147483647 ],"
        "  framerate = (fraction)[ 0/1, 2147483647/1 ];"
#endif
    )
    );

static GstStaticPadTemplate gst_ce_mpeg4_encoder_src_factory =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg,"
        "  mpegversion=(int)4, "
        "  systemstream=(boolean)false,"
        "  width = (int) [ 1, 4096 ],"
        "  height = (int) [ 1, 4096 ],"
        "  framerate = (fraction)[ 0/1, 2147483647/1 ];")
    );

enum
{
  PROP_0,
  PROP_SINGLE_NAL
};


/* base_init of the class */
static void
gst_ce_mpeg4_encoder_base_init (GstCEMPEG4EncoderClass * klass)
{
}

/* base finalize for the class */
static void
gst_ce_mpeg4_encoder_base_finalize (GstCEMPEG4EncoderClass * klass)
{
}

static void
gst_ce_mpeg4_encoder_set_property (GObject * object, guint prop_id,
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
gst_ce_mpeg4_encoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_SINGLE_NAL:
      break;
    default:
      break;
  }
}

GstBuffer *
gst_ce_mpeg4_encoder_generate_codec_data (GstBuffer * buffer)
{

  guchar *data;
  gint i;
  GstBuffer *codec_data = NULL;

  GstMapInfo info_buffer;
  if (!gst_buffer_map (buffer, &info_buffer, GST_MAP_WRITE)) {
    GST_DEBUG ("Can't access data from buffer");
  }

  data = info_buffer.data;

  /* Search the object layer start code */
  for (i = 0; i < gst_buffer_get_size (buffer) - 4; ++i) {
    if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1 &&
        data[i + 3] == 0x20) {
      break;
    }
  }
  i++;
  /* Search next start code */
  for (; i < gst_buffer_get_size (buffer) - 4; ++i) {
    if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
      break;
    }
  }

  if ((i != (gst_buffer_get_size (buffer) - 4)) && (i != 0)) {
    /* We found a codec data */
    codec_data = gst_buffer_new_and_alloc (i);
    gst_buffer_fill (codec_data, 0, data, i);
  }

  if (codec_data == NULL) {
    GST_WARNING ("Problems with generate codec data");
  }

  return codec_data;
}

/* Implementation of fix_src_caps depending of template src caps 
 * and src_peer caps */
GstCaps *
gst_ce_mpeg4_encoder_fixate_src_caps (GstCEBaseVideoEncoder * base_video_encoder, 
    GstCaps * filter)
{

  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (base_video_encoder);
  GstCEBaseVideoEncoder *video_encoder =
      GST_CE_BASE_VIDEO_ENCODER (base_video_encoder);
  GstCEMPEG4Encoder *mpeg4_encoder = GST_CE_MPEG4_ENCODER (base_video_encoder);

  GstCaps *caps, *othercaps;
  GstCaps *template_caps;
  GstStructure *structure;
  GstStructure *other_structure;
  gboolean caps_subset;

  GST_DEBUG_OBJECT (mpeg4_encoder, "Enter fixate_src_caps mpeg4 encoder");

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
    GST_WARNING_OBJECT (mpeg4_encoder, "Suggest width, height or framerate don't valid");
    return NULL;
  }

  GST_DEBUG_OBJECT (mpeg4_encoder, "Leave fixate_src_caps mpeg4 encoder");
  return caps;

}

/* Init of the class */
static void
gst_ce_mpeg4_encoder_init (GstCEMPEG4Encoder * mpeg4_encoder)
{

  GstCEMPEG4EncoderClass *mpeg4_encoder_class =
      CE_MPEG4_ENCODER_GET_CLASS (mpeg4_encoder);

  /* Obtain base class and instance */
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (mpeg4_encoder);

  /* Obtain base video class */
  GstCEBaseVideoEncoderClass *video_encoder_class =
      GST_CE_BASE_VIDEO_ENCODER_CLASS (mpeg4_encoder_class);

  GST_DEBUG_OBJECT (mpeg4_encoder, "ENTER");

  /* Process the sinkpad */
  base_encoder->sink_pad =
      gst_pad_new_from_static_template (&gst_ce_mpeg4_encoder_sink_factory,
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
      gst_pad_new_from_static_template (&gst_ce_mpeg4_encoder_src_factory,
      "src");
  gst_element_add_pad (GST_ELEMENT (base_encoder), base_encoder->src_pad);

  /* Setup codec name */
  base_encoder->codec_name = "mpeg4enc";

  /* Init the engine handler */
  Engine_Error *engine_error = NULL;
  GST_CE_BASE_ENCODER (mpeg4_encoder)->engine_handle =
      Engine_open ("codecServer", NULL, engine_error);

  if (engine_error != Engine_EOK) {
    GST_WARNING_OBJECT (mpeg4_encoder, "Problems in Engine_open with code: %d",
        (int)engine_error);
  }

  GST_DEBUG_OBJECT (mpeg4_encoder, "LEAVE");

}

/* Function that override the post process method of the base class */
GstBuffer *
gst_ce_mpeg4_encoder_post_process (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, GList ** actual_free_slice)
{

  GstBuffer *codec_data;
  GstCaps *caps;
  gboolean set_caps_ret;

  if (base_encoder->first_buffer == FALSE) {
    /* fixate the caps */
    caps =
        gst_ce_mpeg4_encoder_fixate_src_caps (GST_CE_BASE_VIDEO_ENCODER
        (base_encoder), GST_CE_BASE_VIDEO_ENCODER(base_encoder)->sink_caps);
    if (caps == NULL) {
      GST_WARNING_OBJECT (GST_CE_MPEG4_ENCODER (base_encoder),
          "Problems for fixate the caps");
    }

    /* Generate the codec data */
    codec_data = gst_ce_mpeg4_encoder_generate_codec_data (buffer);

    /* Update the caps with the codec data */
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, codec_data, NULL);
    set_caps_ret = gst_pad_set_caps (base_encoder->src_pad, caps);
    if (set_caps_ret == FALSE) {
      GST_WARNING_OBJECT (GST_CE_MPEG4_ENCODER (base_encoder),
          "Caps can't be set");
    }
    gst_buffer_unref (codec_data);


    base_encoder->first_buffer = TRUE;
  }

  /* Restore unused memory after encode */
  gst_ce_base_encoder_restore_unused_memory (base_encoder, buffer,
      actual_free_slice);

  return buffer;
}

/* class_init of the class */
static void
gst_ce_mpeg4_encoder_class_init (GstCEMPEG4EncoderClass * klass)
{

  /* Obtain base class */
  GstCEBaseEncoderClass *base_encoder_class = GST_CE_BASE_ENCODER_CLASS (klass);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstCEBaseVideoEncoderClass *video_encoder_class =
      GST_CE_BASE_VIDEO_ENCODER_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (ceenc_mpeg4, "ceenc_mpeg4", 0,
      "CodecEngine mpeg4 encoder");

  GST_DEBUG ("ENTER");

  /* Override of heredity functions */
  base_encoder_class->base_encoder_post_process =
      gst_ce_mpeg4_encoder_post_process;
  gobject_class->set_property = gst_ce_mpeg4_encoder_set_property;
  gobject_class->get_property = gst_ce_mpeg4_encoder_get_property;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_mpeg4_encoder_src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_mpeg4_encoder_sink_factory));

  /*g_object_class_install_property (gobject_class, PROP_SINGLE_NAL,
     g_param_spec_boolean ("single-nal", "Single NAL optimization",
     "Assume encoder generates single NAL units per frame encoded to optimize avc stream generation",
     FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)); */



  GST_DEBUG ("LEAVE");
}

/* Obtain the type of the class */
GType
gst_ce_mpeg4_encoder_get_type (void)
{
  static GType object_type = 0;
  if (object_type == 0) {
    static const GTypeInfo object_info = {
      sizeof (GstCEMPEG4EncoderClass),
      (GBaseInitFunc) gst_ce_mpeg4_encoder_base_init,
      (GBaseFinalizeFunc) gst_ce_mpeg4_encoder_base_finalize,
      (GClassInitFunc) gst_ce_mpeg4_encoder_class_init,
      NULL,
      NULL,
      sizeof (GstCEMPEG4Encoder),
      0,
      (GInstanceInitFunc) gst_ce_mpeg4_encoder_init,
      NULL
    };

    object_type = g_type_register_static (GST_TYPE_CE_VIDENC1,
        "GstCEMPEG4Encoder", &object_info, (GTypeFlags) 0);
  }
  return object_type;
};
