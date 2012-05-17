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

#include <gst/gst.h>
#include <gsttiplugin.h>
#include <gstcevidenc1.h>
#include <gstcevideoutils.h>

#include <ti/sdo/ce/utils/xdm/XdmUtils.h>
#include <ti/sdo/ce/visa.c>
#include <ti/sdo/ce/Engine.c>
#include <ti/xdais/dm/ividenc1.h>
#include <pthread.h>



#define GST_CAT_DEFAULT gst_ce_videnc1_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_0,
  PROP_RATECONTROL,
  PROP_ENCODINGPRESET,
  PROP_MAXBITRATE,
  PROP_TARGETBITRATE,
  PROP_INTRAFRAMEINTERVAL
};



static void
gst_ce_videnc1_base_init (GstCEVIDENC1Class * klass)
{
}

static void
gst_ce_videnc1_base_finalize (GstCEVIDENC1Class * klass)
{
}

/* Commands for the _control call of the codec instace */
static const gchar *cmd_id_strings[] =
    { "XDM_GETSTATUS", "XDM_SETPARAMS", "XDM_RESET", "XDM_SETDEFAULT",
  "XDM_FLUSH", "XDM_GETBUFINFO", "XDM_GETVERSION", "XDM_GETCONTEXTINFO"
};

/* Implementation for the control function, 
 * for obtain or set information of the codec instance after create it */
static gboolean
gst_ce_videnc1_implement_control (GstCEBaseEncoder * base_encoder, gint cmd_id)
{
  GST_DEBUG_OBJECT (base_encoder,
      "ENTER videnc1_control with command: %s cevidenc1",
      cmd_id_strings[cmd_id]);
  if (base_encoder->codec_handle != NULL) {
    Int32 ret;
    VIDENC1_Status encStatus;
    encStatus.size = sizeof (VIDENC1_Status);
    encStatus.data.buf = NULL;

    ret = VIDENC1_control (base_encoder->codec_handle,
        cmd_id,
        (VIDENC1_DynamicParams *) base_encoder->codec_dynamic_params,
        &encStatus);

    if (ret != VIDENC1_EOK) {
      GST_WARNING_OBJECT (base_encoder,
          "Failure run control cmd: %s, status error %x",
          cmd_id_strings[cmd_id], (unsigned int) encStatus.extendedError);
      return FALSE;
    }
  } else {
    GST_WARNING_OBJECT (base_encoder,
        "Not running control cmd since codec is not initialized");
  }
  GST_DEBUG_OBJECT (base_encoder, "LEAVE videnc1_control cevidenc1");
  return TRUE;
}



/* Init the static and dynamic params for the codec instance */
static gboolean
gst_ce_videnc1_implement_initialize_params (GstCEBaseEncoder * base_encoder)
{


  GST_DEBUG ("Entry initialize_params cevidenc1");

  GstCEBaseVideoEncoder *video_encoder =
      GST_CE_BASE_VIDEO_ENCODER (base_encoder);

  /* Access the dynamic and static params */
  VIDENC1_Params *params = base_encoder->codec_params;
  VIDENC1_DynamicParams *dynamic_params = base_encoder->codec_dynamic_params;

  GST_DEBUG_OBJECT (base_encoder, "Configuring codec with %dx%d at %d/%d fps",
      GST_VIDEO_INFO_WIDTH (&video_encoder->video_info),
      GST_VIDEO_INFO_HEIGHT (&video_encoder->video_info),
      GST_VIDEO_INFO_FPS_N (&video_encoder->video_info),
      GST_VIDEO_INFO_FPS_D (&video_encoder->video_info));

  /* Set static params */
  params->maxWidth = GST_VIDEO_INFO_WIDTH (&video_encoder->video_info);
  params->maxHeight = GST_VIDEO_INFO_HEIGHT (&video_encoder->video_info);

  /* Set dynamic params */
  dynamic_params->inputHeight = params->maxHeight;
  dynamic_params->inputWidth = params->maxWidth;
  /* Right now we use the stride from first plane, given that VIDENC1 assumes 
   * that all planes have the same stride
   */
  dynamic_params->captureWidth =
      GST_VIDEO_INFO_PLANE_STRIDE (&video_encoder->video_info, 0);

  params->maxFrameRate =
      GST_VIDEO_INFO_FPS_N (&video_encoder->video_info) * 1000;

  dynamic_params->refFrameRate = params->maxFrameRate;
  dynamic_params->targetFrameRate = params->maxFrameRate;

  params->inputChromaFormat =
      gst_ce_video_utils_gst_video_info_to_xdm_chroma_format
      (GST_VIDEO_INFO_FORMAT (&video_encoder->video_info));
  params->reconChromaFormat = params->inputChromaFormat;
  params->inputContentType =
      gst_ce_video_utils_gst_video_info_to_xdm_content_type
      (GST_VIDEO_INFO_FORMAT (&video_encoder->video_info));

  GST_DEBUG ("Leave initialize_params cevidenc1");
  return TRUE;
}

