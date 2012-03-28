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

/* Free the codec instance in case of no null */
gboolean
gst_ce_base_encoder_finalize_codec(GstCEBaseVideoEncoder * video_encoder){
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER(video_encoder);  

  if (base_encoder->codec_handle != NULL) {
    if (!gst_ce_base_encoder_delete(base_encoder))
      return FALSE;
  }

  return TRUE;
}

/* Check for a previews instance of the codec, init the params and create the codec instance */
gboolean
gst_ce_base_encoder_init_codec(GstCEBaseVideoEncoder * video_encoder){
  
  /* Finalize any previous configuration  */
  if (!gst_ce_base_encoder_finalize_codec(video_encoder))
    return FALSE;
  
  /* Set the value of the params */
  if (!gst_ce_base_encoder_initialize_params(video_encoder))
    return FALSE;

  /* Give a chance to downstream caps to modify the params or dynamic
   * params before we use them
   */
  //if (! CE_BASE_VIDEO_ENCODER_GET_CLASS(video_encoder)->(
    //  video_encoder))
    //return FALSE;

  /* Create the codec instance */
  if (!gst_ce_base_encoder_create(video_encoder))
    return FALSE;

  
  /* TODO: create buffers, and initialize dynamic params*/

  return TRUE;
}

static gboolean
gst_ce_base_video_encoder_default_sink_set_caps (
  GstCEBaseVideoEncoder * video_encoder, GstCaps * caps)
{
  
  GST_DEBUG_OBJECT(video_encoder,"Entry default_sink_set_caps base video encoder");
  gboolean ret;
  GstVideoInfo info;
  
  /* Fixate and set caps */
  gst_ce_base_video_encoder_src_fixate_caps(video_encoder);
   
  /* get info from caps */
  if (!gst_video_info_from_caps (&info, caps))
    goto refuse_caps;

  video_encoder->video_info = info;

  /* We are ready to init the codec */
  ret = gst_ce_base_encoder_init_codec(video_encoder);
  
  GST_DEBUG_OBJECT(video_encoder,"Leave default_sink_set_caps base video encoder");
  return ret;
  
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
    GST_CE_BASE_VIDEO_ENCODER(gst_pad_get_parent(pad));
  GstCEBaseEncoder *base_encoder = GST_CE_BASE_ENCODER(video_encoder);  
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
gst_ce_base_video_encoder_default_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GST_DEBUG_OBJECT(parent,"Entry default_sink_event base video encoder");
  GstCEBaseVideoEncoder *video_encoder = GST_CE_BASE_VIDEO_ENCODER (parent);
  gboolean res = FALSE;
  
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      
      gst_event_parse_caps (event, &caps);
      
      /* Set src caps */
      res = gst_ce_base_video_encoder_sink_set_caps (video_encoder,
        caps);
      gst_event_unref (event);
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }
 
  GST_DEBUG_OBJECT(video_encoder,"Leave default_sink_event base video encoder");
  return res;
}

/* Default implementation for sink_query */
static gboolean
gst_ce_base_video_encoder_default_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{	
  GstCEBaseVideoEncoder *video_encoder = GST_CE_BASE_VIDEO_ENCODER (parent);
  gboolean res = FALSE;
  
  GST_DEBUG_OBJECT(video_encoder,"Entry default_sink_query base video encoder");
  
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
 
  GST_DEBUG_OBJECT(video_encoder,"Leave default_sink_query base video encoder");
  return res;
}

