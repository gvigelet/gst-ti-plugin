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


/*static void fetch_nal(guchar *buffer_data, int buffer_data_length, gint type, int *index, int *length)
{
    gint i;
    
    guchar *data = buffer_data;
    GstBuffer *nal_buffer;
    gint nal_idx = 0;
    gint nal_len = 0;
    gint nal_type = 0;
    gint found = 0;
    gint done = 0;

    for (i = 0; i < buffer_data_length; i++) {
        if (buffer_data[i] == 0 && buffer_data[i + 1] == 0 && buffer_data[i + 2] == 0 
            && buffer_data[i + 3] == 1) {
            if (found == 1) {
                nal_len = i - nal_idx;
                done = 1;
                break;
            }
            */
            /* Calculate the type of the nal */
            /*nal_type = (buffer_data[i + 4]) & 0x1f;
               g_print("nal_type: %d type: %d \n", nal_type, type);
               if (nal_type == type)
               {
               found = 1;
               nal_idx = i + 4;
               i += 4;
               g_print("Entro (nal_type == type) found:%d nal_idx:%d\n", found, nal_idx);
               }
               }
               }
             */
    /* Check if the NAL stops at the end */
  /*  g_print("done:%d i:%d buffer_data_length:%d\n", done, i, buffer_data_length);
     if (found == 1 && i == buffer_data_length) {
     nal_len = buffer_data_length - nal_idx;
     done = 1;
     g_print("Entro primer if\n");
     }

     if (done == 1) {
     *index = nal_idx;
     *length = nal_len;
     g_print("Entro segundo if\n");

     } else { */
        /* Indicate that the type of Nal was not found */
        /* *index = -1;
         *length = -1;
         }
         }*/

/*static GstBuffer*
gst_ce_h264_encoder_buffer_header(guchar *buffer_data, int buffer_data_lenght){
    GstBuffer *avcc = NULL;
    guchar *avcc_data = NULL;
                                                                                gint avcc_len = 7;*/// Default 7 bytes w/o SPS, PPS data
    /*gint i;

       GstBuffer *sps = NULL;
       guchar *sps_data = NULL;
       gint num_sps=0;

       GstBuffer *pps = NULL;
       gint num_pps=0;

       guchar profile;
       guchar compatibly;
       guchar level;
       GstMapInfo sps_info;
       GstMapInfo pps_info;
       int sps_index = -1;
       int sps_length = -1;
       int pps_index = -1;
       int pps_length = -1;

                                                                                                                                                              fetch_nal(buffer_data, buffer_data_lenght, 7, &sps_index, &sps_length); */// 7 = SPS
    /*if ((sps_index != -1) && (sps_length != -1)){
       num_sps = 1;
       avcc_len += sps_length + 2; */
        //gst_buffer_map (sps, &sps_info, GST_MAP_WRITE);
        //sps_data = sps_info.data;

       /* profile     = buffer_data[sps_index + 1];
          compatibly  = buffer_data[sps_index + 2];
          level       = buffer_data[sps_index + 3]; 

          } else {

                                                          profile     = 66;   */// Default Profile: Baseline
        //compatibly  = 0;
        //level       = 30;   // Default Level: 3.0
    /*}
       fetch_nal(buffer_data, buffer_data_lenght, 8, &pps_index, &pps_length); // 8 = PPS
       if ((pps_index != -1) && (pps_length != -1)){
       num_pps = 1;
       avcc_len += pps_length + 2;
       }

       avcc = gst_buffer_new_and_alloc(avcc_len);
       GstMapInfo avcc_info;
       gst_buffer_map (avcc, &avcc_info, GST_MAP_WRITE);

       avcc_data = avcc_info.data;
                                                                          avcc_data[0] = 1;             */// [0] 1 byte - version
    //avcc_data[1] = profile;       // [1] 1 byte - h.264 stream profile
    //avcc_data[2] = compatibly;    // [2] 1 byte - h.264 compatible profiles
    //avcc_data[3] = level;         // [3] 1 byte - h.264 stream level
    //avcc_data[4] = 0xfc | (NAL_TAG_LENGTH-1);  // [4] 6 bits - reserved all ONES = 0xfc
                                  // [4] 2 bits - NAL length ( 0 - 1 byte; 1 - 2 bytes; 3 - 4 bytes)
    //avcc_data[5] = 0xe0 | num_sps;// [5] 3 bits - reserved all ONES = 0xe0
                                  // [5] 5 bits - number of SPS

    /*i = 6;
       if (num_sps > 0){
       avcc_data[i++] = sps_length >> 8;
       avcc_data[i++] = sps_length & 0xff;
       memcpy(&avcc_data[i],(buffer_data + sps_index),sps_length);
       i += sps_length;
       }
                                                                            avcc_data[i++] = num_pps;      */// [6] 1 byte  - number of PPS
    /*if (num_pps > 0){
       avcc_data[i++] = pps_length >> 8;
       avcc_data[i++] = pps_length & 0xff;
       memcpy(&avcc_data[i],(buffer_data + pps_index),pps_length);
       i += pps_length;
       }

       gst_buffer_unmap (avcc, &avcc_info);

       return avcc;
       }

       /* Function for calculate the pps index in the header */
