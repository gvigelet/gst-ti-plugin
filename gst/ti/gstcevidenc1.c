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
static const gchar* cmd_id_strings[] = 
{ "XDM_GETSTATUS", "XDM_SETPARAMS", "XDM_RESET", "XDM_SETDEFAULT", 
  "XDM_FLUSH", "XDM_GETBUFINFO", "XDM_GETVERSION", "XDM_GETCONTEXTINFO"
};

/* Implementation for the control function, 
 * for obtain or set information of the codec instance after create it */
static gboolean
gst_ce_videnc1_control(GstCEBaseEncoder * base_encoder, gint cmd_id){
  GST_DEBUG_OBJECT(base_encoder,"ENTER videnc1_control with command: %s cevidenc1", cmd_id_strings[cmd_id]);
  if (base_encoder->codec_handle != NULL){
    Int32 ret;
    VIDENC1_Status encStatus;
    encStatus.size = sizeof(VIDENC1_Status);
    encStatus.data.buf = NULL;
    
    ret = VIDENC1_control(base_encoder->codec_handle,
                          cmd_id, 
                          (VIDENC1_DynamicParams *) base_encoder->codec_dynamic_params,
                          &encStatus);
    
    if (ret != VIDENC1_EOK) {
      GST_WARNING_OBJECT(base_encoder,"Failure run control cmd: %s, status error %ud",
                         cmd_id_strings[cmd_id], (unsigned int) encStatus.extendedError);
      return FALSE;
    }
  } else {
    GST_WARNING_OBJECT(base_encoder,"Not running control cmd since codec is not initialized");
  }
  GST_DEBUG_OBJECT(base_encoder,"LEAVE videnc1_control cevidenc1");
  return TRUE;
}

static void
gst_ce_videnc1_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      break;
  }

}

static void
gst_ce_videnc1_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      break;
  }
}

/* Init the static and dynamic params for the codec instance */
static gboolean
gst_ce_videnc1_initialize_params (GstCEBaseEncoder * base_encoder) {
  
  
  GST_DEBUG("Entry initialize_params cevidenc1");

  GstCEBaseVideoEncoder * video_encoder = 
    GST_CE_BASE_VIDEO_ENCODER(base_encoder);
  
  /* Access the dynamic and static params */
  VIDENC1_Params *params = base_encoder->codec_params;
  VIDENC1_DynamicParams *dynamic_params = base_encoder->codec_dynamic_params;
  
  GST_DEBUG_OBJECT(base_encoder,"Configuring codec with %dx%d at %d/%d fps",
    GST_VIDEO_INFO_WIDTH(&video_encoder->video_info),
    GST_VIDEO_INFO_HEIGHT(&video_encoder->video_info),
    GST_VIDEO_INFO_FPS_N(&video_encoder->video_info),
    GST_VIDEO_INFO_FPS_D(&video_encoder->video_info));
  
  /* Set static params */
  params->maxWidth = GST_VIDEO_INFO_WIDTH(&video_encoder->video_info);
  params->maxHeight = GST_VIDEO_INFO_HEIGHT(&video_encoder->video_info);
  
  /* Set dynamic params */
  dynamic_params->inputHeight = params->maxHeight;
  dynamic_params->inputWidth = params->maxWidth;
  /* Right now we use the stride from first plane, given that VIDENC1 assumes 
   * that all planes have the same stride
   */
  dynamic_params->captureWidth = GST_VIDEO_INFO_PLANE_STRIDE(&video_encoder->video_info,0);
  
  params->maxFrameRate = GST_VIDEO_INFO_FPS_N(&video_encoder->video_info) * 1000;// / 
    //GST_VIDEO_INFO_FPS_D(&video_encoder->video_info);
  dynamic_params->refFrameRate = params->maxFrameRate;
  dynamic_params->targetFrameRate = params->maxFrameRate;
  
  params->inputChromaFormat = gst_ce_video_utils_gst_video_info_to_xdm_chroma_format(
    GST_VIDEO_INFO_FORMAT(&video_encoder->video_info));
  params->inputContentType = gst_ce_video_utils_gst_video_info_to_xdm_content_type(
      GST_VIDEO_INFO_FORMAT(&video_encoder->video_info));

  GST_DEBUG("Leave initialize_params cevidenc1");
  return TRUE;
}

