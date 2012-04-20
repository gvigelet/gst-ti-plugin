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
  GST_DEBUG_OBJECT (base_encoder, "Entry _init base encoder");

  /* Init the attributes */
  base_encoder->codec_params = NULL;
  base_encoder->codec_dynamic_params = NULL;
  base_encoder->codec_handle = NULL;
  base_encoder->submitted_input_arguments = g_malloc0 (sizeof (VIDENC1_InArgs));
  base_encoder->submitted_output_arguments =
      g_malloc0 (sizeof (VIDENC1_OutArgs));
  base_encoder->submitted_push_output_buffers = NULL;
  base_encoder->codec_data = NULL;
  base_encoder->submitted_input_buffers = NULL;
  base_encoder->submitted_output_buffers = NULL;

  /* Allocate the push out put buffer */
  //base_encoder->submitted_push_output_buffers = gst_buffer_new_and_alloc(DEFAULT_BUF_SIZE);
  //if(base_encoder->submitted_push_output_buffers == NULL) {
  // GST_DEBUG_OBJECT(base_encoder,"Memory couldn't be allocated for put output buffer");
  //}

  /* Allocate the thread for the async encode process */
  base_encoder->processAsyncthread = g_malloc0 (sizeof (pthread_t));

  /* Allocate the arguments for the async encode process */
  base_encoder->arguments = g_malloc0 (sizeof (processAsyncArguments));

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

/* Process the encode algorithm */
GstFlowReturn
gst_ce_base_encoder_encode (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer, GstBuffer * raw_data)
{

  int ret;
  GstBuffer *buffer_push_out = NULL;
  pthread_t processAsyncthread;
  gboolean processAsyncReturn;
  GstMapInfo info_raw_data;

  static gboolean first_buffer = TRUE;

  if (!gst_buffer_map (raw_data, &info_raw_data, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT (base_encoder, "Can't access data from buffer");
  }

  /* Entry only once */
  if (first_buffer == TRUE) {

    /* Prepare output and input buffers */
    gsize bytes_copied = gst_buffer_fill (input_buffer, 0, info_raw_data.data,
        gst_buffer_get_size (raw_data));
    if (bytes_copied == 0) {
      GST_DEBUG_OBJECT (base_encoder,
          "Can't obtain data from buffer to input buffer");
    }

    /* Prepare the arguments for the asynchronous process */
    base_encoder->arguments->base_encoder = base_encoder;
    base_encoder->arguments->input_buffer = input_buffer;
    base_encoder->arguments->output_buffer = output_buffer;

    /* Prepare the thread for encode and init the process */
    int ret = pthread_create (base_encoder->processAsyncthread, NULL,
        CE_BASE_ENCODER_GET_CLASS (base_encoder)->encoder_process_async,
        base_encoder->arguments);

    first_buffer = FALSE;
  } else {
    /* wait for previus call of processAsync */
    pthread_join (*(base_encoder->processAsyncthread), NULL);

    /* Prepare the push output buffer */
    VIDENC1_OutArgs outArgs =
        *((VIDENC1_OutArgs *) base_encoder->submitted_output_arguments);

    buffer_push_out = gst_buffer_new_and_alloc (outArgs.encodedBuf.bufSize);
    gst_buffer_fill (buffer_push_out, 0, outArgs.encodedBuf.buf,
        outArgs.encodedBuf.bufSize);

    /* Prepare output and input buffers */
    gsize bytes_copied = gst_buffer_fill (input_buffer, 0, info_raw_data.data,
        gst_buffer_get_size (raw_data));
    if (bytes_copied == 0) {
      GST_DEBUG_OBJECT (base_encoder,
          "Can't obtain data from buffer to input buffer");
    }
    //* Prepare the arguments for the asynchronous process */
    base_encoder->arguments->base_encoder = base_encoder;
    base_encoder->arguments->input_buffer = input_buffer;
    base_encoder->arguments->output_buffer = output_buffer;

    /* Prepare the thread for encode and init the process */
    int ret = pthread_create (base_encoder->processAsyncthread, NULL,
        CE_BASE_ENCODER_GET_CLASS (base_encoder)->encoder_process_async,
        base_encoder->arguments);

  }

  if (buffer_push_out == NULL) {
    /* Only obtain the next buffer */
    ret = GST_FLOW_OK;
  } else {

    /* Copy extra data from the original buffer to the push out buffer */
    gst_buffer_copy_into (buffer_push_out, raw_data, GST_BUFFER_COPY_META, 0,
        gst_buffer_get_size (raw_data));
    gst_buffer_copy_into (buffer_push_out, raw_data, GST_BUFFER_COPY_TIMESTAMPS,
        0, gst_buffer_get_size (raw_data));
    gst_buffer_copy_into (buffer_push_out, raw_data, GST_BUFFER_COPY_FLAGS, 0,
        -1);

    /*push the buffer and check for any error */
    ret = gst_pad_push (base_encoder->src_pad, buffer_push_out);

    if (GST_FLOW_OK != ret) {
      GST_ERROR_OBJECT (base_encoder, "Push buffer return with error: %d", ret);
    }
  }

  return ret;

}
