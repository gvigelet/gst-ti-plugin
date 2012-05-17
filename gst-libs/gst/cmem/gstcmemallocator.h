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

#ifndef _GST_CMEM_ALLOCATOR_H_
#define _GST_CMEM_ALLOCATOR_H_

#include <gst/gst.h>

#include <xdc/std.h>
#include <ti/sdo/ce/osal/Memory.h>

#define GST_ALLOCATOR_CMEM "ContiguosMemory"

void gst_cmem_allocator_initialize (void);

void 
gst_cmem_allocator_set_data(GstMemory * mem, guint8 *new_data);


#endif
