/* GStreamer
 * Copyright (C) 2012 RidgeRun
 *
 * Author: Diego Dompe <ddompe@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GST_CMEM_META_H_
#define _GST_CMEM_META_H_

#include <gst/gst.h>

#include <xdc/std.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <gstcebaseencoder.h>


G_BEGIN_DECLS

#define GST_CMEM_META "GstCMEMMeta"
/** 
 * Contiguous memory (CMEM) metadata
 *
 * This meta data is used to abstract hardware accelerated buffers (in the
 * sense that they can be shared with hardware subsystems that does not 
 * support MMU).
 */
struct _GstCMEMMeta
{
  GstMeta meta;

  /** physical_address of the buffer */
  gpointer physical_address;
  
  /* Object that management buffer */
  GstCEBaseEncoder *base_encoder;
};

typedef struct _GstCMEMMeta GstCMEMMeta;

const GstMetaInfo *gst_cmem_meta_register (void);
#define GST_CMEM_META_REGISTER (gst_cmem_meta_register())

#define gst_buffer_get_cmem_meta(b) \
  ((GstCMEMMeta*)gst_buffer_get_meta((b),GST_CMEM_META_INFO))
#define gst_buffer_add_cmem_meta(b) \
  ((GstCMEMMeta*)gst_buffer_add_meta((b),GST_CMEM_META_INFO,NULL))

/** Obtains the physical address from a buffer that contains #_GstCMEMMeta
 * @param buffer a GstBuffer that has GstCMEMMeta
 * @returns a pointer to the physical address of the buffer, or NULL if the
 *  buffer does not contains physical contiguous memory
 * @related _GstCMEMMeta
 */
gpointer gst_buffer_get_cmem_physical_address (GstBuffer * buffer);

/** Set the physical address of a buffer, appending a #_GstCMEMMeta metadata 
 * if it does not have such metadata already.
 * @param buffer a GstBuffer
 * @param paddr a physical address of the buffer (should be contiguous on memory)
 * @related _GstCMEMMeta
 */
void gst_buffer_set_cmem_physical_address (GstBuffer * buffer, gpointer paddr);


const GstMetaInfo* gst_cmem_meta_get_info();


G_END_DECLS
#endif
