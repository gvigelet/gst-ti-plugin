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

#ifndef ___GST_CE_BASE_VIDEO_ENCODER_H__
#define ___GST_CE_BASE_VIDEO_ENCODER_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gstcebaseencoder.h>

G_BEGIN_DECLS
#define GST_TYPE_CE_BASE_VIDEO_ENCODER \
  (gst_ce_base_video_encoder_get_type())
#define GST_CE_BASE_VIDEO_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CE_BASE_VIDEO_ENCODER,GstCEBaseVideoEncoder))
#define GST_CE_BASE_VIDEO_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CE_BASE_VIDEO_ENCODER,GstCEBaseVideoEncoderClass))
#define GST_IS_CE_BASE_VIDEO_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CE_BASE_VIDEO_ENCODER))
#define GST_IS_CE_BASE_VIDEO_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CE_BASE_VIDEO_ENCODER))
#define CE_BASE_VIDEO_ENCODER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_CE_BASE_VIDEO_ENCODER, GstCEBaseVideoEncoderClass))
typedef struct _GstCEBaseVideoEncoder GstCEBaseVideoEncoder;
typedef struct _GstCEBaseVideoEncoderClass GstCEBaseVideoEncoderClass;

/**
 * This is the base class for the CodecEngine based video encoders
 * @extends _GstCEBaseEncoder
 */
struct _GstCEBaseVideoEncoder
{
  GstCEBaseEncoder base_encoder;
  GstVideoInfo video_info;
};

struct _GstCEBaseVideoEncoderClass
{
  GstCEBaseEncoderClass parent_class;

    GstFlowReturn (*chain) (GstPad * pad, GstObject * parent,
      GstBuffer * buffer);
    gboolean (*sink_event) (GstPad * pad, GstObject * parent, GstEvent * query);
    gboolean (*sink_query) (GstPad * pad, GstObject * parent, GstQuery * query);
    gboolean (*sink_set_caps) (GstCEBaseVideoEncoder * video_encoder,
      GstCaps * caps);
  GstCaps *(*sink_get_caps) (GstPad * pad, GstCaps * filter);
    gboolean (*fixate_src_caps) (GstCEBaseVideoEncoder * video_encoder,
      GstCaps * filter);
};

GType gst_ce_base_video_encoder_get_type (void);

/*---------------------*/
/* Protected Functions */
/*---------------------*/
gboolean gst_ce_base_encoder_init_codec (GstCEBaseVideoEncoder * video_encoder);

#define gst_ce_base_video_encoder_chain(obj,pad, parent, buffer) \
  CE_BASE_VIDEO_ENCODER_GET_CLASS(obj)->chain(pad, parent, query)

#define gst_ce_base_video_encoder_sink_event(obj, pad, parent, event) \
  CE_BASE_VIDEO_ENCODER_GET_CLASS(obj)->sink_event(pad, parent, event)

#define gst_ce_base_video_encoder_sink_query(obj, pad, parent, query) \
  CE_BASE_VIDEO_ENCODER_GET_CLASS(obj)->sink_query(pad, parent, query)

#define gst_ce_base_video_encoder_sink_set_caps(obj, caps) \
  CE_BASE_VIDEO_ENCODER_GET_CLASS(obj)->sink_set_caps(obj, caps)

#define gst_ce_base_video_encoder_sink_get_caps(obj, pad, filter) \
  CE_BASE_VIDEO_ENCODER_GET_CLASS(obj)->sink_get_caps(pad, filter)

#define gst_ce_base_video_encoder_src_fixate_caps(obj, filter) \
  CE_BASE_VIDEO_ENCODER_GET_CLASS(obj)->fixate_src_caps(obj, filter)

/* Abstract Functions */

G_END_DECLS
#endif /* ___GST_BASE_CE_VIDEO_ENCODER_H__ */
