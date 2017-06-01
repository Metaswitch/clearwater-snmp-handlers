# @file cwc_mib_generator.py
#
# Copyright (C) 2016  Metaswitch Networks Ltd

import re
import os.path

COMMON_MIB = "./CLEARWATER-MIB-COMMON"
PC_EXTRAS = "./CLEARWATER-MIB-PC-EXTRAS"
CWC_EXTRAS = "../CLEARWATER-MIB-CWC-EXTRAS"

PC_MIB_NAME = "./PROJECT-CLEARWATER-MIB"
CWC_MIB_NAME = "../METASWITCH-CLEARWATER-CORE-MIB"


def find_header(data, header_regex):
    header_index = None
    for i, line in enumerate(data):
        if header_regex.match(line):
            header_index = i
            break

    return header_index


def find_end_statement(data):
    end_regex = re.compile(r"^END")
    return find_header(data, end_regex)


def find_section(data, section_regex):
    generic_header_regex = re.compile(r"(^\-\- [\w ]+|^END)")

    module_start_index = find_header(data, section_regex)
    module_end_index = module_start_index + \
                       find_header(data[module_start_index+1:], 
                                   generic_header_regex)

    return module_start_index, module_end_index


def append_compliance(data, compliance_objects):
    compliance_regex = re.compile(r"\-\- Module Compliance")
    compliance_section = find_section(data, compliance_regex)
    data[compliance_section[1]:compliance_section[1]] = compliance_objects[1:]


def append_at_end(data, extra_objects):
    end_index = find_end_statement(data)
    data[end_index:end_index] = extra_objects


def split_compliance_and_entries(extras_data):
    extras_compliance_regex = re.compile(r"\-\- [\w ]+ Module Compliance")
    compliance_section = find_section(extras_data, extras_compliance_regex)

    if not compliance_section[0] == 0:
        raise Exception("Badly formatted extras MIB file")

    end_index = find_end_statement(extras_data)

    compliance_data = extras_data[0:compliance_section[1]]
    entry_data = extras_data[compliance_section[1]:end_index]

    return compliance_data, entry_data


def update_title(data, mib_file_name):
    name = re.search(r"[A-Z\-]+$", mib_file_name).group(0)
    data[0] = name + " DEFINITIONS ::= BEGIN\n"


def generate_mib(extras_file_name, mib_file_name):
    with open(COMMON_MIB, "r") as common_mib:
        mib_lines = common_mib.readlines()

    with open(extras_file_name, "r") as extras:
        extras_data = extras.readlines()
        extras_compliance, extras_entries = split_compliance_and_entries(
                                                                  extras_data)

    update_title(mib_lines, mib_file_name)
    append_compliance(mib_lines, extras_compliance)
    append_at_end(mib_lines, extras_entries)

    with open(mib_file_name, "w") as mib_file:
        mib_file.writelines(mib_lines)


if __name__ == "__main__":
    generate_mib(PC_EXTRAS, PC_MIB_NAME)

    if os.path.isfile(CWC_EXTRAS): 
        generate_mib(CWC_EXTRAS, CWC_MIB_NAME)
