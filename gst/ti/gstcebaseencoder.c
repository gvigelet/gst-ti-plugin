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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gstcebasevideoencoder.h>
#include <gstcebaseencoder.h>
#include <string.h>
#include <stdlib.h>
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/video1/videnc1.h>


#define GST_CAT_DEFAULT gst_ce_base_encoder_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);


enum
{
  PROP_0,
  PROP_A
};

static void
gst_ce_base_encoder_base_init (GstCEBaseEncoderClass * klass)
{
  /* Initialize dynamic data */
}

static void
gst_ce_base_encoder_base_finalize (GstCEBaseEncoderClass * klass)
{
}

static void
gst_ce_base_encoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GST_DEBUG_OBJECT(object,"Entry to set_property base encoder");
  /* Set base params */
  switch (prop_id) {
    default:
      break;
  }
  
  GST_DEBUG_OBJECT(object,"Leave set_property base encoder");
}

static void
gst_ce_base_encoder_finalize (GObject * object)
{
}

static void
gst_ce_base_encoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{

  /* Get base params */
  switch (prop_id) {
    default:
      break;
  }
}


static void
gst_ce_base_encoder_class_init (GstCEBaseEncoderClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  
  /* Init debug instance */
  GST_DEBUG_CATEGORY_INIT (gst_ce_base_encoder_debug, "cebaseencoder", 0,
      "CodecEngine base encoder class");
  
  /* Override inheritance methods */
  gobject_class->set_property = gst_ce_base_encoder_set_property;
  gobject_class->get_property = gst_ce_base_encoder_get_property;
  gobject_class->finalize = gst_ce_base_encoder_finalize;
  
  /* Install properties for the class */
  g_object_class_install_property (gobject_class, PROP_A,
      g_param_spec_boolean ("testa", "Test A", "Testing A", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  
  
}

void
gst_ce_base_encoder_class_finalize (GstCEBaseEncoderClass * klass,
    gpointer * class_data)
{
}

static void
gst_ce_base_encoder_init (GstCEBaseEncoder * base_encoder, 
                          GstCEBaseEncoderClass * base_encode_class)
{
  GST_DEBUG_OBJECT(base_encoder,"Entry _init base encoder");
  
  /* Init the attributes */
  base_encoder->codec_params = NULL;
  base_encoder->codec_dynamic_params = NULL;
  base_encoder->codec_handle = NULL;
  base_encoder->submitted_input_arguments = g_malloc0(sizeof(VIDENC1_InArgs));
  base_encoder->submitted_output_arguments = g_malloc0(sizeof(VIDENC1_OutArgs));
  
  /* Obtain the allocator */
  GstAllocator *buffer_allocator = gst_allocator_find("ContiguosMemory");
  if(buffer_allocator == NULL) {
    GST_DEBUG_OBJECT(base_encoder, "Can't find the buffer allocator");
  }
  
  /* Allocate the input buffer with alignment of 4 bytes and with default buffer size */
  g_print("Antes\n");
  base_encoder->submitted_input_buffers = gst_buffer_new_allocate (buffer_allocator, DEFAULT_BUF_SIZE, 3);
  g_print("Despues\n");
  
  if(base_encoder->submitted_input_buffers == NULL) {
    GST_DEBUG_OBJECT(base_encoder,"Memory couldn't be allocated for input buffer");
  }
  
  /* Allocate the output buffer with alignment of 4 bytes */
  base_encoder->submitted_output_buffers = gst_buffer_new_allocate (buffer_allocator, DEFAULT_BUF_SIZE, 3);
  if(base_encoder->submitted_output_buffers == NULL) {
    GST_DEBUG_OBJECT(base_encoder,"Memory couldn't be allocated for output buffer");
  }
  
  GST_DEBUG_OBJECT(base_encoder,"Leave _init base encoder");
  
}

/* Obtain and register the type of the class */
GType
gst_ce_base_encoder_get_type (void)
{
  
  static GType object_type = 0;

  if (object_type == 0) {

    static const GTypeInfo object_info = {
      sizeof (GstCEBaseEncoderClass),
      (GBaseInitFunc) gst_ce_base_encoder_base_init,
      (GBaseFinalizeFunc) gst_ce_base_encoder_base_finalize,
      (GClassInitFunc) gst_ce_base_encoder_class_init,
      NULL,
      NULL,
      sizeof (GstCEBaseEncoder),
      0,
      (GInstanceInitFunc) gst_ce_base_encoder_init,
    };
    
    object_type = g_type_register_static (GST_TYPE_ELEMENT,
        "GstCEBaseEncoder", &object_info, 0);
  }

  return object_type;
};

/** @brief gst_ce_base_encoder_get_output_buffer
 *  @param base_encoder : a #GstCEBaseEncoder object
 *  @param size : size for the buffer
 *
 *  @return a #GstBuffer 
 */
GstBuffer *
gst_ce_base_encoder_get_output_buffer (GstCEBaseEncoder * base_encoder,
    gsize size)
{
  return NULL;
}

/* Reconfig the output buffer size of the encoder */
void
gst_ce_base_encoder_shrink_output_buffer (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, gsize new_size)
{
}

/* Process the encode algorithm */
void
gst_ce_base_encoder_encode (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer)
{
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
  //g_print("Size before: %d\n", gst_buffer_get_size(output_buffer));
  status = VIDENC1_process(base_encoder->codec_handle, &inBufDesc, &outBufDesc, inArgs, outArgs);

    
  /* Re alloc the output buffer with the encode data */
  //output_buffer = gst_buffer_new_and_alloc(outArgs.encodedBuf.bufSize);
  //gst_buffer_fill (output_buffer, 0, outArgs.encodedBuf.buf, outArgs.encodedBuf.bufSize);
  //g_print("Size after: %d\n", gst_buffer_get_size(output_buffer));
      
  if (status != VIDENC1_EOK) {
    GST_WARNING_OBJECT(base_encoder,"Incorrect encode process with extended error: 0x%x", 
      (unsigned int) outArgs->extendedError);
  }
  
}
