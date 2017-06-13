#!/usr/bin/python

# Copyright (C) Metaswitch Networks 2017
# If license terms are provided to you in a COPYING file in the root directory
# of the source code repository by which you are accessing this code, then
# the license outlined in that COPYING file applies to your use.
# Otherwise no rights are granted except for those provided to you by
# Metaswitch Networks in a separate written agreement.

"""Metaswitch Clearwater MIB Generator
Generates the complete PROJECT-CLEARWATER-MIB from base fragments.
If the location of the Clearwater Core MIB fragment is supplied, the
METASWITCH-CLEARWATER-CORE-MIB will be generated instead.

Script should be run directly using Python 2.7 or >3.3.

NOTE: If the supplied output directory already contains a file with the same
name as the output MIB, it will be over-written!

Usage:
  cw_mib_generator.py [--cwc-mib-dir=DIR] OUTPUT_DIR

Arguments:
    OUTPUT_DIR       Directory for writing output MIBs

Options:
  --cwc-mib-dir=DIR  Directory containing Clearwater Core MIB fragment
"""

from __future__ import print_function
import sys
import subprocess
import os.path
from string import Template
from docopt import docopt

# Name of common MIB fragment - containing the imports, module-identity, full
# list of node OIDs, and all stat entries common to both output MIBs
COMMON_MIB = "CLEARWATER-MIB-COMMON"

# Name of Project Clearwater MIB fragment - containing the conformance
# statements and entries for modules not included in Clearwater Core
PC_EXTRAS = "CLEARWATER-MIB-PC-EXTRAS"

# Name of Clearwater Core MIB fragment - containing the conformance statements
# and entries for modules exclusive to Clearwater Core
CWC_EXTRAS = "CLEARWATER-MIB-CWC-EXTRAS"

# Names of the completed output MIB files
PC_MIB_NAME = "PROJECT-CLEARWATER-MIB"
CWC_MIB_NAME = "METASWITCH-CLEARWATER-CORE-MIB"

# MIBs must only be modified via the fragments and not the auto-generated
# files - so add this extra statement to the auto-generated MIBs.
EDIT_STATEMENT = "-- THIS MIB IS BUILT FROM A TEMPLATE - DO NOT EDIT DIRECTLY!"


def main(args):
    """
    Generates the MIB for Project Clearwater. If a directory containing the
    Clearwater Core fragment is supplied in args, also generates the MIB
    for Clearwater Core.
    """
    common_dir = os.path.dirname(os.path.realpath(__file__))
    output_dir = os.path.abspath(args['OUTPUT_DIR'])

    if args['--cwc-mib-dir']:
        cwc_dir = os.path.abspath(args['--cwc-mib-dir'])

        print("Generating Clearwater Core MIB at {}".format(output_dir))
        generate_mib(common_dir,
                     cwc_dir,
                     output_dir,
                     COMMON_MIB,
                     CWC_EXTRAS,
                     CWC_MIB_NAME)
        print("Successfully generated Clearwater Core MIB!")
    else:
        # PC MIB fragment is found in the common directory, so supply
        # common_dir as the extras directory also.
        print("Generating Project Clearwater MIB at {}".format(output_dir))
        generate_mib(common_dir,
                     common_dir,
                     output_dir,
                     COMMON_MIB,
                     PC_EXTRAS,
                     PC_MIB_NAME)
        print("Successfully generated Project Clearwater MIB!")


def generate_mib(common_dir,
                 extras_dir,
                 output_dir,
                 common_mib_name,
                 extras_mib_name,
                 output_mib_name):
    """
    Generates a MIB file by combining a common MIB fragment and an extra MIB
    fragment. The output MIB is written to the same directory as the extra
    MIB file.

    Args:
        common_dir (str): Directory containing common MIB fragment.
        extras_dir (str): Directory containing extra MIB fragment.
        output_dir (str): Output directory for full MIB.
        common_mib_name (str): File name of common MIB.
        extras_mib_name (str): File name of extras MIB.
        output_mib_name (str): File name for full MIB. NOTE: If this file
            already exists in output_dir, it will be over-written!
    """

    common_mib_path = os.path.join(common_dir, common_mib_name)
    extras_mib_path = os.path.join(extras_dir, extras_mib_name)
    output_mib_path = os.path.join(output_dir, output_mib_name)

    # The MIB title line is templated, as it needs to contain the file name of
    # the MIB. The EDIT_STATEMENT is also added after the title - we don't
    # include this statement in the fragments, as we want them to be edited.
    raw_common_data = read_mib_fragment(common_mib_path)
    common_src_template = Template(raw_common_data)
    title_statement = "{} DEFINITIONS ::= BEGIN\n\n{}".format(
        output_mib_name, EDIT_STATEMENT)
    substitute_dict = {'title_statement': title_statement}

    try:
        common_src = common_src_template.substitute(substitute_dict)
    except ValueError:
        print("ERROR: Common MIB fragment contains an invalid placeholder!")
        raise
    except KeyError:
        print("ERROR: Common MIB fragment contains unrecognised placeholder!")
        raise

    # The extras MIB files contain no templating, so can be appended straight
    # onto the common MIB data.
    extras_src = read_mib_fragment(extras_mib_path)
    full_mib_data = common_src + extras_src

    write_mib_file(output_mib_path, full_mib_data)
    verify_mib(common_dir, output_mib_path)


def read_mib_fragment(mib_path):
    """
    Read MIB fragment at specified path. Returns read data if successful.

    Args:
        mib_path (str): Path to MIB file to read

    Returns:
        str: Contents of file
    """
    try:
        with open(mib_path, "r") as mib:
            mib_data = mib.read()
    except IOError:
        print("ERROR: Could not read from MIB fragment!")
        raise

    return mib_data


def write_mib_file(mib_path, mib_contents):
    """
    Write given string at specified path.

    Args:
        mib_path (str): Path of file to write
        mib_contents (str): Data to write
    """
    try:
        with open(mib_path, "w") as mib_file:
            mib_file.write(mib_contents)
    except IOError:
        print("ERROR: Could not write MIB file!")
        raise


def verify_mib(common_dir, mib_path):
    """
    Verify that the MIB contains no errors. If an error is found, exit with
    response code 1. (Any syntax warnings will be ignored, as they should be
    tolerated by all implementations - they are simply recommendations.)

    MIB is verified using smilint, a syntax/semantic checker for SMIv1/v2 MIB
    modules. In order for this function to work, smilint must be installed,
    and the required IETF MIBs must be located at /var/lib/mibs/ietf/. Extra
    config is supplied to smilint to provide this IETF MIB folder.

    Args:
        common_dir (str): Common MIB fragment directory, containing the extra
            smilint config file.
        mib_path (str): Path to MIB to be verified.
    """

    smilint_conf = os.path.join(common_dir, "smilint.conf")
    try:
        cmd = ['smilint', '-s', '-c', smilint_conf, mib_path]

        # If errors are found in the MIB but smilint doesn't exit abnormally,
        # then it still exits with code 0, so we also need to read the stderr
        # to check for any errors.
        p = subprocess.Popen(cmd, stderr=subprocess.PIPE)
        _, stderr = p.communicate()

    except:
        print("ERROR: Could not run smilint!")
        raise

    if p.returncode != 0 or stderr:
        print(stderr)
        sys.exit("ERROR: MIB failed validation!")

if __name__ == "__main__":
    args = docopt(__doc__)
    main(args)
    sys.exit(0)