/* Delete the actual codec instance */
static gboolean
gst_ce_videnc1_delete (GstCEBaseEncoder * base_encoder) {
  GST_DEBUG_OBJECT(base_encoder,"ENTER");

  if (base_encoder != NULL) {
    VIDENC1_delete (base_encoder->codec_handle);
  }
  base_encoder->codec_handle = NULL;
  
  GST_DEBUG_OBJECT(base_encoder,"LEAVE");
  return TRUE;
}

/* Create the codec instance and supply the dynamic params */
static gboolean
gst_ce_videnc1_create (GstCEBaseEncoder * base_encoder) {
  
  GST_DEBUG("Enter _create cevidenc1");
  gboolean ret;
  
  
  /* Check for the entry values */
  if(base_encoder->engine_handle == NULL) {
    GST_WARNING("Engine handle is null");
  }
  if(base_encoder->codec_params == NULL) {
    GST_WARNING("Params are null");
  }
  
  if(base_encoder->codec_name == NULL) {
    GST_WARNING("Codec name is null");
  }
  
  /* Create the codec handle */
  base_encoder->codec_handle = VIDENC1_create(base_encoder->engine_handle,
                                              (Char *)base_encoder->codec_name,
                                              (VIDENC1_Params *)base_encoder->codec_params);
  
  /* Set the size of the message for the coprocessor */
  IVIDENC1_Handle alg = VISA_getAlgHandle((VISA_Handle)base_encoder->codec_handle);
  VISA_Handle visa = (VISA_Handle)alg;
  visa->maxMsgSize = 1500;
  visa->cmd = g_malloc0(sizeof(VISA_Msg) * visa->nCmds);
  
  
  
  if (base_encoder->codec_handle == NULL){
  
    GST_ERROR("Failed to create the instance of the codec %s with the given parameters",
                     base_encoder->codec_name);
    return FALSE;
  }
  
  /* Supply the dynamic params */
  ret = gst_ce_videnc1_control(base_encoder,XDM_SETPARAMS);
  
  GST_DEBUG("Leave _create cevidenc1");
  return ret;
}


/* Implementation of process_sync for encode the buffer in a synchronous way */
static gboolean
gst_ce_videnc1_process_sync (GstCEBaseEncoder * base_encoder,
   GstBuffer * input_buffer, GstBuffer * output_buffer){
  return TRUE;
}

