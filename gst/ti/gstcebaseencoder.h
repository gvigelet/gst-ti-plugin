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

#ifndef _GST_CE_BASE_ENCODER_H_
#define _GST_CE_BASE_ENCODER_H_

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_CE_BASE_ENCODER \
  (gst_ce_base_encoder_get_type())
#define GST_CE_BASE_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CE_BASE_ENCODER,GstCEBaseEncoder))
#define GST_CE_BASE_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CE_BASE_ENCODER,GstCEBaseEncoderClass))
#define GST_IS_CE_BASE_ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CE_BASE_ENCODER))
#define GST_IS_CE_BASE_ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CE_BASE_ENCODER))
#define CE_BASE_ENCODER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_CE_BASE_ENCODER, GstCEBaseEncoderClass  ))
typedef struct _GstCEBaseEncoder GstCEBaseEncoder;
typedef struct _GstCEBaseEncoderClass GstCEBaseEncoderClass;
typedef struct _processAsyncArguments processAsyncArguments;

/**
 * This is the base class for the CodecEngine based encoders
 * @extends GstElement
 */
struct _GstCEBaseEncoder
{
  GstElement element;

  /** 
   * @brief sink pad of the encoder
   * @details This object is instatiated by a any sub-class (usually the 
   *  one that is responsible for the actual caps of the encoder)
   * @protected
   */
  GstPad *sink_pad;
  
  /** 
   * @brief src pad of the encoder
   * @details This object is instatiated by a any sub-class (usually the 
   *  one that is responsible for the actual caps of the encoder)
   * @protected
   */
  GstPad *src_pad;

  /* Members for encode process */
  char *codec_name;

  /* Handler for the encoder instance */
  gpointer engine_handle;

  /* Handler for the encoder instance */
  gpointer codec_handle;

  /* Static params for indicate the behavior of the encoder */
  gpointer codec_params;

  /* Static params for indicate the behavior of the encoder in run time */
  gpointer codec_dynamic_params;

  /* Pointers to hold data submitted into CodecEngine */
  /* Input buffer for the encoder instance */
  gpointer submitted_input_buffers;
  
  /* Output buffer of the encoder instance */
  gpointer submitted_output_buffers;
   
  /* Input arguments for the encoder instance */
  gpointer submitted_input_arguments;
  
  /* Output arguments of the encoder instance */
  gpointer submitted_output_arguments;
  
  /* Thread for async encode process */
  pthread_t *process_async_thread;
  
  /* Arguments for the async encode process */
  processAsyncArguments *process_async_arguments;
  
  /* Codec data for the encode */
  gpointer codec_data;

};

struct _GstCEBaseEncoderClass
{
  GstElementClass parent_class;

  gboolean (*base_encoder_initialize_params) (GstCEBaseEncoder * base_encoder);
  gboolean (*base_encoder_control) (GstCEBaseEncoder * base_encoder, gint cmd_id);
  gboolean (*base_encoder_delete) (GstCEBaseEncoder * base_encoder);
  gboolean (*base_encoder_create) (GstCEBaseEncoder * base_encoder);
  gboolean (*base_encoder_process_sync) (GstCEBaseEncoder * base_encoder,
    gpointer input_buffer, gpointer output_buffer);
  gboolean (*base_encoder_process_async) (processAsyncArguments * arguments);
  gpointer (*base_encoder_generate_codec_data) (gpointer push_out_buffer);
  gpointer (*base_encoder_encode) (GstCEBaseEncoder * base_encoder, gpointer raw_data_buffer);
  gboolean (*base_encoder_init_codec) (GstCEBaseEncoder * base_encoder);
  gboolean (*base_encoder_finalize_codec) (GstCEBaseEncoder * base_encoder);
};


struct _processAsyncArguments
{
  GstCEBaseEncoder *base_encoder;
  gpointer input_buffer;
  gpointer output_buffer;
};

GType gst_ce_base_encoder_get_type (void);

/* Macros that allow access to the methods of the class */

/*------------------*/
/* Public functions */
/*------------------*/

#define gst_ce_base_encoder_finalize_codec(obj) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_finalize_codec(obj)

#define gst_ce_base_encoder_init_codec(obj) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_init_codec(obj)

#define gst_ce_base_encoder_process_sync(obj, input_buf, output_buf) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_process_sync(obj, input_buf, output_buf)


/**
 * @memberof _GstCEBaseEncoder
 * @brief Allocates output buffers for share efficiently with co-processors
 * @details This function allocates GstBuffers that are contiguous on memory
 *  (have #_GstCMEMMeta). This buffers can be shrinked efficiently to re-use the
 *  limited-available contiguous memory.
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @param size the size of the buffer
 */