/*void 
gst_ce_h264_encoder_scan_data(gint8 *data, int data_size, gint ret[]) {
  
  int index;
  gboolean first_nal = TRUE;
  for(index = 0; index < data_size; index++) {
    if (data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 0 
        && data[index + 3] == 1) {
      if(first_nal == TRUE) {
        first_nal = FALSE;
      }
      else {
                                                                                                                                      index = index + NAL_TAG_LENGTH; *//* jump the nal tag */
      /*  break;
         }
         }
         }
       */
  /* Recognize if sps or pps don't exist */
  /*if(first_nal == TRUE) {
     ret[0] = NO_PPS_SPS;
     ret[1] = index;
     }
     else if(index == data_size){
     ret[0] = NO_PPS;
     ret[1] = index;
     }
                                                      else if(index == 8){ *///8 = after 2 nal tags
/*    ret[0] = NO_SPS;
    ret[1] = index;
  }
  else {
    ret[0] = 0;
    ret[1] = index;
  } 
  return ret;
}*/

/* Function for generate SPS and PPS, and built the codec data */
static GstBuffer *
gst_ce_h264_encoder_generate_codec_data (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer)
{

  //int buf_exam[2];

  /* Set the params */
  VIDENC1_DynamicParams *dynamic_params = base_encoder->codec_dynamic_params;
  dynamic_params->generateHeader = XDM_GENERATE_HEADER;
  CE_BASE_ENCODER_GET_CLASS (base_encoder)->encoder_control (base_encoder,
      XDM_SETPARAMS);


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
  inBufDesc.bufDesc[0].bufSize = gst_buffer_get_size (input_buffer);    /*NNPodria ser esto */
  inBufDesc.bufDesc[0].buf = info_in.data;
  inBufDesc.bufDesc[1].bufSize = gst_buffer_get_size (input_buffer);
  inBufDesc.bufDesc[1].buf = info_in.data + (inBufDesc.frameWidth * inBufDesc.frameHeight);     //(gst_buffer_get_size(input_buffer) * (2 / 3));
  //inBufDesc.numBufs = 2;

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
  //ret = gst_ce_h264_encoder_buffer_header(outArgs->encodedBuf.buf, outArgs->encodedBuf.bufSize);

  /* Restart the params */
  dynamic_params->generateHeader = XDM_ENCODE_AU;
  dynamic_params->forceFrame = XDM_ENCODE_AU;
  CE_BASE_ENCODER_GET_CLASS (base_encoder)->encoder_control (base_encoder,
      XDM_SETPARAMS);

  return ret;

}

/* Implementation of fix_src_caps depending of template src caps 
 * and src_peer caps */
static gboolean
gst_ce_h264_fixate_src_caps (GstCEBaseVideoEncoder * base_video_encoder,
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
  input_buffer = gst_buffer_new_and_alloc (100);
  output_buffer = gst_buffer_new_and_alloc (100);
  codec_data =
      gst_ce_h264_encoder_generate_codec_data (GST_CE_BASE_ENCODER
      (base_video_encoder), input_buffer, output_buffer);
  gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, codec_data, NULL);

  /* Set the src caps and check for errors */
  ret = gst_pad_set_caps (base_encoder->src_pad, caps);
  if (ret == FALSE) {
    GST_WARNING_OBJECT (h264_encoder, "Caps can't be set", ret);
  } else {
    GST_DEBUG_OBJECT (h264_encoder, "Caps set succesfull");
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
      GST_DEBUG_FUNCPTR (video_encoder_class->chain));
  gst_pad_set_event_function (base_encoder->sink_pad,
      GST_DEBUG_FUNCPTR (video_encoder_class->sink_event));
  gst_pad_set_query_function (base_encoder->sink_pad,
      GST_DEBUG_FUNCPTR (video_encoder_class->sink_query));
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
  gobject_class->set_property = gst_ce_h264_encoder_set_property;
  gobject_class->get_property = gst_ce_h264_encoder_get_property;

  video_encoder_class->fixate_src_caps = gst_ce_h264_fixate_src_caps;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ce_h264_encoder_sink_factory));

  g_object_class_install_property (gobject_class, PROP_SINGLE_NAL,
      g_param_spec_boolean ("single-nal", "Single NAL optimization",
          "Assume encoder generates single NAL units per frame encoded to optimize avc stream generation",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Implementation of inherenci functions */
  base_encoder_class->encoder_generate_codec_data =
      GST_DEBUG_FUNCPTR (gst_ce_h264_encoder_generate_codec_data);

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
