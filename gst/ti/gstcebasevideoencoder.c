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
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/video1/videnc1.h>
#include <gstcmemallocator.h>
#include <gstcmemmeta.h>
#include <string.h>

#define GST_CAT_DEFAULT gst_ce_base_video_encoder_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_0,
};


static void
gst_ce_base_video_encoder_base_init (GstCEBaseVideoEncoderClass * klass)
{
  /* Initialize dynamic data */
}

static void
gst_ce_base_video_encoder_base_finalize (GstCEBaseVideoEncoderClass * klass)
{
}

static void
gst_ce_base_video_encoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      break;
  }

}

static void
gst_ce_base_video_encoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      break;
  }
}

/* Default implementation of the method  video_encoder_sink_set_caps */
static gboolean
gst_ce_base_video_encoder_default_sink_set_caps (GstCEBaseVideoEncoder *
    video_encoder, GstCaps * caps)
{

  GST_DEBUG_OBJECT (video_encoder,
      "Entry default_sink_set_caps base video encoder");
  gboolean ret;
  GstVideoInfo info;

  /* get info from caps */
  if (!gst_video_info_from_caps (&info, caps))
    goto refuse_caps;

  video_encoder->video_info = info;

  /* Free any previus definition of base atributes */
  gst_ce_base_encoder_finalize_attributes (GST_CE_BASE_ENCODER (video_encoder));

  /* We are ready to init the codec */
  if (!gst_ce_base_encoder_init_codec (video_encoder))
    goto refuse_caps;


  /* save the suggest caps for then update the caps */
  GST_CE_BASE_ENCODER (video_encoder)->suggest_caps = caps;

  GST_DEBUG_OBJECT (video_encoder,
      "Leave default_sink_set_caps base video encoder");

  return TRUE;

  /* ERRORS */
refuse_caps:
  {
    GST_ERROR_OBJECT (video_encoder, "refused caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
}


/* Default implementation for sink_get_caps */
static GstCaps *
gst_ce_base_video_encoder_default_sink_get_caps (GstPad * pad, GstCaps * filter)
{
  GstCEBaseVideoEncoder *video_encoder =
      GST_CE_BASE_VIDEO_ENCODER (gst_pad_get_parent (pad));
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER (video_encoder);
  GstCaps *caps, *othercaps;
  const GstCaps *templ;

  gint i, j;
  GstStructure *structure = NULL;

  /* If we already have caps return them */
  if ((caps = gst_pad_get_current_caps (pad)) != NULL) {
    goto done;
  }

  /* we want to proxy properties like width, height and framerate from the 
     other end of the element */
  othercaps = gst_pad_peer_query_caps (base_encoder->src_pad, filter);
  if (othercaps == NULL ||
      gst_caps_is_empty (othercaps) || gst_caps_is_any (othercaps)) {
    caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));
    goto done;
  }

  caps = gst_caps_new_empty ();
  templ = gst_pad_get_pad_template_caps (pad);

  /* Set caps with peer caps values */
  for (i = 0; i < gst_caps_get_size (templ); i++) {
    /* pick fields from peer caps */
    for (j = 0; j < gst_caps_get_size (othercaps); j++) {
      GstStructure *s = gst_caps_get_structure (othercaps, j);
      const GValue *val;

      structure = gst_structure_copy (gst_caps_get_structure (templ, i));
      if ((val = gst_structure_get_value (s, "width")))
        gst_structure_set_value (structure, "width", val);
      if ((val = gst_structure_get_value (s, "height")))
        gst_structure_set_value (structure, "height", val);
      if ((val = gst_structure_get_value (s, "framerate")))
        gst_structure_set_value (structure, "framerate", val);

      gst_caps_merge_structure (caps, structure);
    }
  }

done:
  gst_caps_replace (&othercaps, NULL);
  gst_object_unref (video_encoder);

  return caps;
}

/* Default implementation for sink_event */
static gboolean
gst_ce_base_video_encoder_default_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GST_DEBUG_OBJECT (parent, "Entry default_sink_event base video encoder");
  GstCEBaseVideoEncoder *video_encoder = GST_CE_BASE_VIDEO_ENCODER (parent);
  gboolean res = FALSE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);

      /* Set src caps */
      res = gst_ce_base_video_encoder_sink_set_caps (video_encoder, caps);
      gst_event_unref (event);


      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  GST_DEBUG_OBJECT (video_encoder,
      "Leave default_sink_event base video encoder");
  return res;
}

