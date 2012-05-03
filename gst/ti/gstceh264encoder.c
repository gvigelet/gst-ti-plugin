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

/* Function for generate SPS and PPS, and built the codec data */
GstBuffer *
gst_ce_h264_encoder_generate_codec_data (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer)
{

  /* Set the params */
  VIDENC1_DynamicParams *dynamic_params = base_encoder->codec_dynamic_params;
  dynamic_params->generateHeader = XDM_GENERATE_HEADER;
  
  gst_ce_base_encoder_control(base_encoder, XDM_SETPARAMS);
  

  /* Generate SPS and PPS */
  GstBuffer *ret;
  IVIDEO1_BufDescIn inBufDesc;
  XDM_BufDesc outBufDesc;
  VIDENC1_InArgs *inArgs;
  VIDENC1_OutArgs *outArgs;

  GstMapInfo info_in;
  GstMapInfo info_out;
  int outBufSizeArray[1];
  int status;

  inArgs = (VIDENC1_InArgs *) base_encoder->submitted_input_arguments;
  outArgs = (VIDENC1_OutArgs *) base_encoder->submitted_output_arguments;

  /* Access the data of the input and output buffer */
  if (!gst_buffer_map (input_buffer, &info_in, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT (base_encoder, "Can't access data from input buffer");
  }
  if (!gst_buffer_map (output_buffer, &info_out, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT (base_encoder, "Can't access data from output buffer");
  }

  /* Prepare the input buffer descriptor for the encode process */
  inBufDesc.frameWidth =
      GST_VIDEO_INFO_WIDTH (&GST_CE_BASE_VIDEO_ENCODER
      (base_encoder)->video_info);
  inBufDesc.frameHeight =
      GST_VIDEO_INFO_HEIGHT (&GST_CE_BASE_VIDEO_ENCODER
      (base_encoder)->video_info);
  inBufDesc.framePitch =
      GST_VIDEO_INFO_PLANE_STRIDE (&GST_CE_BASE_VIDEO_ENCODER
      (base_encoder)->video_info, 0);

  /* The next piece of code depend of the mime type of the buffer */
  inBufDesc.bufDesc[0].bufSize = gst_buffer_get_size (input_buffer);   
  inBufDesc.bufDesc[0].buf = info_in.data;
  inBufDesc.bufDesc[1].bufSize = gst_buffer_get_size (input_buffer);
  inBufDesc.bufDesc[1].buf = info_in.data + (inBufDesc.framePitch * inBufDesc.frameHeight);    

  /* Prepare the output buffer descriptor for the encode process */
  outBufSizeArray[0] = gst_buffer_get_size (output_buffer);
  outBufDesc.numBufs = 1;
  outBufDesc.bufs = &(info_out.data);
  outBufDesc.bufSizes = outBufSizeArray;

  /* Set output and input arguments for the encode process */
  inArgs->size = sizeof (VIDENC1_InArgs);
  inArgs->inputID = 1;
  inArgs->topFieldFirstFlag = 1;

  outArgs->size = sizeof (VIDENC1_OutArgs);

  /* Procees la encode and check for errors */
  status =
      VIDENC1_process (base_encoder->codec_handle, &inBufDesc, &outBufDesc,
      inArgs, outArgs);

  if (status != VIDENC1_EOK) {

    GST_WARNING_OBJECT (base_encoder,
        "Incorrect generate codec data with extended error: 0x%x",
        (unsigned int) outArgs->extendedError);
    return NULL;
  }

  /**********************/
  /* Prepare the result, experimental can change */
  /**********************/
  ret = gst_buffer_new_and_alloc (outArgs->encodedBuf.bufSize);
  gst_buffer_fill (ret, 0, outArgs->encodedBuf.buf,
      outArgs->encodedBuf.bufSize);

  /* Restart the params */
  dynamic_params->generateHeader = XDM_ENCODE_AU;
  dynamic_params->forceFrame = XDM_ENCODE_AU;
  gst_ce_base_encoder_control(base_encoder, XDM_SETPARAMS);

  return ret;

}

/* Implementation of fix_src_caps depending of template src caps 
 * and src_peer caps */
static gboolean
gst_ce_h264_encoder_implement_fixate_src_caps (GstCEBaseVideoEncoder * base_video_encoder,
    GstCaps * filter)
{

  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (base_video_encoder);
  GstCEBaseVideoEncoder *video_encoder =
      GST_CE_BASE_VIDEO_ENCODER (base_video_encoder);
  GstCEH264Encoder *h264_encoder = GST_CE_H264_ENCODER (base_video_encoder);

  GstBuffer *codec_data;
  GstBuffer *input_buffer;
  GstBuffer *output_buffer;
  GstCaps *caps, *othercaps;
  GstCaps *template_caps;
  GstStructure *structure;
  GstStructure *other_structure;
  GstStructure *actual_structure;
  const gchar *stream_format;
  const gchar *alignment;
  gboolean ret;

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
  
  if(!gst_caps_is_writable(caps)) {
    caps = gst_caps_make_writable(caps);
  }
  
  /* Check that the caps are fixated */
  if (!gst_caps_is_fixed (caps)) {
    gst_caps_fixate (caps);
  }
  
  structure = gst_caps_get_structure (caps, 0);
  if (structure == NULL) {
    GST_ERROR_OBJECT (base_video_encoder, "Failed to get src caps structure");
    return FALSE;
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
  /* Set the width, height and framerate */
  gst_structure_set (structure, "width", G_TYPE_INT,
      GST_VIDEO_INFO_WIDTH (&video_encoder->video_info), NULL);
  gst_structure_set (structure, "height", G_TYPE_INT,
      GST_VIDEO_INFO_HEIGHT (&video_encoder->video_info), NULL);
  gst_structure_set (structure, "framerate", GST_TYPE_FRACTION,
      GST_VIDEO_INFO_FPS_N (&video_encoder->video_info),
      GST_VIDEO_INFO_FPS_D (&video_encoder->video_info), NULL);

  /* Save the specific decision for future use */
  h264_encoder->generate_bytestream =
      !strcmp (stream_format, "byte-stream") ? TRUE : FALSE;
  h264_encoder->generate_aud = !strcmp (alignment, "aud") ? TRUE : FALSE;

  /* Obtain the codec data  */
  input_buffer = gst_buffer_new_and_alloc (100); /* Dummy buffers */
  output_buffer = gst_buffer_new_and_alloc (100); /* Dummy buffers */
  codec_data =
      gst_ce_h264_encoder_generate_codec_data (GST_CE_BASE_ENCODER
      (base_video_encoder), input_buffer, output_buffer);
  gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, codec_data, NULL);
  gst_buffer_unref(codec_data);;
  gst_buffer_unref(input_buffer);
  gst_buffer_unref(output_buffer);
  
  base_encoder->codec_data = codec_data;

  /* Set the src caps and check for errors */
  ret = gst_pad_set_caps (base_encoder->src_pad, caps);
  if (ret == FALSE) {
    GST_WARNING_OBJECT (h264_encoder, "Caps can't be set", ret);
  }

  GST_DEBUG_OBJECT (h264_encoder, "Leave fixate_src_caps h264 encoder");
  return ret;

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
  Engine_Error *engine_error;
  GST_CE_BASE_ENCODER (h264_encoder)->engine_handle =
      Engine_open ("codecServer", NULL, engine_error);

  if (engine_error != Engine_EOK) {
    GST_WARNING_OBJECT (h264_encoder, "Problems in Engine_open with code: %d",
        engine_error);
  }

  GST_DEBUG_OBJECT (h264_encoder, "LEAVE");

}

/* Set properties own from the h264 format */
static void
gst_ce_h264_encoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  
  VIDENC1_DynamicParams *dynamic_params = GST_CE_BASE_ENCODER(object)->codec_dynamic_params;
  
  switch (prop_id) {
    case PROP_FORCEINTRA:
      dynamic_params->forceFrame = g_value_get_boolean(value)?IVIDEO_IDR_FRAME:IVIDEO_NA_FRAME;
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
  VIDENC1_DynamicParams *dynamic_params = GST_CE_BASE_ENCODER(object)->codec_dynamic_params;
  switch (prop_id) {
    case PROP_FORCEINTRA:
      g_value_set_boolean(value, dynamic_params->forceFrame==IVIDEO_NA_FRAME?FALSE:TRUE);
      break;
    default:
      break;
  }
}

/* Install properties own from h264 format  */
void gst_ce_h264_install_properties(GObjectClass *gobject_class) {
  
    g_object_class_install_property(gobject_class, PROP_FORCEINTRA,
        g_param_spec_boolean("forceintra",
            "Force next frame to be an intracodec frame",
            "Force next frame to be an intracodec frame",
            FALSE, G_PARAM_READWRITE));
}

/* class_init of the class */
static void
gst_ce_h264_encoder_class_init (GstCEH264EncoderClass * klass)
{

  /* Obtain base class */
  GstCEBaseEncoderClass *base_encoder_class = GST_CE_BASE_ENCODER_CLASS (klass);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstCEBaseVideoEncoderClass *video_encoder_class =
      GST_CE_BASE_VIDEO_ENCODER_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (ceenc_h264, "ceenc_h264", 0,
      "CodecEngine h264 encoder");

  GST_DEBUG ("ENTER");
  
  /* Instance the class methods */
  klass->h264_encoder_fixate_src_caps = gst_ce_h264_encoder_implement_fixate_src_caps;
  
  /* Override of heredity functions */
  video_encoder_class->video_encoder_fixate_src_caps = klass->h264_encoder_fixate_src_caps;
  gobject_class->set_property = gst_ce_h264_encoder_set_property;
  gobject_class->get_property = gst_ce_h264_encoder_get_property;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_sink_factory));
  
  /* Install properties for the class */
  gst_ce_h264_install_properties(gobject_class);
  
  /*g_object_class_install_property (gobject_class, PROP_SINGLE_NAL,
      g_param_spec_boolean ("single-nal", "Single NAL optimization",
          "Assume encoder generates single NAL units per frame encoded to optimize avc stream generation",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));*/



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
