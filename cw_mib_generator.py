#!/usr/bin/python

# Copyright (C) Metaswitch Networks 2017
# If license terms are provided to you in a COPYING file in the root directory
# of the source code repository by which you are accessing this code, then
# the license outlined in that COPYING file applies to your use.
# Otherwise no rights are granted except for those provided to you by
# Metaswitch Networks in a separate written agreement.

import re
import sys
import os.path
from string import Template

# MIB fragment file paths
COMMON_MIB_PATH = "./CLEARWATER-MIB-COMMONS"
PC_EXTRAS_PATH = "./CLEARWATER-MIB-PC-EXTRAS"
CWC_EXTRAS_PATH = "../cwc-build/CLEARWATER-MIB-CWC-EXTRAS"

# File paths for output MIBs
PC_MIB_PATH = "./PROJECT-CLEARWATER-MIB"
CWC_MIB_PATH = "../cwc-build/METASWITCH-CLEARWATER-CORE-MIB"

# Statement added to top of auto-generated MIBs
EDIT_STATEMENT = "THIS MIB IS BUILT FROM A TEMPLATE - DO NOT EDIT DIRECTLY!"


def print_err_and_exit(error_text):
    sys.stderr.write("ERROR: {}\nFailed to generate MIB, exiting\n"
                     .format(error_text))
    sys.exit(1)


def generate_title(mib_file_path):
    # Extract the MIB file name from the file path.
    try:
        name = re.search(r"[A-Z\-]+$", mib_file_path).group(0)
    except AttributeError:
        print_err_and_exit(
            "Could not find a valid MIB file name in file path: '{}'"
            .format(mib_file_path))

    return name + " DEFINITIONS ::= BEGIN"


def generate_mib(extras_file_path, mib_file_path):
    # Read in the common and specific MIB fragments, fill in templated lines
    # and write the complete MIB file.
    full_common_mib_path = os.path.abspath(COMMON_MIB_PATH)
    try:
        with open(full_common_mib_path, "r") as common_mib:
            common_src_template = Template(common_mib.read())
    except IOError:
        print_err_and_exit(
            "Could not read from common MIB fragment at: '{}'"
            .format(full_common_mib_path))

    substitute_dict = { 'title_statement' : generate_title(mib_file_path),
                        'direct_edit_statement' : EDIT_STATEMENT }

    try:
        common_src = common_src_template.substitute(substitute_dict)
    except ValueError as ve:
        print_err_and_exit(
            "Common MIB fragment contains an invalid placeholder:\n - {}"
            .format(ve))
    except KeyError as ke:
        print_err_and_exit(
            "Common MIB fragment contains unrecognised placeholder: '{}'"
            .format(ke))

    try:
        with open(extras_file_path, "r") as extras_mib:
            extras_src = extras_mib.read()
    except IOError:
        print_err_and_exit(
            "Could not read from extra MIB fragment at path: '{}'"
            .format(extras_file_path))

    try:
        with open(mib_file_path, "w") as mib_file:
            mib_file.write(common_src + extras_src)
    except IOError:
        print_err_and_exit(
            "Could not write MIB file at path: '{}'".format(mib_file_path))

if __name__ == "__main__":
    # Always generate PC MIB. If CWC fragment is found, generate CWC MIB.
    # For debugging purposes, find the full file path for each file.
    full_pc_output_mib_path = os.path.abspath(PC_MIB_PATH)
    full_pc_fragment_path = os.path.abspath(PC_EXTRAS_PATH)

    sys.stdout.write("Generating Project Clearwater MIB at: '{}'\n"
                     .format(full_pc_output_mib_path))

    generate_mib(full_pc_fragment_path, full_pc_output_mib_path)

    sys.stdout.write("Successfully generated Project Clearwater MIB!\n")

    full_cwc_fragment_path = os.path.abspath(CWC_EXTRAS_PATH)

    if os.path.isfile(full_cwc_fragment_path):
        full_cwc_output_mib_path = os.path.abspath(CWC_MIB_PATH)

        sys.stdout.write("Generating Clearwater Core MIB at: '{}'\n"
                         .format(full_cwc_output_mib_path))

        generate_mib(full_cwc_fragment_path, full_cwc_output_mib_path)

        sys.stdout.write("Successfully generated Clearwater Core MIB!\n")

    sys.exit(0)
