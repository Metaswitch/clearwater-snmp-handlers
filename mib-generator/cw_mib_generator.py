#!/usr/bin/python

# Copyright (C) Metaswitch Networks 2017
# If license terms are provided to you in a COPYING file in the root directory
# of the source code repository by which you are accessing this code, then
# the license outlined in that COPYING file applies to your use.
# Otherwise no rights are granted except for those provided to you by
# Metaswitch Networks in a separate written agreement.

"""Metaswitch Clearwater MIB Generator
Usage:
  cw_mib_generator.py [--cwc-mib-dir=DIR]

Options:
  --cwc-mib-dir=DIR  Directory containing Clearwater Core MIB fragment
"""

import re
import sys
import os.path
from string import Template
from docopt import docopt

# Project Clearwater MIB paths
COMMON_MIB_PATH = "./CLEARWATER-MIB-COMMON"
PC_EXTRAS_PATH = "./CLEARWATER-MIB-PC-EXTRAS"
PC_MIB_OUTPUT_PATH = "../PROJECT-CLEARWATER-MIB"

# Clearwater Core MIB names
CWC_EXTRAS_NAME = "CLEARWATER-MIB-CWC-EXTRAS"
CWC_MIB_OUTPUT_NAME = "METASWITCH-CLEARWATER-CORE-MIB"

# Statement added to top of auto-generated MIBs
EDIT_STATEMENT = "THIS MIB IS BUILT FROM A TEMPLATE - DO NOT EDIT DIRECTLY!"


def print_err_and_exit(error_text):
    # Prints an error message with the specified text, and exits with code 1.
    sys.stderr.write("ERROR: {}\nFailed to generate MIB, exiting\n"
                     .format(error_text))
    sys.exit(1)


def read_mib_fragment(mib_path):
    # Try to read MIB fragment at specified path. Returns read data if
    # successful.
    try:
        with open(mib_path, "r") as mib:
            mib_data = mib.read()
    except IOError as ioe:
        print_err_and_exit("Could not read from MIB fragment:\n - {}"
                           .format(ioe))

    return mib_data


def write_mib_file(mib_path, mib_contents):
    # Try to write specified data to file at specified MIB path.
    try:
        with open(mib_path, "w") as mib_file:
            mib_file.write(mib_contents)
    except IOError as ioe:
        print_err_and_exit("Could not write MIB file:\n - {}".format(ioe))


def generate_title(mib_file_path):
    # Try to extract MIB name from the file path, and append definitions
    # statement.
    try:
        name = re.search(r"[\w\-]+$", mib_file_path).group(0)
    except AttributeError:
        print_err_and_exit("Could not find a valid MIB file name in: '{}'"
                           .format(mib_file_path))

    return name + " DEFINITIONS ::= BEGIN"


def generate_mib(extras_file_path, output_mib_path):
    # Read in the common and specific MIB fragments, fill in templated lines
    # and write the complete MIB file.
    full_common_mib_path = os.path.abspath(COMMON_MIB_PATH)
    raw_common_data = read_mib_fragment(full_common_mib_path)
    common_src_template = Template(raw_common_data)

    substitute_dict = { 'title_statement' : generate_title(output_mib_path),
                        'direct_edit_statement' : EDIT_STATEMENT }

    try:
        common_src = common_src_template.substitute(substitute_dict)
    except ValueError as ve:
        print_err_and_exit(
            "Common MIB fragment contains an invalid placeholder:\n - {}"
            .format(ve))
    except KeyError as ke:
        print_err_and_exit(
            "Common MIB fragment contains unrecognised placeholder: {}"
            .format(ke))

    extras_src = read_mib_fragment(extras_file_path)

    full_mib_data = common_src + extras_src
    write_mib_file(output_mib_path, full_mib_data)


if __name__ == "__main__":
    args = sys.argv[1:]

    # If args are supplied, try to parse the CWC MIB directory.
    if args:
        arguments = docopt(__doc__, argv=args)

        # Validate the parsed directory.
        cwc_dir = os.path.abspath(arguments['--cwc-mib-dir'])
        full_cwc_fragment_path = os.path.join(cwc_dir, CWC_EXTRAS_NAME)
        full_cwc_output_mib_path = os.path.join(cwc_dir, CWC_MIB_OUTPUT_NAME)

        if not os.path.isfile(full_cwc_fragment_path):
            print_err_and_exit(
                "Supplied cwc-mib-dir does not contain CWC MIB fragment '{}'"
                .format(CWC_EXTRAS_NAME))

    else:
        cwc_dir = None
        sys.stdout.write("No Clearwater Core MIB directory supplied, building "
                         "Project Clearwater MIB only\n")

    full_pc_output_mib_path = os.path.abspath(PC_MIB_OUTPUT_PATH)
    full_pc_fragment_path = os.path.abspath(PC_EXTRAS_PATH)

    sys.stdout.write("Generating Project Clearwater MIB at: '{}'\n"
                     .format(full_pc_output_mib_path))

    generate_mib(full_pc_fragment_path, full_pc_output_mib_path)

    sys.stdout.write("Successfully generated Project Clearwater MIB!\n")

    if cwc_dir is not None:
        sys.stdout.write("Generating Clearwater Core MIB at: '{}'\n"
                         .format(full_cwc_output_mib_path))

        generate_mib(full_cwc_fragment_path, full_cwc_output_mib_path)

        sys.stdout.write("Successfully generated Clearwater Core MIB!\n")

    sys.exit(0)