/* Default implementation for sink_query */
static gboolean
gst_ce_base_video_encoder_default_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstCEBaseVideoEncoder *video_encoder = GST_CE_BASE_VIDEO_ENCODER (parent);
  gboolean res = FALSE;

  GST_DEBUG_OBJECT (video_encoder,
      "Entry default_sink_query base video encoder");

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);

      /* Get the caps using the filter */
      caps = gst_ce_base_video_encoder_sink_get_caps (video_encoder,
          pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;

      break;
    }
    default:
      /* Sent the query to all pads internally linked to "pad". */
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  GST_DEBUG_OBJECT (video_encoder,
      "Leave default_sink_query base video encoder");
  return res;
}

/* Function for check if the Buffer is CMEM */
gboolean
gst_ce_base_video_is_cmem_buffer (GstCEBaseVideoEncoder * video_encoder,
    GstBuffer * buffer)
{

  gboolean ret = FALSE;
  gpointer state = NULL;

  /* The first GstMeta correspond with the CMEM meta */
  GstMeta *buffer_meta_data = gst_buffer_iterate_meta (buffer, &state);
  if (buffer_meta_data != NULL) {
    GstMetaInfo *buffer_meta_info = buffer_meta_data->info;
    gchar *buffer_meta_type_name = g_type_name (buffer_meta_info->type);

    /* Compare type name  */
    if (strcmp (GST_CMEM_META, buffer_meta_type_name) == 0) {
      GST_CE_BASE_ENCODER (video_encoder)->submitted_input_buffers = buffer;
      ret = TRUE;
    } else {
      GST_WARNING_OBJECT (video_encoder, "Buffer isn't CMEM");
    }
  } else {
    GST_WARNING_OBJECT (video_encoder, "Can't obtain buffer meta info");
  }


  return ret;
}


/* Default implementation for _chain */
static GstFlowReturn
gst_ce_base_video_encoder_default_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{

  GstCEBaseVideoEncoder *video_encoder = GST_CE_BASE_VIDEO_ENCODER (parent);
  GST_DEBUG_OBJECT (video_encoder, "Enter default_chain base video encoder");

  int ret;
  GstClockTime start, end;

  GstMapInfo info_buffer;
  GstMapInfo info_input_buffer;
  GstBuffer *buffer_in;
  GstBuffer *buffer_out;
  GstBuffer *buffer_push_out;
  gboolean input_buffer_copied = FALSE;
  GstAllocator *buffer_allocator = NULL;

  start = gst_util_get_timestamp ();

  /* Check if the buffer is CMEM */
  input_buffer_copied =
      gst_ce_base_video_is_cmem_buffer (video_encoder, buffer);


  /* Check for attributes that help in push_out_buffer managent */
  /*if(GST_CE_BASE_ENCODER (video_encoder)->freeSlices == NULL) {
     GST_CE_BASE_ENCODER (video_encoder)->minSizeSlicePushOutBuf = gst_buffer_get_size (buffer);
     slice = g_malloc0(sizeof(struct cmemSlice));
     slice->start = 0;
     slice->end = GST_CE_BASE_ENCODER (video_encoder)->minSizeSlicePushOutBuf * 3;  3:default size for the out_pust_buffer */
  /*slice->size = GST_CE_BASE_ENCODER (video_encoder)->minSizeSlicePushOutBuf * 3;
     GST_CE_BASE_ENCODER (video_encoder)->freeMutex = g_mutex_new();
     GST_CE_BASE_ENCODER (video_encoder)->freeSlices = g_list_append(
     GST_CE_BASE_ENCODER (video_encoder)->freeSlices, slice);

     }  */

  /* Check for the input_buffer */
  if (GST_CE_BASE_ENCODER (video_encoder)->submitted_input_buffers == NULL) {

    GST_CE_BASE_ENCODER (video_encoder)->submitted_input_buffers =
        gst_ce_base_encoder_get_cmem_buffer (GST_CE_BASE_ENCODER
        (video_encoder), gst_buffer_get_size (buffer));
  }

  /* Check for the output_buffer */
  if (GST_CE_BASE_ENCODER (video_encoder)->submitted_output_buffers == NULL) {

    GST_CE_BASE_ENCODER (video_encoder)->submitted_output_buffers =
        gst_ce_base_encoder_get_cmem_buffer (GST_CE_BASE_ENCODER
        (video_encoder), gst_buffer_get_size (buffer));

  }

  /* Check for copied the entry buffer */
  if (input_buffer_copied == FALSE) {

    /* Access the data from the entry buffer */
    if (!gst_buffer_map (buffer, &info_buffer, GST_MAP_WRITE)) {
      GST_WARNING_OBJECT (video_encoder, "Can't access data from buffer");
    }

    gst_buffer_fill (GST_CE_BASE_ENCODER (video_encoder)->
        submitted_input_buffers, 0, info_buffer.data,
        gst_buffer_get_size (buffer));
  }

  /* Encode the actual buffer */
  buffer_push_out =
      gst_ce_base_encoder_encode (GST_CE_BASE_ENCODER (video_encoder));

  if (buffer_push_out == NULL) {
    /* Only obtain the next buffer */
    ret = GST_FLOW_OK;
  } else {

    /* Copy extra data from the original buffer to the push out buffer */
    gst_buffer_copy_into (buffer_push_out, buffer, GST_BUFFER_COPY_META, 0,
        gst_buffer_get_size (buffer));
    gst_buffer_copy_into (buffer_push_out, buffer, GST_BUFFER_COPY_TIMESTAMPS,
        0, gst_buffer_get_size (buffer));
    gst_buffer_copy_into (buffer_push_out, buffer, GST_BUFFER_COPY_FLAGS, 0,
        -1);

    /* push the buffer and check for any error */
    ret =
        gst_pad_push (GST_CE_BASE_ENCODER (video_encoder)->src_pad,
        buffer_push_out);

    if (GST_FLOW_OK != ret) {
      GST_ERROR_OBJECT (video_encoder, "Push buffer return with error: %d",
          ret);
    }
  }

  /* unref the original buffer */
  gst_buffer_unref (buffer);

  GST_DEBUG_OBJECT (video_encoder, "LEAVE");
  return ret;
}

