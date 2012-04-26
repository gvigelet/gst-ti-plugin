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

#ifndef __GST_CE_H264_ENCODER_H__
#define __GST_CE_H264_ENCODER_H__

#include <gst/gst.h>
#include <gstcevidenc1.h>
#include <config.h>

G_BEGIN_DECLS
#define GST_TYPE_CE_H264_ENCODER \
  (gst_ce_h264_encoder_get_type())
#define GST_CE_H264_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CE_H264_ENCODER,GstCEH264Encoder))
#define GST_CE_H264_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CE_H264_ENCODER,GstCEH264EncoderClass))
#define GST_IS_CE_H264_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CE_H264_ENCODER))
#define GST_IS_CE_H264_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CE_H264_ENCODER))
#define CE_H264_ENCODER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_CE_H264_ENCODER, GstCEH264EncoderClass))
typedef struct _GstCEH264Encoder GstCEH264Encoder;
typedef struct _GstCEH264EncoderClass GstCEH264EncoderClass;

/**
 * This class implements the video encoder for h264
 * @extends _GstCEVIDENC1
 */
struct _GstCEH264Encoder
{
  GstCEVIDENC1 parent;

  gboolean generate_aud;
  gboolean generate_bytestream;
};

struct _GstCEH264EncoderClass
{
  GstCEVIDENC1Class parent_class;
  gboolean (*h264_encoder_fixate_src_caps) (GstCEBaseVideoEncoder * base_video_encoder,
    GstCaps * filter);
};

/* Macros that allow access to the methods of the class */

/*-------------------*/
/* Public methods ***/
/*-------------------*/

#define gst_ce_h264_encoder_fixate_src_caps(obj, filter) \
  CE_H264_ENCODER_GET_CLASS(GST_CE_H264_ENCODER(obj))->h264_encoder_fixate_src_caps(obj, filter)

/* Auxiliar functions for the class
 * Work similar to public methods  */

GstBuffer *gst_ce_h264_encoder_generate_codec_data (GstCEBaseEncoder * base_encoder,
    GstBuffer * input_buffer, GstBuffer * output_buffer);

GType gst_ce_h264_encoder_get_type (void);


G_END_DECLS
#endif /* __GST_CEH264_H__ */
