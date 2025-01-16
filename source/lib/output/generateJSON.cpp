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

#include "generateJSON.hpp"
#include "output_stream.hpp"
#include "statistics.hpp"
#include "timestamps.hpp"

#include "lib/common/string_entry.hpp"
#include "lib/common/utility.hpp"

#include <rocprofiler-sdk/fwd.h>
#include <rocprofiler-sdk/marker/api_id.h>

#include <utility>

namespace rocprofiler
{
namespace tool
{
json_output::json_output(const output_config&       cfg,
                         std::string_view           filename,
                         JSONOutputArchive::Options _opts)
: stream{get_output_stream(cfg, filename, ".json")}
, archive{new JSONOutputArchive{*stream.stream, _opts}}
{
    archive->setNextName("rocprofiler-sdk-tool");
    archive->startNode();
    archive->makeArray();
}

json_output::~json_output() { close(); }

void
json_output::close()
{
    if(archive && stream)
    {
        archive->finishNode();
        archive.reset();
        stream.close();
    }
}

json_output
open_json(const output_config& cfg)
{
    constexpr auto json_prec   = 16;
    constexpr auto json_indent = JSONOutputArchive::Options::IndentChar::space;
    auto           json_opts   = JSONOutputArchive::Options{json_prec, json_indent, 0};
    auto           filename    = std::string_view{"results"};

    return json_output{cfg, filename, json_opts};
}

void
json_output::start_process()
{
    startNode();
}

void
json_output::finish_process()
{
    finishNode();
}

void
close_json(json_output& json_ar)
{
    json_ar.close();
}

void
write_json(json_output&         json_ar,
           const output_config& cfg,
           const metadata&      tool_metadata,
           uint64_t             pid)
{
    // metadata
    {
        auto timestamps =
            timestamps_t{tool_metadata.process_start_ns, tool_metadata.process_end_ns};

        json_ar.setNextName("metadata");
        json_ar.startNode();
        json_ar(cereal::make_nvp("pid", pid));
        json_ar(cereal::make_nvp("init_time", timestamps.app_start_time));
        json_ar(cereal::make_nvp("fini_time", timestamps.app_end_time));
        json_ar(cereal::make_nvp("config", cfg));
        json_ar(cereal::make_nvp("command", common::read_command_line(pid)));
        json_ar.finishNode();
    }

    json_ar(cereal::make_nvp("agents", tool_metadata.agents));
    json_ar(cereal::make_nvp("counters", tool_metadata.get_counter_info()));

    {
        auto callback_name_info = tool_metadata.callback_names;
        auto buffer_name_info   = tool_metadata.buffer_names;
        auto counter_dims       = tool_metadata.get_counter_dimension_info();
        auto marker_msg_data    = tool_metadata.marker_messages.get();

        json_ar.setNextName("strings");
        json_ar.startNode();
        json_ar(cereal::make_nvp("callback_records", callback_name_info));
        json_ar(cereal::make_nvp("buffer_records", buffer_name_info));
        json_ar(
            cereal::make_nvp("pc_sample_instructions", tool_metadata.get_pc_sample_instructions()));
        json_ar(cereal::make_nvp("pc_sample_comments", tool_metadata.get_pc_sample_comments()));
        json_ar(cereal::make_nvp("marker_api", marker_msg_data));
        json_ar(
            cereal::make_nvp("pc_sample_instructions", tool_metadata.get_pc_sample_instructions()));
        json_ar(cereal::make_nvp("pc_sample_comments", tool_metadata.get_pc_sample_comments()));

        {
            auto _extern_corr_id_strings = std::map<size_t, std::string>{};
            if(cfg.kernel_rename)
            {
                for(auto itr : tool_metadata.external_corr_ids.get())
                {
                    if(itr > 0)
                    {
                        const auto* _str = tool_metadata.get_string_entry(itr);
                        if(_str) _extern_corr_id_strings.emplace(itr, *_str);
                    }
                }
            }

            json_ar.setNextName("correlation_id");
            json_ar.startNode();
            json_ar(cereal::make_nvp("external", _extern_corr_id_strings));
            json_ar.finishNode();
        }

        {
            json_ar.setNextName("counters");
            json_ar.startNode();
            json_ar(cereal::make_nvp("dimension_ids", counter_dims));
            json_ar.finishNode();
        }

        json_ar.finishNode();
    }

    {
        auto kern_sym_data = tool_metadata.get_kernel_symbols();
        auto host_sym_data = tool_metadata.get_host_symbols();
        auto code_obj_data = tool_metadata.get_code_objects();

        json_ar(cereal::make_nvp("code_objects", code_obj_data));
        json_ar(cereal::make_nvp("kernel_symbols", kern_sym_data));
        json_ar(cereal::make_nvp("host_functions", host_sym_data));
    }
}

void
write_json(json_output& json_ar,
           const output_config& /*cfg*/,
           const metadata& /*tool_metadata*/,
           const domain_stats_vec_t&                                        domain_stats,
           generator<rocprofiler_buffer_tracing_hip_api_record_t>&&         hip_api_gen,
           generator<rocprofiler_buffer_tracing_hsa_api_record_t>           hsa_api_gen,
           generator<rocprofiler_buffer_tracing_kernel_dispatch_record_t>   kernel_dispatch_gen,
           generator<rocprofiler_buffer_tracing_memory_copy_record_t>       memory_copy_gen,
           generator<tool_counter_record_t>                                 counter_collection_gen,
           generator<rocprofiler_buffer_tracing_marker_api_record_t>        marker_api_gen,
           generator<rocprofiler_buffer_tracing_scratch_memory_record_t>    scratch_memory_gen,
           generator<rocprofiler_buffer_tracing_rccl_api_record_t>          rccl_api_gen,
           generator<rocprofiler_buffer_tracing_memory_allocation_record_t> memory_allocation_gen,
           generator<rocprofiler_tool_pc_sampling_host_trap_record_t>       pc_sampling_gen)

{
    // summary
    {
        json_ar.setNextName("summary");
        json_ar.startNode();
        json_ar.makeArray();

        for(const auto& itr : domain_stats)
        {
            auto _name = get_domain_column_name(itr.first);
            json_ar.startNode();

            json_ar(cereal::make_nvp("domain", std::string{_name}));
            json_ar(cereal::make_nvp("stats", itr.second));

            json_ar.finishNode();
        }

        json_ar.finishNode();
    }

    {
        json_ar.setNextName("callback_records");
        json_ar.startNode();
        json_ar(cereal::make_nvp("counter_collection", counter_collection_gen));
        json_ar.finishNode();
    }

    {
        json_ar.setNextName("buffer_records");
        json_ar.startNode();
        json_ar(cereal::make_nvp("kernel_dispatch", kernel_dispatch_gen));
        json_ar(cereal::make_nvp("hip_api", hip_api_gen));
        json_ar(cereal::make_nvp("hsa_api", hsa_api_gen));
        json_ar(cereal::make_nvp("marker_api", marker_api_gen));
        json_ar(cereal::make_nvp("rccl_api", rccl_api_gen));
        json_ar(cereal::make_nvp("memory_copy", memory_copy_gen));
        json_ar(cereal::make_nvp("memory_allocation", memory_allocation_gen));
        json_ar(cereal::make_nvp("scratch_memory", scratch_memory_gen));
        json_ar(cereal::make_nvp("pc_sample_host_trap", pc_sampling_gen));
        json_ar.finishNode();
    }
}
}  // namespace tool
}  // namespace rocprofiler
