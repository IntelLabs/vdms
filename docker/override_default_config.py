#!/usr/bin/python3
#
# The MIT License
#
# @copyright Copyright (c) 2024 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import os
import json
import argparse
from pathlib import Path


class JSONCustomDecoder(json.JSONDecoder):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def decode(self, str_to_decode: str):
        comments_replaced = [
            line if not line.lstrip().startswith("//") else ""
            for line in str_to_decode.split("\n")
        ]
        comments_replaced = []
        for line in str_to_decode.split("\n"):
            if "//" not in line:
                new_value = line
            elif not line.lstrip().startswith("//") and "//" in line:
                comment_idx = line.find("//")
                new_value = line[:comment_idx]
            else:
                new_value = ""
            comments_replaced.append(new_value)
        str_to_decode = "\n".join(comments_replaced)
        return super().decode(str_to_decode)


def get_arguments():
    """
    PURPOSE: This function gets the arguments needed
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-i",
        type=Path,
        dest="config_path",
        default="/vdms/config-vdms.json",
        help="Path of config-vdms.json to update",
    )
    parser.add_argument(
        "-o",
        type=Path,
        dest="out_path",
        default="/vdms/build/config-vdms.json",
        help="Path to write updated config-vdms.json",
    )

    params = parser.parse_args()
    params.config_path = params.config_path.absolute()

    return params


def main(args):
    with open(str(args.config_path), "r") as infile:
        config = json.load(infile, cls=JSONCustomDecoder)

    updated_params = []
    for env_var, env_value in os.environ.items():
        if env_var.startswith("OVERRIDE_"):
            updated_config_key = env_var.replace("OVERRIDE_", "").lower()

            try:
                updated_config_value = int(env_value)
            except:
                updated_config_value = env_value

            config[updated_config_key] = updated_config_value
            updated_params.append(updated_config_key)

    if len(updated_params) > 0:
        print("Updating config parameters")
        for key in updated_params:
            new_value = config[key]
            print(f"\t{key}: {new_value}")

    with open(str(args.out_path), "w") as outfile:
        json.dump(config, outfile, indent=4)


if __name__ == "__main__":
    args = get_arguments()
    main(args)
