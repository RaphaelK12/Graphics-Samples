#
#   This file is a part of Jiayin's Graphics Samples.
#   Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
#

# This script offers an interface to convert glsl code to spirv headers, which will be used during compiling.
# It is supposed to be called this way
#   generate_shader.py [shader_source_code] [output_filename] [glslangValidator.exe]

import os
import sys
import subprocess
import struct
import re

SPIRV_MAGIC = 0x07230203
COLUMNS = 8
INDENT = 4

in_filename     = sys.argv[1]
out_filename    = sys.argv[2]
executable      = sys.argv[3]

def identifierize(s):
    # translate invalid chars
    s = re.sub("[^0-9a-zA-Z_]", "_", s)
    # translate leading digits
    return re.sub("^[^a-zA-Z_]+", "_", s)

# compile glsl code to spirv
def compile(filename, tmpfile):
    # invoke glslangValidator
    try:
        # spawn a process to generate shader binaries
        args = [executable, "-V", "-H", "-o", tmpfile, filename]
        output = subprocess.check_output(args, universal_newlines=True)
    except subprocess.CalledProcessError as e:
        print(e.output, file=sys.stderr)
        exit(1)

    # read the temp file into a list of SPIR-V words
    words = []
    with open(tmpfile, "rb") as f:
        data = f.read()
        assert(len(data) and len(data) % 4 == 0)

        # determine endianness
        fmt = ("<" if data[0] == (SPIRV_MAGIC & 0xff) else ">") + "I"
        for i in range(0, len(data), 4):
            words.append(struct.unpack(fmt, data[i:(i + 4)])[0])

        assert(words[0] == SPIRV_MAGIC)

    # remove temp file
    os.remove(tmpfile)

    return (words, output.rstrip())

base = os.path.basename(in_filename)
words, comments = compile(in_filename, base + ".tmp")

literals = []
for i in range(0, len(words), COLUMNS):
    columns = ["0x%08x" % word for word in words[i:(i + COLUMNS)]]
    literals.append(" " * INDENT + ", ".join(columns) + ",")

# this string merges the generate meta data and shader blob data
header = """//
//   This file is a part of Jiayin's Graphics Samples.
//   Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

// This file is generated, don't attempt to modify it.

#if 0
%s
#endif

static const unsigned int %s[%d] = {
%s
};
""" % (comments, identifierize(base), len(words), "\n".join(literals))

# output the generated header
if out_filename:
    with open(out_filename, "w") as f:
        print(header, end="", file=f)
else:
        print(header, end="")