/* Delete the actual codec instance */
static gboolean
gst_ce_videnc1_implement_delete (GstCEBaseEncoder * base_encoder)
{
  GST_DEBUG_OBJECT (base_encoder, "ENTER");

  if (base_encoder != NULL) {
    VIDENC1_delete (base_encoder->codec_handle);
  }
  base_encoder->codec_handle = NULL;

  GST_DEBUG_OBJECT (base_encoder, "LEAVE");
  return TRUE;
}

/* Create the codec instance and supply the dynamic params */
static gboolean
gst_ce_videnc1_implement_create (GstCEBaseEncoder * base_encoder)
{

  GST_DEBUG ("Enter _create cevidenc1");
  gboolean ret;

  /* Check for the entry values */
  if (base_encoder->engine_handle == NULL) {
    GST_WARNING_OBJECT (base_encoder, "Engine handle is null");
  }
  if (base_encoder->codec_params == NULL) {
    GST_WARNING_OBJECT (base_encoder, "Params are null");
  }

  if (base_encoder->codec_name == NULL) {
    GST_WARNING_OBJECT (base_encoder, "Codec name is null");
  }

  /* Create the codec handle */
  base_encoder->codec_handle = VIDENC1_create (base_encoder->engine_handle,
      (Char *) base_encoder->codec_name,
      (VIDENC1_Params *) base_encoder->codec_params);


  if (base_encoder->codec_handle == NULL) {

    GST_WARNING_OBJECT (base_encoder,
        "Failed to create the instance of the codec %s with the given parameters",
        base_encoder->codec_name);
    return FALSE;
  }

  /* Supply the dynamic params */
  ret = gst_ce_videnc1_control (base_encoder, XDM_SETPARAMS);

  GST_DEBUG ("Leave _create cevidenc1");
  return ret;
}


/* Implementation of process_sync for encode the buffer in a synchronous way */
static GstBuffer *
gst_ce_videnc1_implement_process_sync (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer)
{

  GstBuffer *encoded_buffer = NULL;
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
  inBufDesc.bufDesc[1].buf =
      info_in.data + (inBufDesc.framePitch * inBufDesc.frameHeight);
  inBufDesc.numBufs = 2;


  /* Prepare the output buffer descriptor for the encode process */
  outBufSizeArray[0] = gst_buffer_get_size (input_buffer);
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
        "Incorrect sync encode process with extended error: 0x%x",
        (unsigned int) outArgs->extendedError);
    return NULL;
  }
  
  gst_buffer_unmap(output_buffer, &info_out);
  base_encoder->memoryUsed = outArgs->bytesGenerated;
  

  return output_buffer;
}

