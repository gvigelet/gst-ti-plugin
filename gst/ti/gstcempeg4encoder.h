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

#ifndef __GST_CE_MPEG4_ENCODER_H__
#define __GST_CE_MPEG4_ENCODER_H__

#include <gst/gst.h>
#include <gstcevidenc1.h>
#include <config.h>

G_BEGIN_DECLS
#define GST_TYPE_CE_MPEG4_ENCODER \
  (gst_ce_mpeg4_encoder_get_type())
#define GST_CE_MPEG4_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CE_MPEG4_ENCODER,GstCEMPEG4Encoder))
#define GST_CE_MPEG4_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CE_MPEG4_ENCODER,GstCEMPEG4EncoderClass))
#define GST_IS_CE_MPEG4_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CE_MPEG4_ENCODER))
#define GST_IS_CE_MPEG4_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CE_MPEG4_ENCODER))
#define CE_MPEG4_ENCODER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_CE_MPEG4_ENCODER, GstCEMPEG4EncoderClass))
typedef struct _GstCEMPEG4Encoder GstCEMPEG4Encoder;
typedef struct _GstCEMPEG4EncoderClass GstCEMPEG4EncoderClass;

/**
 * This class implements the video encoder for MPEG4
 * @extends _GstCEVIDENC1
 */
struct _GstCEMPEG4Encoder
{
  GstCEVIDENC1 parent;
};

struct _GstCEMPEG4EncoderClass
{
  GstCEVIDENC1Class parent_class;
  GstBuffer* (*mpeg4_encoder_post_process) (GstCEBaseEncoder * base_encoder, GstBuffer *buffer, 
    GList **actual_free_slice);
};


/* Macros that allow access to the methods of the class */

/*-------------------*/
/* Public methods ***/
/*-------------------*/

#define gst_ce_mpeg4_encoder_post_process(obj, buf, actual_slice) \
  CE_MPEG4_ENCODER_GET_CLASS(GST_CE_MPEG4_ENCODER(obj))->mpeg4_encoder_post_process(obj, buf, actual_slice)


/* Auxiliar functions for the class
 * Work similar to public methods  */

GType gst_ce_mpeg4_encoder_get_type (void);

GstBuffer* gst_ce_mpeg4_encoder_generate_codec_data(GstBuffer *buffer);

GstCaps* gst_ce_mpeg4_encoder_fixate_src_caps(GstCEBaseVideoEncoder * base_video_encoder,
  GstCaps * filter);


G_END_DECLS
#endif /* __GST_CE_MPEG4_ENCODER_H__ */
