/*
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
#include <gstcmemallocator.h>



#define GST_CAT_DEFAULT gst_ce_base_encoder_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);





enum
{
  PROP_0,
  PROP_SIZE_OUTPUT_BUF,
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
  
  if(base_encoder->freeSlices != NULL) {
    base_encoder->freeMutex = NULL;
    base_encoder->freeSlices = NULL;
    base_encoder->outBufSize = 0;
  }
  base_encoder->first_buffer = FALSE;
  
  return TRUE;
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
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (object);
  GST_DEBUG_OBJECT (base_encoder, "Entry to set_property base encoder");
  /* Set base params */
  switch (prop_id) {
    case PROP_SIZE_OUTPUT_BUF:
      base_encoder->outBufSize = g_value_get_int (value);
      GST_LOG ("setting \"outBufSize\" to \"%d\"\n", base_encoder->outBufSize);
      break;
    default:
      break;
  }

  GST_DEBUG_OBJECT (base_encoder, "Leave set_property base encoder");
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
gst_ce_base_encoder_default_buffer_add_cmem_meta (GstBuffer * buffer)
{

  GstMapInfo info_buffer;
  const GstMetaInfo *cmem_buffer_meta_info;

  cmem_buffer_meta_info = gst_cmem_meta_get_info ();
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

/* Release la unused memory to the correspond slice of free memory */
void
gst_ce_base_encoder_restore_unused_memory (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, GList **actual_free_slice)
{

  gint unused;
  struct cmemSlice *slice;

  /* Change the size of the buffer */
  GstMemory *buffer_mem = gst_buffer_get_memory (buffer, 0);
  buffer_mem->size = base_encoder->memoryUsed;
  buffer_mem->maxsize = base_encoder->memoryUsed;

  g_mutex_lock (base_encoder->freeMutex);
  /* Return unused memory */
  unused =
      gst_buffer_get_size (base_encoder->submitted_input_buffers) -
      base_encoder->memoryUsed;
  slice = (struct cmemSlice *) ((*actual_free_slice)->data);
  slice->start -= unused;
  slice->size += unused;
  if (slice->size == 0) {
    g_free (slice);
    base_encoder->freeSlices =
        g_list_delete_link (base_encoder->freeSlices, *actual_free_slice);
  }
  g_mutex_unlock (base_encoder->freeMutex);

}


/* Default implementation of the post_process method */
static GstBuffer *
gst_ce_base_encoder_default_post_process (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, GList ** actual_free_slice)
{

  /* Restore unused memory after encode */
  gst_ce_base_encoder_restore_unused_memory (base_encoder, buffer,
      actual_free_slice);

  return buffer;
}

static GstBuffer *
gst_ce_base_encoder_default_pre_process (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, GList ** actual_free_slice)
{

  GstBuffer *output_buffer;

  /*Obtain the slice of the output buffer to use */
  output_buffer =
      gst_ce_base_encoder_get_output_buffer (base_encoder, actual_free_slice);

  return output_buffer;
}

/* Obtain the free memory slide for being use */
GList *
gst_ce_base_encoder_get_valid_slice (GstCEBaseEncoder * base_encoder,
    gint * size)
{

  GList *first_fit, *alternative_fit;
  struct cmemSlice *slice, *maxSliceAvailable;
  int maxSize = 0;

  /* Find free memory */
  GST_DEBUG ("Finding free memory");
  g_mutex_lock (base_encoder->freeMutex);
  first_fit = base_encoder->freeSlices;
  while (first_fit) {
    slice = (struct cmemSlice *) first_fit->data;
    GST_DEBUG ("Evaluating free slice from %d to %d", slice->start, slice->end);
    if (slice->size >= *size) {
      /* We mark all the memory as buffer at this point
       * to avoid merges while we are using the area
       * Once we know how much memory we actually used, we 
       * update to the real memory size that was used
       */
      slice->start += *size;
      slice->size -= *size;
      g_mutex_unlock (base_encoder->freeMutex);
      return first_fit;
    }
    if (slice->size > maxSize) {
      maxSliceAvailable = slice;
      maxSize = slice->size;
      alternative_fit = first_fit;
    }

    first_fit = g_list_next (first_fit);
  }
  GST_WARNING
      ("Free memory not found, using our best available free block of size %d...",
      *size);

  maxSliceAvailable->start += maxSliceAvailable->size;
  *size = maxSliceAvailable->size;
  maxSliceAvailable->size = 0;

  g_mutex_unlock (base_encoder->freeMutex);
  return alternative_fit;
}


static gboolean
gst_ce_base_encoder_buffer_dispose (GstMiniObject * obj)
{

  gpointer state = NULL;
  GstMapInfo info_buffer;
  GstMapInfo info_outbuffer;
  GstCMEMMeta *buffer_meta_data;
  GstBuffer *buf;

  /* Obtain the meta data from the buffer */
  buf = GST_BUFFER (obj);
  buffer_meta_data = (GstCMEMMeta *) gst_buffer_iterate_meta (buf, &state);
  GstCEBaseEncoder *base_encoder = buffer_meta_data->base_encoder;


  /* Access the information of the buffer */
  GstMemory *buffer_memory = gst_buffer_get_memory (buf, 0);
  if (!gst_memory_map (buffer_memory, &info_buffer, GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base_encoder, "Can't access data from buffer");
  }

  /* Access the information of the circular buffer */
  if (!gst_buffer_map (base_encoder->submitted_output_buffers,
          &info_outbuffer, GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base_encoder, "Can't access data from buffer");
  }

  if (base_encoder->freeMutex)
    g_mutex_lock (base_encoder->freeMutex);

  if (base_encoder->submitted_output_buffers == NULL
      || base_encoder->freeSlices == NULL) {
    GST_DEBUG ("Releasing memory after memory structures were freed");
    /* No need for unlock, since it wasn't taked */
    return TRUE;
  }
  gint spos = info_buffer.data - info_outbuffer.data;
  gint buffer_size = info_buffer.size;
  gint epos = spos + buffer_size;
  struct cmemSlice *slice, *nslice;
  GList *actual_element;

  if (!epos > gst_buffer_get_size (base_encoder->submitted_output_buffers)) {
    GST_ELEMENT_ERROR (base_encoder, RESOURCE, NO_SPACE_LEFT, (NULL),
        ("Releasing buffer how ends outside memory boundaries"));
    return FALSE;
  }

  GST_DEBUG ("Releasing memory from %d to %d", spos, epos);
  actual_element = base_encoder->freeSlices;

  /* Merge free memory */
  while (actual_element) {
    slice = (struct cmemSlice *) actual_element->data;

    /* Are we contigous to this block? */
    if (slice->start == epos) {
      GST_DEBUG ("Merging free buffer at beggining free block (%d,%d)",
          slice->start, slice->end);
      /* Merge with current block */
      slice->start -= buffer_size;
      slice->size += buffer_size;
      /* Merge with previous block? */
      if (g_list_previous (actual_element)) {
        nslice = (struct cmemSlice *) g_list_previous (actual_element)->data;
        if (nslice->end == slice->start) {
          GST_DEBUG ("Closing gaps...");
          nslice->end += slice->size;
          nslice->size += slice->size;
          g_free (slice);
          base_encoder->freeSlices =
              g_list_delete_link (base_encoder->freeSlices, actual_element);
        }
      }
      g_mutex_unlock (base_encoder->freeMutex);
      return TRUE;
    }
    if (slice->end == spos) {
      GST_DEBUG ("Merging free buffer at end of free block (%d,%d)",
          slice->start, slice->end);
      /* Merge with current block */
      slice->end += buffer_size;
      slice->size += buffer_size;
      /* Merge with next block? */
      if (g_list_next (actual_element)) {
        nslice = (struct cmemSlice *) g_list_next (actual_element)->data;
        if (nslice->start == slice->end) {
          GST_DEBUG ("Closing gaps...");
          slice->end += nslice->size;
          slice->size += nslice->size;
          g_free (nslice);
          base_encoder->freeSlices =
              g_list_delete_link (base_encoder->freeSlices,
              g_list_next (actual_element));
        }
      }
      g_mutex_unlock (base_encoder->freeMutex);
      return TRUE;
    }
    /* Create a new free slice */
    if (slice->start > epos) {
      GST_DEBUG ("Creating new free slice %d,%d before %d,%d", spos, epos,
          slice->start, slice->end);
      nslice = g_malloc0 (sizeof (struct cmemSlice));
      nslice->start = spos;
      nslice->end = epos;
      nslice->size = buffer_size;
      base_encoder->freeSlices =
          g_list_insert_before (base_encoder->freeSlices, actual_element,
          nslice);
      g_mutex_unlock (base_encoder->freeMutex);
      return TRUE;
    }

    actual_element = g_list_next (actual_element);
  }

  GST_DEBUG ("Creating new free slice %d,%d at end of list", spos, epos);
  /* We reach the end of the list, so we append the free slice at the 
     end
   */
  nslice = g_malloc0 (sizeof (struct cmemSlice));
  nslice->start = spos;
  nslice->end = epos;
  nslice->size = buffer_size;
  base_encoder->freeSlices =
      g_list_insert_before (base_encoder->freeSlices, NULL, nslice);
  g_mutex_unlock (base_encoder->freeMutex);

  return TRUE;
}


/* Obtain the out put buffer of the encoder */
GstBuffer *
gst_ce_base_encoder_get_output_buffer (GstCEBaseEncoder * base_encoder,
    GList ** actual_free_slice)
{

  struct cmemSlice *slice;
  gint offset;
  gint size = gst_buffer_get_size (base_encoder->submitted_input_buffers);
  GstMapInfo info_output;
  GstBuffer *output_buffer;
  GstMemory *memory;
  GstAllocator *buffer_allocator;
  const GstMetaInfo *cmem_buffer_meta_info;

  /* Search for valid free slice of memory */
  *actual_free_slice =
      gst_ce_base_encoder_get_valid_slice (base_encoder, &size);
  if (!*actual_free_slice) {
    GST_WARNING_OBJECT (base_encoder,
        "Not enough space free on the output buffer");
    return NULL;
  }
  slice = (struct cmemSlice *) ((*actual_free_slice)->data);


  /* The offset was already reserved, so we need to correct the start */
  offset = slice->start - size;

  /* Prepare the output buffer */
  output_buffer = gst_buffer_new ();
  buffer_allocator = gst_allocator_find ("ContiguosMemory");
  memory = gst_allocator_alloc (buffer_allocator, 0, 0);
  memory->size = gst_buffer_get_size (base_encoder->submitted_input_buffers);
  memory->maxsize = memory->size;

  /* FIXME: can change in a more efficiently mode */
  if (!gst_buffer_map (base_encoder->submitted_output_buffers, &info_output,
          GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base_encoder, "Can't access data from buffer");
  }

  gst_cmem_allocator_set_data (memory, info_output.data + offset);
  gst_buffer_take_memory (output_buffer, 0, memory);

  /* Copy extra data from the original buffer to the push out buffer */
  cmem_buffer_meta_info = gst_cmem_meta_get_info ();
  GstCMEMMeta *cmem_buffer_meta =
      (GstCMEMMeta *) gst_buffer_add_meta (output_buffer,
      cmem_buffer_meta_info, NULL);

  cmem_buffer_meta->base_encoder = base_encoder;

  /* Set the dispose function */
  GST_MINI_OBJECT (output_buffer)->dispose = gst_ce_base_encoder_buffer_dispose;

  return output_buffer;
}


/* Process the encode algorithm */
static GstBuffer *
gst_ce_base_encoder_default_encode (GstCEBaseEncoder * base_encoder)
{
  GST_DEBUG_OBJECT (base_encoder, "Entry");

  GstBuffer *encoded_buffer;
  GstBuffer *input_buffer;
  GstBuffer *output_buffer;
  GstBuffer *push_out_buffer;
  GList *actual_free_slice;

  /* Reuse the input and output buffers */
  input_buffer = (GstBuffer *) base_encoder->submitted_input_buffers;

  /* Give the chance of transform the buffer before being encode */
  output_buffer =
      gst_ce_base_encoder_pre_process (base_encoder, input_buffer,
      &actual_free_slice);

  /* Encode the buffer */
  encoded_buffer =
      gst_ce_base_encoder_process_sync (base_encoder, input_buffer,
      output_buffer);

  /* Permit to transform encode buffer before push out */
  push_out_buffer =
      gst_ce_base_encoder_post_process (base_encoder, encoded_buffer,
      &actual_free_slice);

  GST_DEBUG_OBJECT (base_encoder, "Leave");

  return push_out_buffer;

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
  klass->base_encoder_buffer_add_cmem_meta =
      gst_ce_base_encoder_default_buffer_add_cmem_meta;
  klass->base_encoder_post_process = gst_ce_base_encoder_default_post_process;
  klass->base_encoder_pre_process = gst_ce_base_encoder_default_pre_process;

  /* Override inheritance methods */
  gobject_class->set_property = gst_ce_base_encoder_set_property;
  gobject_class->get_property = gst_ce_base_encoder_get_property;
  gobject_class->finalize = gst_ce_base_encoder_finalize;

  /* Instal class properties */
  g_object_class_install_property (gobject_class, PROP_SIZE_OUTPUT_BUF,
      g_param_spec_int ("outputBufferSize",
          "Size of the output buffer",
          "Size of the output buffer", 0, G_MAXINT32, 0, G_PARAM_READWRITE));

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
  base_encoder->first_buffer = FALSE;
  base_encoder->outBufSize = 0;
  base_encoder->freeMutex = NULL;
  base_encoder->freeSlices = NULL;
  
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
    GST_WARNING_OBJECT (base_encoder,
        "Memory couldn't be allocated for input buffer");
  }

  return buffer;
}

/* Reconfig the output buffer size of the encoder */
void
gst_ce_base_encoder_shrink_output_buffer (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, gsize new_size)
{
}