/* Implementation of process_async for encode the buffer in a asynchronous way */
/*gboolean
gst_ce_videnc1_implement_process_async (processAsyncArguments * arguments)
{
*/

  /* Obtain the arguments */
 /* GstCEBaseEncoder *base_encoder = (GstCEBaseEncoder * )arguments->base_encoder;
    GstBuffer *input_buffer = (GstBuffer *)arguments->input_buffer;
    GstBuffer *output_buffer = (GstBuffer *)arguments->output_buffer;

    GST_DEBUG_OBJECT (GST_CE_VIDENC1(base_encoder), "Enter implement_process_async base video encoder");
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
  */
  /* Access the data of the input and output buffer */
  /*if (!gst_buffer_map (input_buffer, &info_in, GST_MAP_WRITE)) {
     GST_DEBUG_OBJECT (base_encoder, "Can't access data from input buffer");
     }
     if (!gst_buffer_map (output_buffer, &info_out, GST_MAP_WRITE)) {
     GST_DEBUG_OBJECT (base_encoder, "Can't access data from output buffer");
     }

   */
  /* Prepare the input buffer descriptor for the encode process */
  /*inBufDesc.frameWidth =
     GST_VIDEO_INFO_WIDTH (&GST_CE_BASE_VIDEO_ENCODER (base_encoder)->
     video_info);
     inBufDesc.frameHeight =
     GST_VIDEO_INFO_HEIGHT (&GST_CE_BASE_VIDEO_ENCODER (base_encoder)->
     video_info);
     inBufDesc.framePitch =
     GST_VIDEO_INFO_PLANE_STRIDE (&GST_CE_BASE_VIDEO_ENCODER (base_encoder)->
     video_info, 0);
   */
  /* The next piece of code depend of the mime type of the buffer */
  /*inBufDesc.bufDesc[0].bufSize = gst_buffer_get_size (input_buffer);    
     inBufDesc.bufDesc[0].buf = info_in.data;  //buffer_in_meta_data->physical_address; 
     inBufDesc.bufDesc[1].bufSize = gst_buffer_get_size (input_buffer);
     inBufDesc.bufDesc[1].buf = info_in.data - (inBufDesc.framePitch * inBufDesc.frameHeight);
     inBufDesc.numBufs = 2;
   */

  /* Prepare the output buffer descriptor for the encode process */
  /*outBufSizeArray[0] = gst_buffer_get_size (output_buffer);
     outBufDesc.numBufs = 1;
     outBufDesc.bufs = &(info_out.data); //&(buffer_out_meta_data->physical_address); 
     outBufDesc.bufSizes = outBufSizeArray; */

  /* Set output and input arguments for the encode process */
  /*inArgs->size = sizeof (VIDENC1_InArgs);
     inArgs->inputID = 1;
     inArgs->topFieldFirstFlag = 1;

     outArgs->size = sizeof (VIDENC1_OutArgs);
   */
  /* Procees la encode and check for errors */
 /* status =
    VIDENC1_process (base_encoder->codec_handle, &inBufDesc, &outBufDesc,
    inArgs, outArgs);

    if (status != VIDENC1_EOK) {

    GST_WARNING_OBJECT (base_encoder,
    "Incorrect async encode process with extended error: 0x%x",
    (unsigned int) outArgs->extendedError);
    return FALSE;
    }

    GST_DEBUG_OBJECT (GST_CE_VIDENC1(base_encoder), "Leave implement_process_async base video encoder");

    return TRUE;
    } */


/* Implementation of alloc_params that alloc memory for static and dynamic params 
 * and set the default values of some params */
static void
gst_ce_videnc1_default_alloc_params (GstCEBaseEncoder * base_encoder)
{
  GST_DEBUG_OBJECT (base_encoder, "ENTER");
  VIDENC1_Params *params;
  VIDENC1_DynamicParams *dynamic_params;

  /* Allocate the static params */
  base_encoder->codec_params = g_malloc0 (sizeof (VIDENC1_Params));
  if (base_encoder->codec_params == NULL) {
    GST_WARNING_OBJECT (base_encoder, "Failed to allocate VIDENC1_Params");
    return;
  }
  params = base_encoder->codec_params;
  /* Set default values for static params */
  params->size = sizeof (VIDENC1_Params);
  params->encodingPreset = XDM_HIGH_SPEED;
  params->rateControlPreset = IVIDEO_LOW_DELAY;
  params->maxBitRate = 6000000;
  params->dataEndianness = XDM_BYTE;
  params->maxInterFrameInterval = 1;

  /* Allocate the dynamic params */
  base_encoder->codec_dynamic_params =
      g_malloc0 (sizeof (VIDENC1_DynamicParams));
  if (base_encoder->codec_dynamic_params == NULL) {
    GST_WARNING_OBJECT (base_encoder,
        "Failed to allocate VIDENC1_DynamicParams");
    return;
  }
  /* Set default values for dynamic params */
  dynamic_params = base_encoder->codec_dynamic_params;
  dynamic_params->size = sizeof (VIDENC1_DynamicParams);
  dynamic_params->targetBitRate = 6000000;
  dynamic_params->intraFrameInterval = 30;
  dynamic_params->generateHeader = XDM_ENCODE_AU;
  dynamic_params->forceFrame = IVIDEO_NA_FRAME;
  dynamic_params->interFrameInterval = 1;

  GST_DEBUG_OBJECT (base_encoder, "LEAVE");
}