#define gst_ce_base_encoder_generate_codec_data(obj, push_out_buf) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_generate_codec_data(push_out_buf)

/**
 * @memberof _GstCEBaseEncoder
 * @brief Allocates output buffers for share efficiently with co-processors
 * @details This function allocates GstBuffers that are contiguous on memory
 *  (have #_GstCMEMMeta). This buffers can be shrinked efficiently to re-use the
 *  limited-available contiguous memory.
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @param size the size of the buffer
 */
#define gst_ce_base_encoder_encode(obj, raw_data_buf) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_encode(obj, raw_data_buf)


/*--------------------*/
/* Abstract functions */
/*--------------------*/

/** 
 * @memberof _GstCEBaseEncoder
 * @fn void gst_ce_base_encoder_generate_codec_data(_GstCEBaseEncoder *base_encoder, GstBuffer *push_out_buf)
 * @brief Abstract function that implements the generation of the codec data for the encoder.
 * @details This function is implemented by a sub-class that known the specific form of generate
 * codec data according to the encoder algorithm.
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @param push_out_buf a pointer to the buffer for being push to the next element
 * @protected
 */
#define gst_ce_base_encoder_generate_codec_data(obj, push_out_buf) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_generate_codec_data(push_out_buf)


/** 
 * @memberof _GstCEBaseEncoder
 * @fn void gst_ce_base_encoder_initialize_params(_GstCEBaseEncoder *base_encoder)
 * @brief Abstract function that implements the initialization of static
 *  and dynamic parameters for the codec.
 * @details This function is implemented by a sub-class that handles the right CodecEngine API (live VIDENC1, or IMGENC)
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @protected
 */
#define gst_ce_base_encoder_initialize_params(obj) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_initialize_params(GST_CE_BASE_ENCODER(obj))

/** 
 * @memberof _GstCEBaseEncoder
 * @fn gint32 gst_ce_base_encoder_control(_GstCEBaseEncoder *base_encoder)
 * @brief Abstract function that implements controlling behavior of the codec.
 * @details This function is implemented by a sub-class that handles the right CodecEngine API (live VIDENC1, or IMGENC)
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @protected
 */
#define gst_ce_base_encoder_control(obj, cmd) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_control(GST_CE_BASE_ENCODER(obj), cmd)

/** 
 * @memberof _GstCEBaseEncoder
 * @fn void gst_ce_base_encoder_delete(_GstCEBaseEncoder *base_encoder)
 * @brief Abstract function that implements deleting the instance of the codec.
 * @details This function is implemented by a sub-class that handles the right CodecEngine API (live VIDENC1, or IMGENC)
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @protected
 */
#define gst_ce_base_encoder_delete(obj) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_delete(GST_CE_BASE_ENCODER(obj))

/** 
 * @memberof _GstCEBaseEncoder
 * @fn gpointer gst_ce_base_encoder_create(_GstCEBaseEncoder *base_encoder)
 * @brief Abstract function that implements creating an instance of the codec
 * @details This function is implemented by a sub-class that handles the right CodecEngine API (live VIDENC1, or IMGENC)
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @protected
 */
#define gst_ce_base_encoder_create(obj) \
  CE_BASE_ENCODER_GET_CLASS(obj)->base_encoder_create(GST_CE_BASE_ENCODER(obj))



/* Auxiliar functions for the class
 * Work similar to public methods  */

/**
 * @memberof _GstCEBaseEncoder
 * @brief Allocates output buffers for share efficiently with co-processors
 * @details This function allocates GstBuffers that are contiguous on memory
 *  (have #_GstCMEMMeta). This buffers can be shrinked efficiently to re-use the
 *  limited-available contiguous memory.
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @param size the size of the buffer
 */
GstBuffer *gst_ce_base_encoder_get_output_buffer (GstCEBaseEncoder *
    base_encoder, gsize size);

/**
 * @memberof _GstCEBaseEncoder
 * @brief Shrink output buffers for re-use memory
 * @details This function shrinked efficiently (using CMEN) to re-use the
 *  limited-available contiguous memory.
 * @param base_encoder a pointer to a _GstCEBaseEncoder object
 * @param buffer buffer to be re-size
 * @param new_size new size for the buffer
 */
void gst_ce_base_encoder_shrink_output_buffer (GstCEBaseEncoder * base_encoder,
    GstBuffer * buffer, gsize new_size);


G_END_DECLS
#endif
