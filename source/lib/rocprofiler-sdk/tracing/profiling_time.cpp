// MIT License
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "lib/rocprofiler-sdk/tracing/profiling_time.hpp"

namespace rocprofiler
{
namespace tracing
{
hsa_amd_profiling_dispatch_time_t&
operator+=(hsa_amd_profiling_dispatch_time_t& lhs, uint64_t rhs)
{
    lhs.start += rhs;
    lhs.end += rhs;
    return lhs;
}

hsa_amd_profiling_dispatch_time_t&
operator-=(hsa_amd_profiling_dispatch_time_t& lhs, uint64_t rhs)
{
    lhs.start -= rhs;
    lhs.end -= rhs;
    return lhs;
}

hsa_amd_profiling_dispatch_time_t&
operator*=(hsa_amd_profiling_dispatch_time_t& lhs, uint64_t rhs)
{
    lhs.start *= rhs;
    lhs.end *= rhs;
    return lhs;
}

hsa_amd_profiling_async_copy_time_t&
operator+=(hsa_amd_profiling_async_copy_time_t& lhs, uint64_t rhs)
{
    lhs.start += rhs;
    lhs.end += rhs;
    return lhs;
}

hsa_amd_profiling_async_copy_time_t&
operator-=(hsa_amd_profiling_async_copy_time_t& lhs, uint64_t rhs)
{
    lhs.start -= rhs;
    lhs.end -= rhs;
    return lhs;
}

hsa_amd_profiling_async_copy_time_t&
operator*=(hsa_amd_profiling_async_copy_time_t& lhs, uint64_t rhs)
{
    lhs.start *= rhs;
    lhs.end *= rhs;
    return lhs;
}

profiling_time&
profiling_time::operator+=(uint64_t offset)
{
    start += offset;
    end += offset;
    return *this;
}

profiling_time&
profiling_time::operator-=(uint64_t offset)
{
    start -= offset;
    end -= offset;
    return *this;
}

profiling_time&
profiling_time::operator*=(uint64_t scale)
{
    start *= scale;
    end *= scale;
    return *this;
}
}  // namespace tracing
}  // namespace rocprofiler
