/*
 * cl_memory.cpp - CL memory
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
 * Author: Wind Yuan <feng.yuan@intel.com>
 */

#include "cl_memory.h"
#include "drm_display.h"

namespace XCam {

CLImageDesc::CLImageDesc ()
    : format {CL_R, CL_UNORM_INT8}
, width (0)
, height (0)
, row_pitch (0)
, size (0)
{
}

CLMemory::CLMemory (SmartPtr<CLContext> &context)
    : _context (context)
    , _mem_id (NULL)
    , _mem_fd (-1)
{
    XCAM_ASSERT (context.ptr () && context->is_valid ());
}

CLMemory::~CLMemory ()
{
    release_fd ();

    if (_mem_id) {
        _context->destroy_mem (_mem_id);
    }
}

int32_t
CLMemory::export_fd ()
{
    if (_mem_fd <= 0)
        return _mem_fd;

    _mem_fd = _context->export_mem_fd (_mem_id);
    return _mem_fd;
}

void
CLMemory::release_fd ()
{
    if (_mem_fd <= 0)
        return;

    close (_mem_fd);
    _mem_fd = -1;
}

bool CLMemory::get_cl_mem_info (
    cl_image_info param_name, size_t param_size,
    void *param, size_t *param_size_ret)
{
    cl_mem mem_id = get_mem_id ();
    cl_int error_code = CL_SUCCESS;
    if (!mem_id)
        return false;

    error_code = clGetMemObjectInfo (mem_id, param_name, param_size, param, param_size_ret);
    XCAM_FAIL_RETURN(
        WARNING,
        error_code == CL_SUCCESS,
        false,
        "clGetMemObjectInfo failed on param:%d, errno:%d", param_name, error_code);
    return true;
}

CLImage::CLImage (SmartPtr<CLContext> &context)
    : CLMemory (context)
{
}

uint32_t
CLImage::get_pixel_bytes () const
{
    return calculate_pixel_bytes(_image_desc.format);
}

bool
CLImage::get_cl_image_info (cl_image_info param_name, size_t param_size, void *param, size_t *param_size_ret)
{
    cl_mem mem_id = get_mem_id ();
    cl_int error_code = CL_SUCCESS;
    if (!mem_id)
        return false;

    error_code = clGetImageInfo (mem_id, param_name, param_size, param, param_size_ret);
    XCAM_FAIL_RETURN(
        WARNING,
        error_code == CL_SUCCESS,
        false,
        "clGetImageInfo failed on param:%d, errno:%d", param_name, error_code);
    return true;
}

uint32_t
CLImage::calculate_pixel_bytes (const cl_image_format &fmt)
{
    uint32_t a = 0, b = 0;
    switch (fmt.image_channel_order) {
    case CL_R:
    case CL_A:
    case CL_Rx:
        a = 1;
        break;
    case CL_RG:
    case CL_RA:
    case CL_RGx:
        a = 2;
        break;
    case CL_RGB:
    case CL_RGBx:
        a = 3;
        break;
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
        a = 4;
        break;
    default:
        XCAM_LOG_DEBUG ("calculate_pixel_bytes with wrong channel_order:0x%04x", fmt.image_channel_order);
        return 0;
    }

    switch (fmt.image_channel_data_type) {
    case CL_UNORM_INT8:
    case CL_SNORM_INT8:
    case CL_SIGNED_INT8:
    case CL_UNSIGNED_INT8:
        b = 1;
        break;
    case CL_SNORM_INT16:
    case CL_UNORM_INT16:
    case CL_SIGNED_INT16:
    case CL_UNSIGNED_INT16:
    case CL_HALF_FLOAT:
        b = 2;
        break;
    case CL_UNORM_INT24:
        b = 3;
        break;
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT32:
    case CL_FLOAT:
        b = 4;
        break;
    default:
        XCAM_LOG_DEBUG ("calculate_pixel_bytes with wrong channel_data_type:0x%04x", fmt.image_channel_data_type);
        return 0;
    }

    return a * b;
}

bool
CLImage::video_info_2_cl_image_desc (
    const VideoBufferInfo & video_info,
    CLImageDesc &image_desc)
{
    image_desc.width = video_info.width;
    image_desc.height = video_info.height;
    image_desc.row_pitch = video_info.strides[0];
    XCAM_ASSERT (image_desc.row_pitch >= image_desc.width);

    switch (video_info.format) {
    case XCAM_PIX_FMT_RGB48:
        //cl_image_info.fmt.image_channel_order = CL_RGB;
        //cl_image_info.fmt.image_channel_data_type = CL_UNORM_INT16;
        XCAM_LOG_WARNING (
            "video_info to cl_image_info doesn't support XCAM_PIX_FMT_RGB48, maybe try XCAM_PIX_FMT_RGBA64 instread\n"
            " **** XCAM_PIX_FMT_RGB48 need check with cl implementation ****");
        return false;
        break;

    case XCAM_PIX_FMT_RGBA64:
        image_desc.format.image_channel_order = CL_RGBA;
        image_desc.format.image_channel_data_type = CL_UNORM_INT16;
        break;

    case V4L2_PIX_FMT_RGB24:
        image_desc.format.image_channel_order = CL_RGB;
        image_desc.format.image_channel_data_type = CL_UNORM_INT8;
        break;

    case V4L2_PIX_FMT_RGB565:
        image_desc.format.image_channel_order = CL_RGB;
        image_desc.format.image_channel_data_type = CL_UNORM_SHORT_565;
        break;
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_BGR32:
        image_desc.format.image_channel_order = CL_BGRA;
        image_desc.format.image_channel_data_type = CL_UNORM_INT8;
        break;
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_XRGB32:
        image_desc.format.image_channel_order = CL_ARGB;
        image_desc.format.image_channel_data_type = CL_UNORM_INT8;
        break;

    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SBGGR16:
    case XCAM_PIX_FMT_SGRBG16:
        image_desc.format.image_channel_order = CL_R;
        image_desc.format.image_channel_data_type = CL_UNORM_INT16;
        break;

    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
        image_desc.format.image_channel_order = CL_R;
        image_desc.format.image_channel_data_type = CL_UNORM_INT8;
        break;

    case V4L2_PIX_FMT_NV12:
        image_desc.format.image_channel_order = CL_R;
        image_desc.format.image_channel_data_type = CL_UNORM_INT8;
        image_desc.height *= 2;
        break;

    case V4L2_PIX_FMT_YUYV:
        image_desc.format.image_channel_order = CL_RGBA;
        image_desc.format.image_channel_data_type = CL_UNORM_INT8;
        image_desc.width /= 2;
        break;

    case XCAM_PIX_FMT_LAB:
        image_desc.format.image_channel_order = CL_R;
        image_desc.format.image_channel_data_type = CL_FLOAT;
        break;

    default:
        XCAM_LOG_WARNING (
            "video_info to cl_image_info doesn't support format:%s",
            xcam_fourcc_to_string (video_info.format));
        return false;
    }

    return true;
}

void
CLImage::init_desc_by_image ()
{
    size_t width = 0, height = 0, row_pitch = 0, mem_size = 0;
    cl_image_format format = {CL_R, CL_UNORM_INT8};

    get_cl_image_info (CL_IMAGE_FORMAT, sizeof(format), &format);
    get_cl_image_info (CL_IMAGE_WIDTH, sizeof(width), &width);
    get_cl_image_info (CL_IMAGE_HEIGHT, sizeof(height), &height);
    get_cl_image_info (CL_IMAGE_ROW_PITCH, sizeof(row_pitch), &row_pitch);
    get_cl_mem_info (CL_MEM_SIZE, sizeof(mem_size), &mem_size);

    _image_desc.format = format;
    _image_desc.width = width;
    _image_desc.height = height;
    _image_desc.row_pitch = row_pitch;
    _image_desc.size = mem_size;
}

CLVaImage::CLVaImage (
    SmartPtr<CLContext> &context,
    SmartPtr<DrmBoBuffer> &bo,
    uint32_t offset)
    : CLImage (context)
    , _bo (bo)
{
    CLImageDesc cl_desc;

    const VideoBufferInfo & video_info = bo->get_video_info ();
    if (!video_info_2_cl_image_desc (video_info, cl_desc)) {
        XCAM_LOG_WARNING ("CLVaImage create va image failed on default videoinfo");
        return;
    }

    init_va_image (context, bo, cl_desc, offset);
}

CLVaImage::CLVaImage (
    SmartPtr<CLContext> &context,
    SmartPtr<DrmBoBuffer> &bo,
    const CLImageDesc &image_info,
    uint32_t offset)
    : CLImage (context)
    , _bo (bo)
{
    init_va_image (context, bo, image_info, offset);
}

bool
CLVaImage::init_va_image (
    SmartPtr<CLContext> &context, SmartPtr<DrmBoBuffer> &bo,
    const CLImageDesc &cl_desc, uint32_t offset)
{

    uint32_t bo_name = 0;
    cl_mem mem_id = 0;
    cl_libva_image va_image_info;

    xcam_mem_clear (&va_image_info);
    va_image_info.offset = offset;
    va_image_info.width = cl_desc.width;
    va_image_info.height = cl_desc.height;
    va_image_info.fmt = cl_desc.format;
    va_image_info.row_pitch = cl_desc.row_pitch;

    if (drm_intel_bo_flink (bo->get_bo (), &bo_name) != 0) {
        XCAM_LOG_WARNING ("CLVaImage get bo flick failed");
        return false;
    }

    va_image_info.bo_name = bo_name;
    mem_id = context->create_va_image (va_image_info);
    if (mem_id == NULL) {
        XCAM_LOG_WARNING ("create va image failed");
        return false;
    }

    set_mem_id (mem_id);
    init_desc_by_image ();
    _va_image_info = va_image_info;
    return true;
}


CLImage2D::CLImage2D (
    SmartPtr<CLContext> &context,
    const VideoBufferInfo &video_info,
    cl_mem_flags  flags)
    : CLImage (context)
{
    CLImageDesc cl_desc;

    if (!video_info_2_cl_image_desc (video_info, cl_desc)) {
        XCAM_LOG_WARNING ("CLVaImage create va image failed on default videoinfo");
        return;
    }

    init_image_2d (context, cl_desc, flags);
}

bool CLImage2D::init_image_2d (
    SmartPtr<CLContext> &context,
    const CLImageDesc &desc,
    cl_mem_flags  flags)
{
    cl_mem mem_id = 0;
    cl_image_desc cl_desc;

    xcam_mem_clear (&cl_desc);
    cl_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    cl_desc.image_width = desc.width;
    cl_desc.image_height = desc.height;
    cl_desc.image_depth = 1;
    cl_desc.image_array_size = 0;
    cl_desc.image_row_pitch = 0;
    cl_desc.image_slice_pitch = 0;
    cl_desc.num_mip_levels = 0;
    cl_desc.num_samples = 0;
    cl_desc.buffer = NULL;

    mem_id = context->create_image (flags, desc.format, cl_desc);
    if (mem_id == NULL) {
        XCAM_LOG_WARNING ("CLImage2D create image 2d failed");
        return false;
    }
    set_mem_id (mem_id);
    init_desc_by_image ();
    return true;
}

};
