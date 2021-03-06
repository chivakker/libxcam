/*
 * buffer_pool.h - buffer pool
 *
 *  Copyright (c) 2014-2015 Intel Corporation
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

#ifndef XCAM_BUFFER_POOL_H
#define XCAM_BUFFER_POOL_H

#include "xcam_utils.h"
#include "smartptr.h"
#include "safe_list.h"
#include "video_buffer.h"

namespace XCam {

class BufferPool;

class BufferData {
public:
    explicit BufferData () {}
    virtual ~BufferData () {}

    virtual uint8_t *map () = 0;
    virtual bool unmap () = 0;

private:
    XCAM_DEAD_COPY (BufferData);
};

class BufferProxy
    : public VideoBuffer
{
public:
    explicit BufferProxy (const VideoBufferInfo &info, const SmartPtr<BufferData> &data);
    explicit BufferProxy (const SmartPtr<BufferData> &data);
    virtual ~BufferProxy ();

    void set_parent (const SmartPtr<VideoBuffer> &parent) {
        _parent = parent;
    }
    void set_buf_pool (const SmartPtr<BufferPool> &pool) {
        _pool = pool;
    }

    // derived from VideoBuffer
    virtual uint8_t *map ();
    virtual bool unmap ();

protected:
    SmartPtr<BufferData> &get_buffer_data () {
        return _data;
    }

private:
    XCAM_DEAD_COPY (BufferProxy);

private:
    SmartPtr<BufferData>       _data;
    SmartPtr<BufferPool>       _pool;
    SmartPtr<VideoBuffer>      _parent;
};

class BufferPool {
    friend class BufferProxy;

public:
    explicit BufferPool ();
    virtual ~BufferPool ();

    bool set_video_info (const VideoBufferInfo &info);
    bool reserve (uint32_t max_count = 4);
    SmartPtr<BufferProxy> get_buffer (const SmartPtr<BufferPool> &self);

    void stop ();

    const VideoBufferInfo & get_video_info () const {
        return _buffer_info;
    }

protected:
    virtual bool fixate_video_info (VideoBufferInfo &info);
    virtual SmartPtr<BufferData> allocate_data (const VideoBufferInfo &buffer_info) = 0;
    virtual SmartPtr<BufferProxy> create_buffer_from_data (SmartPtr<BufferData> &data);

    bool add_data_unsafe (SmartPtr<BufferData> data);

private:
    void release (SmartPtr<BufferData> &data);
    XCAM_DEAD_COPY (BufferPool);

private:
    Mutex                    _mutex;
    VideoBufferInfo          _buffer_info;
    SafeList<BufferData>     _buf_list;
    uint32_t                 _allocated_num;
    uint32_t                 _max_count;
    bool                     _started;
};

};

#endif //XCAM_BUFFER_POOL_H

