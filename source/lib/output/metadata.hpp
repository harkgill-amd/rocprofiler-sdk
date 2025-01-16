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

#pragma once

#include "agent_info.hpp"
#include "counter_info.hpp"
#include "host_symbol_info.hpp"
#include "kernel_symbol_info.hpp"
#include "pc_sample_transform.hpp"

#include "lib/common/container/small_vector.hpp"
#include "lib/common/demangle.hpp"
#include "lib/common/logging.hpp"
#include "lib/common/synchronized.hpp"

#include <rocprofiler-sdk/agent.h>
#include <rocprofiler-sdk/buffer_tracing.h>
#include <rocprofiler-sdk/callback_tracing.h>
#include <rocprofiler-sdk/fwd.h>
#include <rocprofiler-sdk/rocprofiler.h>
#include <rocprofiler-sdk/cxx/codeobj/code_printing.hpp>
#include <rocprofiler-sdk/cxx/hash.hpp>
#include <rocprofiler-sdk/cxx/name_info.hpp>
#include <rocprofiler-sdk/cxx/operators.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define ROCPROFILER_CHECK_NESTED(VAR, RESULT)                                                      \
    {                                                                                              \
        rocprofiler_status_t ROCPROFILER_VARIABLE(CHECKSTATUS, VAR) = RESULT;                      \
        if(ROCPROFILER_VARIABLE(CHECKSTATUS, VAR) != ROCPROFILER_STATUS_SUCCESS)                   \
        {                                                                                          \
            std::string_view status_msg =                                                          \
                rocprofiler_get_status_string(ROCPROFILER_VARIABLE(CHECKSTATUS, VAR));             \
            ROCP_FATAL << "[" << __FUNCTION__ << "] " << #RESULT << " failed with error code "     \
                       << ROCPROFILER_VARIABLE(CHECKSTATUS, VAR) << " :: " << status_msg;          \
        }                                                                                          \
    }

#define ROCPROFILER_CHECK(RESULT) ROCPROFILER_CHECK_NESTED(__COUNTER__, RESULT)

namespace rocprofiler
{
namespace tool
{
using marker_message_map_t         = std::unordered_map<uint64_t, std::string>;
using marker_message_ordered_map_t = std::map<uint64_t, std::string>;
using string_entry_map_t           = std::unordered_map<size_t, std::unique_ptr<std::string>>;
using counter_dimension_vec_t      = std::vector<rocprofiler_record_dimension_info_t>;
using external_corr_id_set_t       = std::unordered_set<uint64_t>;
using code_obj_decoder_t = rocprofiler::sdk::codeobj::disassembly::CodeobjAddressTranslate;
using instruction_t      = rocprofiler::sdk::codeobj::disassembly::Instruction;
template <typename Tp>
using synced_map = common::Synchronized<Tp, true>;

struct metadata
{
    using agent_info_ptr_vec_t = common::container::small_vector<const agent_info*, 16>;

    struct inprocess
    {};

    pid_t                             process_id                  = 0;
    uint64_t                          process_start_ns            = 0;
    uint64_t                          process_end_ns              = 0;
    agent_info_vec_t                  agents                      = {};
    agent_info_map_t                  agents_map                  = {};
    agent_counter_info_map_t          agent_counter_info          = {};
    agent_pc_sample_config_info_map_t agent_pc_sample_config_info = {};

    sdk::buffer_name_info                buffer_names      = {};
    sdk::callback_name_info              callback_names    = {};
    synced_map<code_object_data_map_t>   code_objects      = {};
    synced_map<kernel_symbol_data_map_t> kernel_symbols    = {};
    synced_map<marker_message_map_t>     marker_messages   = {};
    synced_map<string_entry_map_t>       string_entries    = {};
    synced_map<external_corr_id_set_t>   external_corr_ids = {};
    synced_map<host_function_info_map_t> host_functions    = {};

    metadata() = default;
    metadata(inprocess);