static void
gst_ce_base_video_encoder_class_init (GstCEBaseVideoEncoderClass *
    video_encoder_class)
{
  GObjectClass *gobject_class = (GObjectClass *) video_encoder_class;

  /* Init debug instance */
  GST_DEBUG_CATEGORY_INIT (gst_ce_base_video_encoder_debug,
      "cebasevideoencoder", 0, "CodecEngine base video encoder Class");

  GST_DEBUG ("ENTER");

  /* Instance the class methods */
  video_encoder_class->video_encoder_chain =
      GST_DEBUG_FUNCPTR (gst_ce_base_video_encoder_default_chain);
  video_encoder_class->video_encoder_sink_event =
      GST_DEBUG_FUNCPTR (gst_ce_base_video_encoder_default_sink_event);
  video_encoder_class->video_encoder_sink_query =
      GST_DEBUG_FUNCPTR (gst_ce_base_video_encoder_default_sink_query);
  video_encoder_class->video_encoder_sink_get_caps =
      GST_DEBUG_FUNCPTR (gst_ce_base_video_encoder_default_sink_get_caps);
  video_encoder_class->video_encoder_sink_set_caps =
      GST_DEBUG_FUNCPTR (gst_ce_base_video_encoder_default_sink_set_caps);

  /* Override heredity methods */
  gobject_class->set_property = gst_ce_base_video_encoder_set_property;
  gobject_class->get_property = gst_ce_base_video_encoder_get_property;

  GST_DEBUG ("LEAVE");
}

static void
gst_ce_base_video_encoder_class_finalize (GstCEBaseVideoEncoderClass * klass,
    gpointer * class_data)
{
}

/* Obtain and register the type of the class */
GType
gst_ce_base_video_encoder_get_type (void)
{
  static GType object_type = 0;

  if (object_type == 0) {
    static const GTypeInfo object_info = {
      sizeof (GstCEBaseVideoEncoderClass),
      (GBaseInitFunc) gst_ce_base_video_encoder_base_init,
      (GBaseFinalizeFunc) gst_ce_base_video_encoder_base_finalize,
      (GClassInitFunc) gst_ce_base_video_encoder_class_init,
      NULL,
      NULL,
      sizeof (GstCEBaseVideoEncoder),
      0,
      NULL,
      NULL
    };

    object_type = g_type_register_static (GST_TYPE_CE_BASE_ENCODER,
        "GstCEBaseVideoEncoder", &object_info, (GTypeFlags) 0);
  }
  return object_type;
};
