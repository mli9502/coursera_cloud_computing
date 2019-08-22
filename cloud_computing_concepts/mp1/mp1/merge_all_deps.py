#!/usr/bin/python3.6
'''
This script will merge all the dependencies that a given .cpp/.h pair requires into the given .cpp/.h file.
Note that the script requires all .h file has a corresponding .cpp file.
'''

import copy
import datetime
import os
import re
import time

from pprint import pprint

std_header_regex = re.compile(r"\#include \<(.*?)\>")
header_regex = re.compile(r"\#include \"(.*?)\"")

target_src_file = 'MP1Node.cpp'
target_header_file = 'MP1Node.h'

def get_gen_file_name(tlu, suffix):
    return '%s.merged.%s' % (tlu, suffix)

def dfs_topological_sort(dependency_graph, curr_header, visited_set, rtn_list):
    if curr_header in visited_set:
        return
    visited_set.add(curr_header)
    if curr_header not in dependency_graph:
        rtn_list.insert(0, curr_header)
        return
    for child in dependency_graph[curr_header]:
        dfs_topological_sort(dependency_graph, child, visited_set, rtn_list)
    rtn_list.insert(0, curr_header)
    return

def topological_sort(dependency_graph):
    visited_set = set()
    rtn_list = []
    for header in dependency_graph.keys():
        if header not in visited_set:
            dfs_topological_sort(dependency_graph, header, visited_set, rtn_list)
    return rtn_list

# Get the name of the translation unit.
def get_tlu(file_name):
    return file_name.split('.')[0]

# Get all the headers that this TLU depends on.
def get_tlu_dependents(tlu):
    rtn = []
    header_file_name = tlu + '.h'
    with open(header_file_name) as fd:
        lines = fd.readlines()
    src_file_name = tlu + '.cpp'
    if os.path.exists(src_file_name):
        with open(src_file_name) as fd:
            lines.extend(fd.readlines())
    for line in lines:
        m = header_regex.search(line)
        if m and get_tlu(m.group(1)) != tlu:
            rtn.append(get_tlu(m.group(1)))
    return rtn

def dfs_build_tlu_graph(curr_tlu, dependency_graph):
    if curr_tlu in dependency_graph:
        return
    deps = get_tlu_dependents(curr_tlu)
    dependency_graph[curr_tlu] = deps
    for dep in deps:
        dfs_build_tlu_graph(dep, dependency_graph)

# Copy <tlu>.h & <tlu>.cpp to the given file.
# All includes will be stripped.
def cp_tlu_to_file(tlu, merged_file, skip_header):
    lines = []
    if not skip_header:
        tlu_header = tlu + '.h'
        with open(tlu_header) as fd:
            lines = fd.readlines()
    tlu_src = tlu + '.cpp'
    if os.path.exists(tlu_src):
        with open(tlu_src) as fd:
            lines.extend(fd.readlines())
    valid_lines = []
    for line in lines:
        if std_header_regex.search(line) or header_regex.search(line):
            continue
        valid_lines.append(line)
    with open(merged_file, 'a') as fd:
        fd.write(''.join(valid_lines))

# For all the .cpp/.h file, get std headers.
def get_std_headers(tlu_list):
    rtn = set()
    for tlu in tlu_list:
        header_file_name = tlu + '.h'
        with open(header_file_name) as fd:
            lines = fd.readlines()
        src_file_name = tlu + '.cpp'
        if os.path.exists(src_file_name):
            with open(src_file_name) as fd:
                lines.extend(fd.readlines())
        for line in lines:
            m = std_header_regex.search(line)
            if m:
                rtn.add(m.group(1))
    return rtn

# Append the whole file to lines list, excluding header files.
def add_file_to_lines(file, lines):
    with open(file) as fd:
        file_lines = fd.readlines()

    for line in file_lines:
        if std_header_regex.search(line) or header_regex.search(line):
            continue
        lines.append(line)

if __name__ == "__main__":
    ts = time.time()

    root_tlu = get_tlu(target_header_file)
    dependency_graph = {}
    dfs_build_tlu_graph(root_tlu, dependency_graph)
    pprint(dependency_graph)
    for key in dependency_graph.keys():
        if root_tlu in dependency_graph[key]:
            dependency_graph[key].remove(root_tlu)
    pprint(dependency_graph)
    # Get the list of headers from base tlu that we don't need to merge.
    exclude_tlu_list = ['stdincludes', 'Params', 'Member', 'Log', 
                        'EmulNet', 'Queue']
    # Header only libraries. We directly copy these library into root tlu header file.
    header_only_tlu_list = ['util', 'TimeoutMap']

    # Clean up depency graph.
    for exclude_tlu in exclude_tlu_list:
        if exclude_tlu in dependency_graph.keys():
            dependency_graph.pop(exclude_tlu)

    for key in dependency_graph.keys():
        for exclude_tlu in exclude_tlu_list:
            if exclude_tlu in dependency_graph[key]:
                dependency_graph[key].remove(exclude_tlu)
    pprint(dependency_graph)

    # Topological sort all the remaining stuff...
    sorted_tlu = topological_sort(dependency_graph)
    sorted_tlu.reverse()
    pprint(sorted_tlu)

    for header_only_tlu in header_only_tlu_list:
        sorted_tlu.remove(header_only_tlu)
    
    pprint(sorted_tlu)
    
    # Then, copy .h & .cpp for that tlu into the new, merged file.
    merged_file = get_gen_file_name(root_tlu, 'cpp')
    prefix_lines = [
        '// Generated by merge_all_deps.py at: %s\n' % (datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')),
        '#include "%s.h"\n' % (root_tlu)
    ]
    with open(merged_file, 'w') as fd:
        fd.write(''.join(prefix_lines))

    for tlu in sorted_tlu:
        skip_header = (tlu == root_tlu)
        cp_tlu_to_file(tlu, merged_file, skip_header)
    
    # For the merged header file, we need to include all the exclude_tlu_list headers, and all the std headers.
    std_header_set = get_std_headers(sorted_tlu)
    local_header_set = copy.deepcopy(exclude_tlu_list)

    # Generate the merged header file.
    # We just replace all its original header files with std_header_set and local_header_set.
    merged_header = get_gen_file_name(root_tlu, 'h')
    merged_header_lines = [
        '// Generated by merge_all_deps.py at: %s\n' % (datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S'))
    ]

    for std_header in std_header_set:
        merged_header_lines.append('#include <%s>\n' % (std_header))
    for local_header in local_header_set:
        merged_header_lines.append('#include "%s.h"\n' % (local_header))

    for tlu in header_only_tlu_list:
        tlu_header_file = tlu + '.h'
        add_file_to_lines(tlu_header_file, merged_header_lines)
    
    root_header_file = root_tlu + '.h'
    add_file_to_lines(root_header_file, merged_header_lines)
    
    with open(merged_header, 'w') as fd:
        fd.write(''.join(merged_header_lines))

    # Merge main.cpp back to Application.cpp.
    # We need to split out main.cpp at the first place to get gtest to work.
    with open('main.cpp') as fd:
        lines = fd.readlines()
    with open('Application.cpp') as fd:
        application_lines = fd.readlines()
    
    for line in application_lines:
        if header_regex.search(line):
            continue
        lines.append(line)
    
    with open('Application.merged.cpp', 'w') as fd:
        fd.write(''.join(lines))
