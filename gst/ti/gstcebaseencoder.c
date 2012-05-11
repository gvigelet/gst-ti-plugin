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
gst_ce_base_encoder_finalize_attributes (GstCEBaseEncoder * base_encoder)
{

  if (base_encoder->submitted_input_buffers != NULL) {
    GstBuffer *input_buffer =
        (GstBuffer *) base_encoder->submitted_input_buffers;
    GstBuffer *output_buffer =
        (GstBuffer *) base_encoder->submitted_output_buffers;
    gst_buffer_unref (input_buffer);
    gst_buffer_unref (output_buffer);
    base_encoder->submitted_input_buffers = NULL;
    base_encoder->submitted_output_buffers = NULL;
  }

  base_encoder->first_buffer = FALSE;
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
gst_ce_base_encoder_default_init_codec (GstCEBaseEncoder * base_encoder)
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

/* Default implementation of the method that add meta data 
 * to the buffer */
static void
gst_ce_base_encoder_default_buffer_add_meta (GstBuffer * buffer)
{

  GstMapInfo info_buffer;

  GstMetaInfo *cmem_buffer_meta_info = gst_cmem_meta_get_info ();
  GstCMEMMeta *cmem_buffer_meta = (GstCMEMMeta *) gst_buffer_add_meta (buffer,
      cmem_buffer_meta_info, NULL);

  /* Access the info of the buffer for obtain the  physical address */
  if (!gst_buffer_map (buffer, &info_buffer, GST_MAP_WRITE)) {
    GST_WARNING ("Can't access data from buffer");
  }

  cmem_buffer_meta->physical_address =
      Memory_getBufferPhysicalAddress (info_buffer.data,
      gst_buffer_get_size (buffer), NULL);

  if (cmem_buffer_meta->physical_address == 0) {
    GST_WARNING ("Physical address could not be obtained.");
  }

}

static GstBuffer *
gst_ce_base_encoder_default_post_process (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer)
{
  return buffer;
}

static GstBuffer *
gst_ce_base_encoder_default_pre_process (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer)
{
  return buffer;
}

/* Process the encode algorithm */
static GstBuffer *
gst_ce_base_encoder_default_encode (GstCEBaseEncoder * base_encoder)
{
  GST_DEBUG_OBJECT (base_encoder, "Entry");

  GstBuffer *encoded_buffer = NULL;
  GstBuffer *input_buffer;
  GstBuffer *output_buffer;
  GstBuffer *push_out_buffer;

  /* Reuse the input and output buffers */
  input_buffer = (GstBuffer *) base_encoder->submitted_input_buffers;
  output_buffer = (GstBuffer *) base_encoder->submitted_output_buffers;


  /* Give the chance of transform the buffer before being encode */
  input_buffer = gst_ce_base_encoder_pre_process (base_encoder, input_buffer);

  /* Encode the buffer */
  encoded_buffer =
      gst_ce_base_encoder_process_sync (base_encoder, input_buffer,
      output_buffer);

  /* Permit to transform encode buffer before push out */
  push_out_buffer =
      gst_ce_base_encoder_post_process (base_encoder, output_buffer);

  GST_DEBUG_OBJECT (base_encoder, "Leave");

  return encoded_buffer;

}


static void
gst_ce_base_encoder_class_init (GstCEBaseEncoderClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  /* Init debug instancbase_encoder_finalize_codece */
  GST_DEBUG_CATEGORY_INIT (gst_ce_base_encoder_debug, "cebaseencoder", 0,
      "CodecEngine base encoder class");

  GST_DEBUG ("ENTER");

  /* Instance the class methods */
  klass->base_encoder_encode = gst_ce_base_encoder_default_encode;
  klass->base_encoder_finalize_codec =
      gst_ce_base_encoder_default_finalize_codec;
  klass->base_encoder_init_codec = gst_ce_base_encoder_default_init_codec;
  klass->base_encoder_buffer_add_meta =
      gst_ce_base_encoder_default_buffer_add_meta;
  klass->base_encoder_post_process = gst_ce_base_encoder_default_post_process;
  klass->base_encoder_pre_process = gst_ce_base_encoder_default_pre_process;

  /* Override inheritance methods */
  gobject_class->set_property = gst_ce_base_encoder_set_property;
  gobject_class->get_property = gst_ce_base_encoder_get_property;
  gobject_class->finalize = gst_ce_base_encoder_finalize;


  GST_DEBUG ("LEAVING");

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
  base_encoder->submitted_input_buffers = NULL;
  base_encoder->submitted_output_buffers = NULL;
  base_encoder->submitted_push_out_buffers = NULL;
  base_encoder->first_buffer = FALSE;

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
gst_ce_base_encoder_get_cmem_buffer (GstCEBaseEncoder * base_encoder,
    gsize size)
{
  GstBuffer *buffer = NULL;
  GstAllocator *buffer_allocator;

  /* Obtain the allocator for the new buffer */
  buffer_allocator = gst_allocator_find ("ContiguosMemory");
  if (buffer_allocator == NULL) {
    GST_DEBUG_OBJECT (base_encoder, "Can't find the buffer allocator");
  }

  /* Allocate the input buffer with alignment of 4 bytes and with default buffer size */
  buffer = gst_buffer_new_allocate (buffer_allocator, size, 3);

  if (buffer == NULL) {
    GST_DEBUG_OBJECT (base_encoder,
        "Memory couldn't be allocated for input buffer");
  }

  /* Add meta data to the buffer */
  gst_ce_base_encoder_buffer_add_meta (base_encoder, buffer);

  return buffer;
}

/* Reconfig the output buffer size of the encoder */
void
gst_ce_base_encoder_shrink_output_buffer (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, gsize new_size)
{
}
