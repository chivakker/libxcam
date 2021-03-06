/*
 * fmt.h - deal with the supported formats
 *
 *  Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: John Ye <john.ye@intel.com>
 */

#ifndef __FMT_H__
#define __FMT_H__

#include <gst/video/video-format.h>
#include <xcam_defs.h>

XCAM_BEGIN_DECLARE

void caps_append(GstCaps *caps);

XCAM_END_DECLARE
#endif  // __FMT_H__
