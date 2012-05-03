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
#include <pthread.h>


#define GST_CAT_DEFAULT gst_ce_base_encoder_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);


enum
{
  PROP_0,
};


/* Free any previus definition of the principal attributes */
gboolean 
gst_ce_base_encoder_finalize_attributes (GstCEBaseEncoder * base_encoder) {
  
  if(base_encoder->submitted_input_buffers != NULL) {
    GstBuffer *input_buffer = (GstBuffer *)base_encoder->submitted_input_buffers;
    GstBuffer *output_buffer = (GstBuffer *)base_encoder->submitted_output_buffers;
    gst_buffer_unref(input_buffer);
    gst_buffer_unref(output_buffer);
    base_encoder->submitted_input_buffers = NULL;
    base_encoder->submitted_output_buffers = NULL;
  }
  
  if(base_encoder->codec_data != NULL) {
    GstBuffer *codec_data = (GstBuffer *)base_encoder->codec_data;
    gst_buffer_unref(codec_data);
    base_encoder->codec_data = NULL;
  }
}

/* Free the codec instance in case of no null */
static gboolean
gst_ce_base_encoder_default_finalize_codec (GstCEBaseEncoder * base_encoder)
{

  if (base_encoder->codec_handle != NULL) {
    if (!gst_ce_base_encoder_delete (base_encoder))
      return FALSE;
  }

  return TRUE;
}

/* Check for a previews instance of the codec, init the params and create the codec instance */
static gboolean
gst_ce_base_encoder_default_init_codec (GstCEBaseEncoder *base_encoder)
{

  /* Finalize any previous configuration  */
  if (!gst_ce_base_encoder_finalize_codec (base_encoder))
    return FALSE;

  /* Set the value of the params */
  if (!gst_ce_base_encoder_initialize_params (base_encoder))
    return FALSE;

  /* Give a chance to downstream caps to modify the params or dynamic
   * params before we use them
   */
  //if (! CE_BASE_VIDEO_ENCODER_GET_CLASS(video_encoder)->(
  //  video_encoder))
  //return FALSE;

  /* Create the codec instance */
  if (!gst_ce_base_encoder_create (base_encoder))
    return FALSE;

  return TRUE;
}

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
  GST_DEBUG_OBJECT (object, "Entry to set_property base encoder");
  /* Set base params */
  switch (prop_id) {
    default:
      break;
  }

  GST_DEBUG_OBJECT (object, "Leave set_property base encoder");
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