static void
gst_ce_videnc1_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  VIDENC1_Params *params = GST_CE_BASE_ENCODER (object)->codec_params;
  VIDENC1_DynamicParams *dynamic_params =
      GST_CE_BASE_ENCODER (object)->codec_dynamic_params;
  switch (prop_id) {
    case PROP_RATECONTROL:
      params->rateControlPreset = g_value_get_int (value);
      break;
    case PROP_ENCODINGPRESET:
      params->encodingPreset = g_value_get_int (value);
      break;
    case PROP_MAXBITRATE:
      params->maxBitRate = g_value_get_int (value);
      break;
    case PROP_TARGETBITRATE:
      dynamic_params->targetBitRate = g_value_get_int (value);
      break;
    case PROP_INTRAFRAMEINTERVAL:
      dynamic_params->intraFrameInterval = g_value_get_int (value);
      break;
    default:
      break;
  }

}

static void
gst_ce_videnc1_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{

  VIDENC1_Params *params = GST_CE_BASE_ENCODER (object)->codec_params;
  VIDENC1_DynamicParams *dynamic_params =
      GST_CE_BASE_ENCODER (object)->codec_dynamic_params;
  switch (prop_id) {
    case PROP_RATECONTROL:
      g_value_set_int (value, params->rateControlPreset);
      break;
    case PROP_ENCODINGPRESET:
      g_value_set_int (value, params->encodingPreset);
      break;
    case PROP_MAXBITRATE:
      g_value_set_int (value, params->maxBitRate);
      break;
    case PROP_TARGETBITRATE:
      g_value_set_int (value, dynamic_params->targetBitRate);
      break;
    case PROP_INTRAFRAMEINTERVAL:
      g_value_set_int (value, dynamic_params->intraFrameInterval);
      break;
    default:
      break;
  }
}