    ~metadata()                   = default;
    metadata(const metadata&)     = delete;
    metadata(metadata&&) noexcept = delete;
    metadata& operator=(const metadata&) = delete;
    metadata& operator=(metadata&&) noexcept = delete;

    void init(inprocess);

    const agent_info*                   get_agent(rocprofiler_agent_id_t _val) const;
    const code_object_info*             get_code_object(uint64_t code_obj_id) const;
    const kernel_symbol_info*           get_kernel_symbol(uint64_t kernel_id) const;
    const host_function_info*           get_host_function(uint64_t host_function_id) const;
    const tool_counter_info*            get_counter_info(uint64_t instance_id) const;
    const tool_counter_info*            get_counter_info(rocprofiler_counter_id_t id) const;
    const counter_dimension_info_vec_t* get_counter_dimension_info(uint64_t instance_id) const;

    code_object_data_vec_t   get_code_objects() const;
    kernel_symbol_data_vec_t get_kernel_symbols() const;
    host_function_data_vec_t get_host_symbols() const;
    agent_info_ptr_vec_t     get_gpu_agents() const;
    counter_info_vec_t       get_counter_info() const;
    counter_dimension_vec_t  get_counter_dimension_info() const;
    pc_sample_config_vec_t   get_pc_sample_config_info(rocprofiler_agent_id_t _val) const;
    std::vector<std::string> get_pc_sample_instructions() const { return instruction_decoder; }
    std::vector<std::string> get_pc_sample_comments() const { return instruction_comment; }
    std::string_view get_instruction(int64_t index) const { return instruction_decoder.at(index); }
    std::string_view get_comment(int64_t index) const { return instruction_comment.at(index); }
    int64_t          get_instruction_index(rocprofiler_pc_t record);
    void             add_decoder(rocprofiler_code_object_info_t* obj_data_v);

    template <typename Tp>
    Tp get_marker_messages(Tp&&);

    bool add_marker_message(uint64_t corr_id, std::string&& msg);
    bool add_code_object(code_object_info obj);
    bool add_kernel_symbol(kernel_symbol_info&& sym);
    bool add_host_function(host_function_info&& func);
    bool add_string_entry(size_t key, std::string_view str);
    bool add_external_correlation_id(uint64_t);

    std::string_view   get_marker_message(uint64_t corr_id) const;
    std::string_view   get_kernel_name(uint64_t kernel_id, uint64_t rename_id) const;
    std::string_view   get_kind_name(rocprofiler_callback_tracing_kind_t kind) const;
    std::string_view   get_kind_name(rocprofiler_buffer_tracing_kind_t kind) const;
    std::string_view   get_operation_name(rocprofiler_callback_tracing_kind_t kind,
                                          rocprofiler_tracing_operation_t     op) const;
    std::string_view   get_operation_name(rocprofiler_buffer_tracing_kind_t kind,
                                          rocprofiler_tracing_operation_t   op) const;
    uint64_t           get_node_id(rocprofiler_agent_id_t _val) const;
    const std::string* get_string_entry(size_t key) const;

private:
    bool                           inprocess_init = false;
    std::unique_ptr<instruction_t> decode_instruction(rocprofiler_pc_t pc);
    synced_map<code_obj_decoder_t> decoder = {};
    // TODO: We may have to reserve the vector size based on map size
    std::vector<std::string> instruction_decoder = {};
    std::vector<std::string> instruction_comment = {};
    std::map<inst_t, size_t> indexes             = {};
};

template <typename Tp>
Tp
metadata::get_marker_messages(Tp&& _inp)
{
    return marker_messages.rlock(
        [](const auto& _data_v, auto&& _inp_v) {
            for(const auto& itr : _data_v)
                _inp_v.emplace(itr.first, itr.second);
            return _inp_v;
        },
        std::move(_inp));
}
}  // namespace tool
}  // namespace rocprofiler
