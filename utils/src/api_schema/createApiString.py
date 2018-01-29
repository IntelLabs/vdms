import sys
import os

with open(sys.argv[1], 'r') as schema_file:
    file = schema_file.readlines()

    output = open(sys.argv[2],"w")
    output.write("const std::string schema_json(\" ")

    for line in file:
        line = line.replace('\"', '\\\"')
        line = line.replace('\n', '\\\n')

        if not line.find("//") != -1:
            output.write(line)

    output.write("\");\n")