/* Install properties own from video encoders  */
void
gst_ce_videnc1_install_properties (GObjectClass * gobject_class)
{
  g_object_class_install_property (gobject_class, PROP_RATECONTROL,
      g_param_spec_int ("ratecontrol",
          "Rate Control Algorithm",
          "Rate Control Algorithm to use:\n"
          "\t\t\t 1 - Constant Bit Rate (CBR), for video conferencing\n"
          "\t\t\t 2 - Variable Bit Rate (VBR), for storage\n"
          "\t\t\t 3 - Two pass rate control for non real time applications\n"
          "\t\t\t 4 - No Rate Control is used\n"
          "\t\t\t 5 - User defined on extended parameters",
          1, 5, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ENCODINGPRESET,
      g_param_spec_int ("encodingpreset",
          "Encoding Preset Algorithm",
          "Encoding Preset Algorithm to use:\n"
          "\t\t\t 0 - Default (check codec documentation)\n"
          "\t\t\t 1 - High Quality\n"
          "\t\t\t 2 - High Speed\n"
          "\t\t\t 3 - User defined on extended parameters",
          0, 3, 2, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAXBITRATE,
      g_param_spec_int ("maxbitrate",
          "Maximum bit rate",
          "Maximum bit-rate to be supported in bits per second",
          1000, 20000000, 6000000, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TARGETBITRATE,
      g_param_spec_int ("targetbitrate",
          "Target bit rate",
          "Target bit-rate in bits per second, should be <= than the maxbitrate",
          1000, 20000000, 6000000, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INTRAFRAMEINTERVAL,
      g_param_spec_int ("intraframeinterval",
          "Intra frame interval",
          "Interval between two consecutive intra frames:\n"
          "\t\t\t 0 - Only first I frame followed by all P frames\n"
          "\t\t\t 1 - No inter frames (all intra frames)\n"
          "\t\t\t 2 - Consecutive IP sequence (if no B frames)\n"
          "\t\t\t N - (n-1) P sequences between I frames\n",
          0, G_MAXINT32, 30, G_PARAM_READWRITE));
}


/* Function that generate the pps and the sps of the codec 
 * with the specific params
 * */
static GstBuffer *
gst_ce_videnc1_default_generate_header (GstCEVIDENC1 * videnc1_encoder)
{

  GstBuffer *header;
  GstBuffer *input_buffer;
  GstBuffer *output_buffer;

  /* Set the params */
  VIDENC1_DynamicParams *dynamic_params =
      GST_CE_BASE_ENCODER (videnc1_encoder)->codec_dynamic_params;
  dynamic_params->generateHeader = XDM_GENERATE_HEADER;
  if (!gst_ce_videnc1_control (videnc1_encoder, XDM_SETPARAMS)) {
    GST_WARNING_OBJECT (videnc1_encoder,
        "Probles for set params for generate header");
    return NULL;
  }

  /* Generate the header */
  input_buffer = gst_buffer_new_and_alloc (100);        /* Dummy buffers */
  output_buffer = gst_buffer_new_and_alloc (100);       /* Dummy buffers */
  header =
      gst_ce_videnc1_process_sync (videnc1_encoder, input_buffer,
      output_buffer);
  gst_buffer_unref (input_buffer);

  /* Reset to the params to the origina value */
  dynamic_params->generateHeader = XDM_ENCODE_AU;
  dynamic_params->forceFrame = XDM_ENCODE_AU;
  gst_ce_videnc1_control (videnc1_encoder, XDM_SETPARAMS);

  if (header == NULL) {
    GST_WARNING_OBJECT (videnc1_encoder,
        "Probles for generate header with the actual params");
    return NULL;
  }

  return header;
}


static void
gst_ce_videnc1_class_init (GstCEVIDENC1Class * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  /* Obtain base class */
  GstCEBaseEncoderClass *base_encoder_class = GST_CE_BASE_ENCODER_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_ce_videnc1_debug, "cevidenc1", 0,
      "Codec Engine VIDENC1 Class");

  GST_DEBUG ("ENTER");

  /* Instance the class methods */
  klass->videnc1_initialize_params = gst_ce_videnc1_implement_initialize_params;
  klass->videnc1_control = gst_ce_videnc1_implement_control;
  klass->videnc1_delete = gst_ce_videnc1_implement_delete;
  klass->videnc1_create = gst_ce_videnc1_implement_create;
  klass->videnc1_process_sync = gst_ce_videnc1_implement_process_sync;
  klass->videnc1_alloc_params = gst_ce_videnc1_default_alloc_params;
  klass->videnc1_generate_header = gst_ce_videnc1_default_generate_header;

  /* Override of heredity functions */
  gobject_class->set_property = gst_ce_videnc1_set_property;
  gobject_class->get_property = gst_ce_videnc1_get_property;
  base_encoder_class->base_encoder_control = klass->videnc1_control;
  base_encoder_class->base_encoder_delete = klass->videnc1_delete;
  base_encoder_class->base_encoder_create = klass->videnc1_create;
  base_encoder_class->base_encoder_process_sync = klass->videnc1_process_sync;
  base_encoder_class->base_encoder_initialize_params =
      klass->videnc1_initialize_params;

  /* Install properties for the class */
  gst_ce_videnc1_install_properties (gobject_class);

  GST_DEBUG ("LEAVE");
}


/* Implementation of free_params that free the memory of
 * static and dynamic params */
static void
gst_ce_videnc1_free_params (GstCEBaseEncoder * base_encoder)
{
  if (base_encoder->codec_params != NULL) {
    g_free (base_encoder->codec_params);
    base_encoder->codec_params = NULL;
  }

  if (base_encoder->codec_dynamic_params != NULL) {
    g_free (base_encoder->codec_dynamic_params);
    base_encoder->codec_dynamic_params = NULL;
  }
}

/* init of the class */
static void
gst_ce_videnc1_init (GstCEBaseEncoder * base_encoder,
    GstCEBaseEncoderClass * base_encoder_class)
{
  GST_DEBUG ("Enter init videnc1");

  /* Allocate memory for the params and set some default values */
  gst_ce_videnc1_alloc_params (base_encoder);

  GST_DEBUG ("Leave init videnc1");
}

/* NOT USE */
static void
gst_ce_videnc1_dispose (GObject * gobject)
{

  GST_DEBUG_OBJECT (gobject, "ENTER");
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (gobject);

  gst_ce_videnc1_free_params (base_encoder);

  GST_DEBUG_OBJECT (base_encoder, "LEAVE");

  /* Chain up to the parent class */
  G_OBJECT_CLASS (&(CE_VIDENC1_GET_CLASS (gobject)->
          parent_class))->dispose (gobject);
}

/* Obtain and register the type of the class */
GType
gst_ce_videnc1_get_type (void)
{
  static GType object_type = 0;

  if (object_type == 0) {
    static const GTypeInfo object_info = {
      sizeof (GstCEVIDENC1Class),
      (GBaseInitFunc) gst_ce_videnc1_base_init,
      (GBaseFinalizeFunc) gst_ce_videnc1_base_finalize,
      (GClassInitFunc) gst_ce_videnc1_class_init,
      NULL,
      NULL,
      sizeof (GstCEVIDENC1),
      0,
      (GInstanceInitFunc) gst_ce_videnc1_init
    };

    object_type = g_type_register_static (GST_TYPE_CE_BASE_VIDEO_ENCODER,
        "GstCEVIDENC1", &object_info, (GTypeFlags) 0);
  }
  return object_type;
};