/* Default implementation for _chain */
static GstFlowReturn
gst_ce_base_video_encoder_default_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  
  GstCEBaseVideoEncoder *video_encoder = 
    GST_CE_BASE_VIDEO_ENCODER (parent);
  GST_DEBUG_OBJECT(video_encoder,"Enter default_chain base video encoder");
  
  int ret;
  
  GstMapInfo info_buffer;
  GstBuffer *buffer_in;
  GstBuffer *buffer_out;
  GstBuffer *buffer_push_out;
  
  /* Obtain and resize the input and output buffer */
  buffer_in = (GstBuffer *)GST_CE_BASE_ENCODER(video_encoder)->submitted_input_buffers;
  buffer_out = (GstBuffer *)GST_CE_BASE_ENCODER(video_encoder)->submitted_output_buffers;
  gst_buffer_resize(buffer_in, 0, gst_buffer_get_size(buffer));
  gst_buffer_resize(buffer_out, 0, gst_buffer_get_size(buffer));
  
  /* Access the data of the entry buffer and past it to a contiguos memory buffer */
  if(!gst_buffer_map (buffer, &info_buffer, GST_MAP_WRITE)) {
    GST_DEBUG_OBJECT(video_encoder,"Can't access data from buffer");
  }
  gsize bytes_copied = gst_buffer_fill (buffer_in, 0, info_buffer.data, gst_buffer_get_size(buffer));
  if(bytes_copied == 0) {
    GST_DEBUG_OBJECT(video_encoder,"Can't obtain data from buffer to input buffer");
  }
  
  g_print("Size before: %d\n", gst_buffer_get_size(buffer_in));
  gst_ce_base_encoder_encode (GST_CE_BASE_ENCODER(video_encoder), buffer_in, buffer_out);

  /* Copy the meta data from the buffer with raw data */
  gst_buffer_copy_into (buffer_out, buffer, GST_BUFFER_COPY_META, 0, -1);
  
  /* Prepare out buffer for push it */
  VIDENC1_OutArgs outArgs = *((VIDENC1_OutArgs *)GST_CE_BASE_ENCODER(video_encoder)->submitted_output_arguments);
  buffer_push_out = gst_buffer_new_and_alloc(outArgs.encodedBuf.bufSize);
  if(buffer_push_out == NULL) {
    GST_DEBUG_OBJECT(video_encoder,"Memory couldn't be allocated for push output buffer");
  }
  gst_buffer_fill (buffer_push_out, 0, outArgs.encodedBuf.buf, outArgs.encodedBuf.bufSize);
  gst_buffer_copy_into (buffer_push_out, buffer, GST_BUFFER_COPY_META, 0, -1);
  
  g_print("Size after: %d\n", gst_buffer_get_size(buffer_push_out));
  /*push the buffer and check for any error*/
  ret = gst_pad_push (GST_CE_BASE_ENCODER(video_encoder)->src_pad, 
    buffer_push_out);
  
  /* Free the push output buffer */
  
  if(GST_FLOW_OK != ret) {
    GST_ERROR_OBJECT (video_encoder, "Push buffer return with error: %d", ret);
  } 
  
  GST_DEBUG_OBJECT(video_encoder,"LEAVE");
  return ret;
}

static void
gst_ce_base_video_encoder_class_init (GstCEBaseVideoEncoderClass * video_encoder_class)
{
  GObjectClass *gobject_class = (GObjectClass *) video_encoder_class;
  
  /* Init debug instance */
  GST_DEBUG_CATEGORY_INIT (gst_ce_base_video_encoder_debug,
      "cebasevideoencoder", 0, "CodecEngine base video encoder Class");
  
  /* Override inheritance methods */
  gobject_class->set_property = gst_ce_base_video_encoder_set_property;
  gobject_class->get_property = gst_ce_base_video_encoder_get_property;
  video_encoder_class->chain = 
    GST_DEBUG_FUNCPTR(gst_ce_base_video_encoder_default_chain);
  video_encoder_class->sink_event = 
    GST_DEBUG_FUNCPTR(gst_ce_base_video_encoder_default_sink_event);
  video_encoder_class->sink_query = 
    GST_DEBUG_FUNCPTR(gst_ce_base_video_encoder_default_sink_query);
  video_encoder_class->sink_get_caps = 
    GST_DEBUG_FUNCPTR(gst_ce_base_video_encoder_default_sink_get_caps);
  video_encoder_class->sink_set_caps = 
    GST_DEBUG_FUNCPTR(gst_ce_base_video_encoder_default_sink_set_caps);
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