/* Implementation of process_async for encode the buffer in a asynchronous way */
static gboolean
gst_ce_videnc1_process_async (processAsyncArguments * arguments)
{
  
  
  /* Obtain the arguments */
  GstCEBaseEncoder *base_encoder = arguments->base_encoder;
  GstBuffer *input_buffer = arguments->input_buffer;
  GstBuffer *output_buffer = arguments->output_buffer;
  
  IVIDEO1_BufDescIn       inBufDesc;
  XDM_BufDesc outBufDesc;
  VIDENC1_InArgs          *inArgs;
  VIDENC1_OutArgs         *outArgs;
  
  GstMapInfo info_in;
  GstMapInfo info_out;
  int outBufSizeArray[1];
  int status;
  
  inArgs = (VIDENC1_InArgs *)base_encoder->submitted_input_arguments;
  outArgs = (VIDENC1_OutArgs *)base_encoder->submitted_output_arguments;

  /* Access the data of the input and output buffer */
  if(!gst_buffer_map (input_buffer, &info_in, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT(base_encoder,"Can't access data from input buffer");
  }
    if(!gst_buffer_map (output_buffer, &info_out, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT(base_encoder,"Can't access data from output buffer");
  }
  
  /* Prepare the input buffer descriptor for the encode process */
  inBufDesc.frameWidth = GST_VIDEO_INFO_WIDTH(&GST_CE_BASE_VIDEO_ENCODER(base_encoder)->video_info);
  inBufDesc.frameHeight = GST_VIDEO_INFO_HEIGHT(&GST_CE_BASE_VIDEO_ENCODER(base_encoder)->video_info);
  inBufDesc.framePitch = GST_VIDEO_INFO_PLANE_STRIDE(&GST_CE_BASE_VIDEO_ENCODER(base_encoder)->video_info, 0);
  
  /* The next piece of code depend of the mime type of the buffer */
  inBufDesc.bufDesc[0].bufSize = gst_buffer_get_size(input_buffer);
  inBufDesc.bufDesc[0].buf = info_in.data;
  inBufDesc.bufDesc[1].bufSize = gst_buffer_get_size(input_buffer);
  inBufDesc.bufDesc[1].buf = info_in.data + (gst_buffer_get_size(input_buffer) * (2 / 3));
  inBufDesc.numBufs = 2;
  
  /* Prepare the output buffer descriptor for the encode process */
  outBufSizeArray[0]                  = gst_buffer_get_size(output_buffer);
  outBufDesc.numBufs                  = 1;
  outBufDesc.bufs                     = &(info_out.data);
  outBufDesc.bufSizes                 = outBufSizeArray;
  
  /* Set output and input arguments for the encode process */
  inArgs->size                         = sizeof(VIDENC1_InArgs);
  inArgs->inputID                      = 1;
  inArgs->topFieldFirstFlag            = 1;
  
  outArgs->size                        = sizeof(VIDENC1_OutArgs);
  
  /* Procees la encode and check for errors*/
  status = VIDENC1_process(base_encoder->codec_handle, &inBufDesc, &outBufDesc, inArgs, outArgs);
  
  if (status != VIDENC1_EOK) {
    
    GST_WARNING_OBJECT(base_encoder,"Incorrect async encode process with extended error: 0x%x", 
      (unsigned int) outArgs->extendedError);
    return FALSE;
  }
  
  return TRUE;
}

/* Implementarion of process_wait for wait any previews calls of the  process_async function */
static gboolean
gst_ce_videnc1_process_wait  (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer, gint timeout){
  
  IVIDEO1_BufDescIn       inBufDesc;
  XDM_BufDesc outBufDesc;
  VIDENC1_InArgs          *inArgs;
  VIDENC1_OutArgs         *outArgs;
  
  GstMapInfo info_in;
  GstMapInfo info_out;
  int outBufSizeArray[1];
  int status;
  
  inArgs = (VIDENC1_InArgs *)base_encoder->submitted_input_arguments;
  outArgs = (VIDENC1_OutArgs *)base_encoder->submitted_output_arguments;
  
  /* Access the data of the input and output buffer */
  if(!gst_buffer_map (input_buffer, &info_in, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT(base_encoder,"Can't access data from input buffer");
  }
    if(!gst_buffer_map (output_buffer, &info_out, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT(base_encoder,"Can't access data from output buffer");
  }
  
  /* Prepare the input buffer descriptor for the encode process */
  inBufDesc.frameWidth = GST_VIDEO_INFO_WIDTH(&GST_CE_BASE_VIDEO_ENCODER(base_encoder)->video_info);
  inBufDesc.frameHeight = GST_VIDEO_INFO_HEIGHT(&GST_CE_BASE_VIDEO_ENCODER(base_encoder)->video_info);
  inBufDesc.framePitch = GST_VIDEO_INFO_PLANE_STRIDE(&GST_CE_BASE_VIDEO_ENCODER(base_encoder)->video_info, 0);
  
  /* The next piece of code depend of the mime type of the buffer */
  inBufDesc.bufDesc[0].bufSize = gst_buffer_get_size(input_buffer);
  inBufDesc.bufDesc[0].buf = info_in.data;
  inBufDesc.bufDesc[1].bufSize = gst_buffer_get_size(input_buffer);
  inBufDesc.bufDesc[1].buf = info_in.data + (gst_buffer_get_size(input_buffer) * (2 / 3));
  inBufDesc.numBufs = 2;
  
  /* Prepare the output buffer descriptor for the encode process */
  outBufSizeArray[0]                  = gst_buffer_get_size(output_buffer);
  outBufDesc.numBufs                  = 1;
  outBufDesc.bufs                     = &(info_out.data);
  outBufDesc.bufSizes                 = outBufSizeArray;
  
  /* Set output and input arguments for the encode process */
  inArgs->size                         = sizeof(VIDENC1_InArgs);
  inArgs->inputID                      = 1;
  inArgs->topFieldFirstFlag            = 1;
  
  outArgs->size                        = sizeof(VIDENC1_OutArgs);
  
  /* Procees la encode and check for errors*/
  status = VIDENC1_processWait(base_encoder->codec_handle, &inBufDesc, &outBufDesc, inArgs, outArgs, timeout);

  if (status != VIDENC1_EOK) {
    GST_WARNING_OBJECT(base_encoder,"Incorrect return message from async encode process with extended error: 0x%x", 
      (unsigned int) outArgs->extendedError);
    return FALSE;
  }
    
  
  return TRUE;
}


static void
gst_ce_videnc1_class_init (GstCEVIDENC1Class * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  
  /* Obtain base class */
  GstCEBaseEncoderClass *base_encoder_class = GST_CE_BASE_ENCODER_CLASS(klass);
  
  GST_DEBUG_CATEGORY_INIT (gst_ce_videnc1_debug, "cevidenc1", 0,
      "Codec Engine VIDENC1 Class");
  
  gobject_class->set_property = gst_ce_videnc1_set_property;
  gobject_class->get_property = gst_ce_videnc1_get_property;
  
  /* Implementation of inherenci functions */  
  base_encoder_class->encoder_control = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_control);
  base_encoder_class->encoder_delete = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_delete);
  base_encoder_class->encoder_create = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_create);
  base_encoder_class->encoder_process_async = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_process_async);
  base_encoder_class->encoder_process_sync = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_process_sync);
  base_encoder_class->encoder_process_wait = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_process_wait);
  base_encoder_class->encoder_initialize_params = 
    GST_DEBUG_FUNCPTR(gst_ce_videnc1_initialize_params);
}