/* Process the encode algorithm */
static void
gst_ce_base_encoder_default_encode (GstCEBaseEncoder * base_encoder,
  GstBuffer * raw_data_buffer)
{
  GST_DEBUG_OBJECT (base_encoder, "Entry");
  
  GstBuffer *buffer_push_out = NULL;
  GstBuffer *input_buffer;
  GstBuffer *output_buffer;
  pthread_t processAsyncthread;
  gboolean processAsyncReturn;
  GstMapInfo info_raw_data;
  GstMapInfo info_output_buffer;
  
  /* Reuse the input and output buffers */
  input_buffer =
      (GstBuffer *)base_encoder->submitted_input_buffers;
  output_buffer =
      (GstBuffer *) base_encoder->submitted_output_buffers;
  
    
  static gboolean first_buffer = TRUE;

  if (!gst_buffer_map (raw_data_buffer, &info_raw_data, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT (base_encoder, "Can't access data from buffer");
  }

  /* Entry only once */
  if (first_buffer == TRUE) {
    
    /* Give the chance of generate codec data */
    if(base_encoder->codec_data == NULL) {
      /* Process the buffer asynchronously*/
      gst_ce_base_encoder_process_sync(base_encoder, input_buffer, output_buffer); 
      
      
      /* Prepare the push output buffer */
      VIDENC1_OutArgs outArgs =
          *((VIDENC1_OutArgs *) base_encoder->submitted_output_arguments);
      
      buffer_push_out = gst_buffer_new_and_alloc (outArgs.bytesGenerated);
      
      if (!gst_buffer_map (output_buffer, &info_output_buffer, GST_MAP_WRITE)) {
       GST_DEBUG_OBJECT (base_encoder, "Can't access data from output buffer");
      }
      
      gst_buffer_fill (buffer_push_out, 0, info_output_buffer.data,
          outArgs.bytesGenerated);
      
      /* Generate the codec data with the encoded buffer */
      base_encoder->codec_data = gst_ce_base_encoder_generate_codec_data(base_encoder, buffer_push_out);
      
      if(base_encoder->codec_data == NULL){
        GST_WARNING_OBJECT (base_encoder,
            "Can't obtain codec data");
        first_buffer = FALSE;
      }
      else {
        
        /* Set the caps with the new codec_data */
        GstCaps* caps = gst_caps_make_writable(gst_pad_get_current_caps (base_encoder->src_pad));
        gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, base_encoder->codec_data, NULL);
        gboolean ret = gst_pad_set_caps (base_encoder->src_pad, caps);
        if (ret == FALSE) {
          GST_WARNING_OBJECT (base_encoder, "Caps can't be set", ret);
        }        
      }
      
    }
    else {
      /************************************************/
      /* Prepare for init asynchronous encode process */
      /************************************************/
      
      /* Prepare output and input buffers */
      gsize bytes_copied = gst_buffer_fill (input_buffer, 0, info_raw_data.data,
          gst_buffer_get_size (raw_data_buffer));
      if (bytes_copied == 0) {
        GST_WARNING_OBJECT (base_encoder,
            "Can't obtain data from buffer to input buffer");
      }
      
      /* Prepare the arguments for the asynchronous process */
      base_encoder->process_async_arguments->base_encoder = base_encoder;
      base_encoder->process_async_arguments->input_buffer = input_buffer;
      base_encoder->process_async_arguments->output_buffer = output_buffer;
      
      /* Prepare the thread for encode and init the process */
      int ret = pthread_create (base_encoder->process_async_thread, NULL,
          CE_BASE_ENCODER_GET_CLASS (base_encoder)->base_encoder_process_async,
          base_encoder->process_async_arguments);
      
      first_buffer = FALSE;
    }
    
    
  } else {
    /* wait for previus call of processAsync */
    pthread_join (*(base_encoder->process_async_thread), NULL);

    /* Prepare the push output buffer */
    VIDENC1_OutArgs outArgs =
        *((VIDENC1_OutArgs *) base_encoder->submitted_output_arguments);
      
    buffer_push_out = gst_buffer_new_and_alloc (outArgs.bytesGenerated);
    
    if (!gst_buffer_map (output_buffer, &info_output_buffer, GST_MAP_WRITE)) {
       GST_DEBUG_OBJECT (base_encoder, "Can't access data from output buffer");
    }
    
    gst_buffer_fill (buffer_push_out, 0, info_output_buffer.data,
        outArgs.bytesGenerated);
    
    /* Prepare output and input buffers */
    gsize bytes_copied = gst_buffer_fill (input_buffer, 0, info_raw_data.data,
        gst_buffer_get_size (raw_data_buffer));
    if (bytes_copied == 0) {
      GST_DEBUG_OBJECT (base_encoder,
          "Can't obtain data from buffer to input buffer");
    }
    /* Prepare the arguments for the asynchronous process */
    base_encoder->process_async_arguments->base_encoder = base_encoder;
    base_encoder->process_async_arguments->input_buffer = input_buffer;
    base_encoder->process_async_arguments->output_buffer = output_buffer;

    /* Prepare the thread for encode and init the process */
    int ret = pthread_create (base_encoder->process_async_thread, NULL,
        CE_BASE_ENCODER_GET_CLASS (base_encoder)->base_encoder_process_async,
        base_encoder->process_async_arguments);

  }
  
  base_encoder->submitted_push_out_buffers = buffer_push_out;

  GST_DEBUG_OBJECT (base_encoder, "Leave");
  

}


static void
gst_ce_base_encoder_class_init (GstCEBaseEncoderClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  /* Init debug instance */
  GST_DEBUG_CATEGORY_INIT (gst_ce_base_encoder_debug, "cebaseencoder", 0,
      "CodecEngine base encoder class");

  GST_DEBUG ("ENTER");
  
  /* Instance the class methods */
  klass->base_encoder_encode = gst_ce_base_encoder_default_encode;
  klass->base_encoder_finalize_codec = gst_ce_base_encoder_default_finalize_codec;
  klass->base_encoder_init_codec = gst_ce_base_encoder_default_init_codec;
  
  /* Override inheritance methods */
  gobject_class->set_property = gst_ce_base_encoder_set_property;
  gobject_class->get_property = gst_ce_base_encoder_get_property;
  gobject_class->finalize = gst_ce_base_encoder_finalize;


  GST_DEBUG ("LEAVER");

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
  GST_DEBUG_OBJECT (base_encoder, "Entry _init base encoder");

  /* Init the attributes */
  base_encoder->codec_params = NULL;
  base_encoder->codec_dynamic_params = NULL;
  base_encoder->codec_handle = NULL;
  base_encoder->submitted_input_arguments = g_malloc0 (sizeof (VIDENC1_InArgs));
  base_encoder->submitted_output_arguments =
      g_malloc0 (sizeof (VIDENC1_OutArgs));
  base_encoder->codec_data = NULL;
  base_encoder->submitted_input_buffers = NULL;
  base_encoder->submitted_output_buffers = NULL;
  base_encoder->submitted_push_out_buffers = NULL;
  base_encoder->codec_data = NULL;

  /* Allocate the thread for the async encode process */
  base_encoder->process_async_thread = g_malloc0 (sizeof (pthread_t));

  /* Allocate the arguments for the async encode process */
  base_encoder->process_async_arguments = g_malloc0 (sizeof (processAsyncArguments));

  GST_DEBUG_OBJECT (base_encoder, "Leave _init base encoder");

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


