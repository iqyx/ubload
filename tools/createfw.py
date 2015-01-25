#!/usr/bin/python

import argparse
import struct


class section_magic:
	verification = 0x6ef44bc0
	verified = 0x1eda84bc
	dummy = 0xba50911a
	firmware = 0x40b80c0f

section_names = {
	section_magic.verification: "verification",
	section_magic.verified: "verified",
	section_magic.dummy: "dummy",
	section_magic.firmware: "firmware"
}

def build_section(section_magic, data):
	return struct.pack("!LL", section_magic, len(data)) + data

def pad_metadata(data, req_len):
	if len(data) > req_len:
		return None
	if len(data) == req_len:
		return data
	if len(data) > (req_len - 8):
		return None

	padding_size = req_len - len(data) - 8
	return data + build_section(section_magic.dummy, " " * padding_size)


def print_section(data, offset, indent):
	if indent > 1:
		return "", 0

	magic, length = struct.unpack("!LL", data[:8])
	section_name = section_names[magic];

	# Strip header
	data = data[8:]

	# Get this section data only
	section_data = data[:length]
	section_data_offset = offset + 8
	print "%ssection %s at 0x%08x, data 0x%08x, len %u bytes" % ("\t" * indent, section_name, offset, section_data_offset, length)

	while len(section_data) > 0:
		section_data, section_data_offset = print_section(section_data, section_data_offset, indent + 1)

	# Return the rest.
	return data[length:], offset + 8 + length


def print_sections(data, offset):
	while len(data) > 0:
		data, offset = print_section(data, offset, 0)


parser = argparse.ArgumentParser(description = "uBLoad firmware creator")
parser.add_argument(
	"--input", "-i",
	metavar = "binfile",
	dest = "binfile",
	required = True,
	type = str,
	help = "Input binary file"
)
parser.add_argument(
	"--output", "-o",
	metavar = "fwfile",
	dest = "fwfile",
	required = True,
	type = str,
	help = "Output uBLoad firmware image file"
)
parser.add_argument(
	"--sign", "-s",
	action = "store_true",
	help = "Sign the firmware image. Firmware hash is added too."
)
parser.add_argument(
	"--check", "-c",
	action = "store_true",
	help = "Add firmware hash for integrity checking."
)
parser.add_argument(
	"--hash-type",
	dest = "hash_type",
	metavar = "HASH",
	default = "sha512",
	help = "Specify hash type for signing/integrity checking."
)
parser.add_argument(
	"--base", "-b",
	dest = "fw_base",
	metavar = "hex",
	default = "0x08008000",
	help = "Firmware base address for loading."
)
parser.add_argument(
	"--offset",
	dest = "fw_offset",
	metavar = "hex",
	default = "0x400",
	help = "Offset of the vector table inside the image."
)
args = parser.parse_args()

# Initialize two required sections
fw_verified = ""
fw_verification = ""

# TODO: populate verified section metadata here

# Verified section header is exactly 8 bytes long, firmware image header has the
# same length. Therefore, verified data must be fw_offset - 16 bytes long. Print
# error if it overflows or pad to required size. Length if padding header must
# also be considered (another 8 bytes).
if len(fw_verified) > (int(args.fw_offset, 16) - 24):
	print "Firmware metadata too big (doesn't fit in the specified firmware offset)";
	exit(1)

# Fill verified section to match required firmware offset.
fw_verified = pad_metadata(fw_verified, int(args.fw_offset, 16) - 16)

# And append input binary file
try:
	with open(args.binfile, "r") as f:
		fw_verified += build_section(section_magic.firmware, f.read())
except:
	print "Cannot read input file '%s'" % args.binfile


# Now the verified section is complete. Compute required hashes and other
# authentication data.
# TODO:
# Append some dummy data for now.
fw_verification += build_section(section_magic.dummy, "dummy data")

# This variable contains full firmware image with all required parts
# Verified and verification sections are always present. Verification
# section can be empty.
fw_image = build_section(section_magic.verified, fw_verified) + build_section(section_magic.verification, fw_verification)

# Print resulting firmware image structure
print_sections(fw_image, int(args.fw_base, 16))

# Save the image to selected file.
try:
	with open(args.fwfile, "w") as f:
		f.write(fw_image)
except:
	print "Cannot write output firmware file '%s'" % args.fwfile