/* Implementation of alloc_params that alloc memory for static and dynamic params 
 * and set the values of some params */
static void
gst_ce_videnc1_alloc_params (GstCEBaseEncoder * base_encoder) {
  GST_DEBUG_OBJECT(base_encoder,"ENTER");
  VIDENC1_Params *params;
  VIDENC1_DynamicParams *dynamic_params;
  
  /* Allocate the static params */
  base_encoder->codec_params = g_malloc0(sizeof(VIDENC1_Params));
  if (base_encoder->codec_params == NULL) {
    GST_WARNING_OBJECT(base_encoder,"Failed to allocate VIDENC1_Params");
    return;
  }
  params = base_encoder->codec_params;
  /* Set default values for static params */
  params->size = sizeof(VIDENC1_Params);
  params->encodingPreset = XDM_DEFAULT;
  params->rateControlPreset = IVIDEO_RATECONTROLPRESET_DEFAULT;
  params->maxBitRate = 600000;
  params->dataEndianness = XDM_BYTE;
  params->maxInterFrameInterval = 1;
  params->reconChromaFormat = XDM_CHROMA_NA;
  
  /* Allocate the dynamic params */
  base_encoder->codec_dynamic_params = g_malloc0(sizeof(VIDENC1_DynamicParams));
  if (base_encoder->codec_dynamic_params == NULL) {
    GST_WARNING_OBJECT(base_encoder,"Failed to allocate VIDENC1_DynamicParams");
    return;
  }
  /* Set default values for dynamic params */
  dynamic_params = base_encoder->codec_dynamic_params;
  dynamic_params->size = sizeof(VIDENC1_DynamicParams);
  dynamic_params->targetBitRate = params->maxBitRate;
  dynamic_params->intraFrameInterval = params->maxInterFrameInterval;
  dynamic_params->generateHeader = XDM_ENCODE_AU;
  dynamic_params->forceFrame = IVIDEO_NA_FRAME;
  dynamic_params->interFrameInterval = 0;
  dynamic_params->mbDataFlag = 0;
  
  GST_DEBUG_OBJECT(base_encoder,"LEAVE");
}

/* Implementation of free_params that free the memory of
 * static and dynamic params */
static void
gst_ce_videnc1_free_params (GstCEBaseEncoder * base_encoder){
  if (base_encoder->codec_params != NULL) {
    g_free(base_encoder->codec_params);
    base_encoder->codec_params = NULL;
  }
  
  if (base_encoder->codec_dynamic_params != NULL) {
    g_free(base_encoder->codec_dynamic_params);
    base_encoder->codec_dynamic_params = NULL;
  }
}

/* init of the class */
static void
gst_ce_videnc1_init (GstCEBaseEncoder * base_encoder,
                     GstCEBaseEncoderClass *base_encoder_class) 
{
  GST_DEBUG("Enter init videnc1");  

  /* Allocate memory for the params and set some fields */
  gst_ce_videnc1_alloc_params(base_encoder);
  
  GST_DEBUG("Leave init videnc1");
}

/* NOT USE */
static void
gst_ce_videnc1_dispose (GObject *gobject) 
{

  GST_DEBUG_OBJECT(gobject,"ENTER");  
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER(gobject);
  
  gst_ce_videnc1_free_params(base_encoder);

  GST_DEBUG_OBJECT(base_encoder,"LEAVE");

  /* Chain up to the parent class */
  G_OBJECT_CLASS (&(CE_VIDENC1_GET_CLASS(gobject)->parent_class))->dispose(gobject);
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
      (GInstanceInitFunc)gst_ce_videnc1_init
    };

    object_type = g_type_register_static (GST_TYPE_CE_BASE_VIDEO_ENCODER,
        "GstCEVIDENC1", &object_info, (GTypeFlags) 0);
  }
  return object_type;
};
