import os

def get_sources():
    sources = {}
    for file in sorted(os.listdir("python")):
        if not file.endswith(".py"):
            continue
        key = file.split(".")[0]
        const_char_array = []
        with open("python/" + file) as f:
            # convert to char array (signed)
            for c in f.read().encode('utf-8'):
                if c < 128:
                    const_char_array.append(str(c))
                else:
                    const_char_array.append(str(c - 256))
        const_char_array.append('0')
        const_char_array = ','.join(const_char_array)
        sources[key] = '{' + const_char_array + '}'
    return sources

sources = get_sources()

# use LF line endings instead of CRLF
with open("include/pocketpy/_generated.h", "wt", encoding='utf-8', newline='\n') as f:
    data = '''#pragma once
// generated by prebuild.py

namespace pkpy{
'''
    for key in sorted(sources.keys()):
        value = sources[key]
        data += f'    extern const char kPythonLibs_{key}[];\n'
    data += '}    // namespace pkpy\n'
    f.write(data)

with open("src/_generated.cpp", "wt", encoding='utf-8', newline='\n') as f:
    data = '''// generated by prebuild.py
#include "pocketpy/_generated.h"

namespace pkpy{
'''
    for key in sorted(sources.keys()):
        value = sources[key]
        data += f'    const char kPythonLibs_{key}[] = {value};\n'
    data += '}    // namespace pkpy\n'
    f.write(data)
