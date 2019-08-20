#!/usr/bin/python3.6
'''
This script will merge all the dependencies that a given .cpp/.h pair requires into the given .cpp/.h file.
Note that the script requires all .h file has a corresponding .cpp file.
'''

import re

from pprint import pprint

std_header_regex = re.compile(r"\#include \<(.*?)\>")
header_regex = re.compile(r"\#include \"(.*?)\"")

target_src_file = 'MP1Node.cpp'
target_header_file = 'MP1Node.h'

# First, read the cpp file and get all the non-std header files recursively.
def dfs_get_header_file(curr_header_file, dependency_graph, std_header_list):
    with open(curr_header_file) as fd:
        lines = fd.readlines()
    for line in lines:
        m = std_header_regex.search(line)
        if m:
            std_header_list.append(m.group(1))
        m = header_regex.search(line)
        if m:
            header = m.group(1)
            visited = header in dependency_graph
            if header not in dependency_graph:
                dependency_graph[header] = []
                dependency_graph[header].append(curr_header_file)
                dfs_get_header_file(header, dependency_graph, std_header_list)
            else:
                dependency_graph[header].append(curr_header_file)
    
if __name__ == "__main__":
    curr_header_file = target_header_file
    dependency_graph = {}
    std_header_list = []

    dfs_get_header_file(curr_header_file, dependency_graph, std_header_list)

    print(dependency_graph)
