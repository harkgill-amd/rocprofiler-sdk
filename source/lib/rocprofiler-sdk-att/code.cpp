// MIT License
//
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "code.hpp"
#include <nlohmann/json.hpp>
#include "outputfile.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace rocprofiler
{
namespace att_wrapper
{
#define ATT_CSV_NAME "att_output.csv"

// Builds a json filetree by recursively inserting "path" into the json object.
void
navigate(nlohmann::json& json, std::vector<std::string>& path, const std::string& filename)
{
    if(path.size() == 1) json[path.at(0)] = filename;

    if(path.size() <= 1) return;

    auto& j = json[path.at(0)];
    path.erase(path.begin());
    navigate(j, path, filename);
}

CodeFile::CodeFile(const Fspath& _dir, std::shared_ptr<AddressTable> _table)
: dir(_dir)
, filename(_dir / "code.json")
, table(_table)
{}

CodeFile::~CodeFile()
{
    std::vector<std::pair<pcinfo_t, std::unique_ptr<CodeLine>>> vec;
    vec.reserve(isa_map.size());

    for(auto& [pc, isa] : isa_map)
        if(isa && isa->code_line) vec.emplace_back(pc, std::move(isa));

    isa_map.clear();
    line_numbers.clear();

    if(GlobalDefs::get().has_format("csv"))
    {
        // Write CSV, ordered by id + vaddr
        std::sort(vec.begin(),
                  vec.end(),
                  [](const std::pair<pcinfo_t, std::unique_ptr<CodeLine>>& a,
                     const std::pair<pcinfo_t, std::unique_ptr<CodeLine>>& b) {
                      if(a.first.marker_id == b.first.marker_id) return a.first.addr < b.first.addr;
                      return a.first.marker_id < b.first.marker_id;
                  });

        OutputFile file(dir / ATT_CSV_NAME);

        file << "CodeObj, Vaddr, Instruction, Hitcount, Latency, Source\n";
        for(auto& [pc, line] : vec)
        {
            if(kernel_names.find(pc) != kernel_names.end())
            {
                file << pc.marker_id << ',' << pc.addr << ",\"; " << kernel_names.at(pc).name
                     << "\",0,0,\"" << kernel_names.at(pc).demangled << "\"\n";
            }
            file << pc.marker_id << ',' << pc.addr << ",\"" << line->code_line->inst << "\","
                 << line->hitcount << ',' << line->latency << ',' << line->code_line->comment
                 << '\n';
        }
    }

    if(!GlobalDefs::get().has_format("json")) return;

    // Write JSON, ordered by exec line number
    std::sort(vec.begin(),
              vec.end(),
              [](const std::pair<pcinfo_t, std::unique_ptr<CodeLine>>& a,
                 const std::pair<pcinfo_t, std::unique_ptr<CodeLine>>& b) {
                  return a.second->line_number < b.second->line_number;
              });

    nlohmann::json jcode;

    std::unordered_set<std::string> snapshots{};

    for(auto& line : vec)
    {
        auto& isa = *line.second;

        if(kernel_names.find(line.first) != kernel_names.end())
        {
            std::stringstream code;
            code << "[\"; " << kernel_names.at(line.first).name << "\", 100, "
                 << (isa.line_number - 1) << ", \"" << kernel_names.at(line.first).demangled
                 << "\", " << line.first.marker_id << ", " << line.first.addr << ", 0, 0]";

            jcode.push_back(nlohmann::json::parse(code.str()));
        }

        std::stringstream code;
        code << "[\"" << isa.code_line->inst << "\", 0, " << isa.line_number << ", \""
             << isa.code_line->comment << "\", " << line.first.marker_id << ", " << line.first.addr
             << ", " << isa.hitcount << ", " << isa.latency << "]";

        jcode.push_back(nlohmann::json::parse(code.str()));

        size_t lineref = isa.code_line->comment.rfind(':');
        if(lineref == 0 || lineref == std::string::npos) continue;

        auto source_ref = isa.code_line->comment.substr(0, lineref);

        if(!source_ref.empty() && snapshots.find(source_ref) == snapshots.end())
            snapshots.insert(std::move(source_ref));
    }

    nlohmann::json json;
    json["code"]    = jcode;
    json["version"] = TOOL_VERSION;

    OutputFile(filename) << json;

    nlohmann::json jsnapfiletree;
    size_t         num_snap = 0;

    for(auto& source_ref : snapshots)
    {
        if(rocprofiler::common::filesystem::exists(source_ref))
        {
            Fspath            filepath(source_ref);
            std::stringstream newfile;
            newfile << "source_" << (num_snap++) << '_' << filepath.filename().string();

            std::vector<std::string> path_elements(filepath.begin(), filepath.end());
            navigate(jsnapfiletree, path_elements, newfile.str());
            rocprofiler::common::filesystem::copy(filepath, dir / newfile.str());
        }
    }

    if(num_snap != 0) OutputFile(dir / "snapshots.json") << jsnapfiletree;
}

}  // namespace att_wrapper
}  // namespace rocprofiler
