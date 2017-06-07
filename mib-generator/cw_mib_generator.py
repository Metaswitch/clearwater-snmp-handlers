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

from __future__ import print_function
import sys
import subprocess
import os.path
from string import Template
from docopt import docopt

# MIB names
COMMON_MIB = "CLEARWATER-MIB-COMMON"
PC_EXTRAS = "CLEARWATER-MIB-PC-EXTRAS"
PC_OUTPUT_MIB = "PROJECT-CLEARWATER-MIB"

CWC_EXTRAS = "CLEARWATER-MIB-CWC-EXTRAS"
CWC_OUTPUT_MIB = "METASWITCH-CLEARWATER-CORE-MIB"

# Statement added to top of auto-generated MIBs
EDIT_STATEMENT = "-- THIS MIB IS BUILT FROM A TEMPLATE - DO NOT EDIT DIRECTLY!"


def main(args):
    common_dir = os.path.dirname(os.path.realpath(__file__))

    if args['--cwc-mib-dir']:
        cwc_dir = os.path.abspath(args['--cwc-mib-dir'])

        print("Generating Clearwater Core MIB at {}".format(cwc_dir))
        generate_mib(
            common_dir, cwc_dir, COMMON_MIB, CWC_EXTRAS, CWC_OUTPUT_MIB)
        print("Successfully generated Clearwater Core MIB!")
    else:
        print("No Clearwater Core MIB directory supplied, building Project "
              "Clearwater MIB only")

    pc_dir = common_dir

    print("Generating Project Clearwater MIB at {}".format(pc_dir))
    generate_mib(common_dir, pc_dir, COMMON_MIB, PC_EXTRAS, PC_OUTPUT_MIB)
    print("Successfully generated Clearwater Core MIB!")


def generate_mib(common_dir, extras_dir, common_mib, extras_mib, output_mib):
    # Find full paths for the fragments and output MIB file
    common_mib_path = os.path.join(common_dir, common_mib)
    extras_mib_path = os.path.join(extras_dir, extras_mib)
    output_mib_path = os.path.join(extras_dir, output_mib)

    # Read common MIB, and fill in templated data
    raw_common_data = read_mib_fragment(common_mib_path)
    common_src_template = Template(raw_common_data)

    substitute_dict = {'title_statement': generate_title(output_mib),
                       'edit_statement': EDIT_STATEMENT}

    try:
        common_src = common_src_template.substitute(substitute_dict)
    except ValueError:
        print("ERROR: Common MIB fragment contains an invalid placeholder!")
        raise
    except KeyError:
        print("ERROR: Common MIB fragment contains unrecognised placeholder!")
        raise

    # Read extras MIB, append to common data, and write the output MIB
    extras_src = read_mib_fragment(extras_mib_path)
    full_mib_data = common_src + extras_src
    write_mib_file(output_mib_path, full_mib_data)

    # Verify that the MIB is valid
    verify_mib(common_dir, output_mib_path)


def generate_title(mib_name):
    return "{} DEFINITIONS ::= BEGIN\n\n{}".format(mib_name, EDIT_STATEMENT)


def read_mib_fragment(mib_path):
    # Try to read MIB fragment at specified path. Returns read data if
    # successful.
    try:
        with open(mib_path, "r") as mib:
            mib_data = mib.read()
    except IOError:
        print("ERROR: Could not read from MIB fragment!")
        raise

    return mib_data


def write_mib_file(mib_path, mib_contents):
    # Try to write specified data to file at specified MIB path.
    try:
        with open(mib_path, "w") as mib_file:
            mib_file.write(mib_contents)
    except IOError:
        print("ERROR: Could not write MIB file!")
        raise


def verify_mib(common_dir, mib_path):
    smilint_conf = os.path.join(common_dir, "smilint.conf")
    try:
        cmd = ['smilint', '-s', '-c', smilint_conf, mib_path]
        p = subprocess.Popen(cmd, stderr=subprocess.PIPE)
        _, stderr = p.communicate()

    except:
        print("ERROR: Could not run smilint!")
        raise

    if p.returncode != 0 or len(stderr) != 0:
        print(stderr)
        sys.exit("ERROR: MIB failed validation!")

if __name__ == "__main__":
    args = docopt(__doc__)
    main(args)
    sys.exit(0)
