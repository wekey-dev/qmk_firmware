#!/usr/bin/env python3
# Copyright 2026 WeKey
# SPDX-License-Identifier: GPL-2.0-or-later
"""Finalise a QMK app image for the nRF Core.

Reads the raw binary produced by `objcopy -O binary` (starts with
struct nrf_app_header), fills in image_size and crc32, then writes the final
.bin and an Intel HEX file located at the header's load address.

Header layout: see sdk/nrf_core_api.h. Prints nothing on
success (QMK's build treats any output as a warning).
"""
import argparse
import struct
import sys
import zlib

NRF_APP_MAGIC = 0x414B4D51
HEADER_SIZE = 116  # NRF_APP_HEADER_SIZE / NRF_APP_HEADER_OFF_* in nrf_core_api.h
OFF_HEADER_SIZE = 8
OFF_LOAD_ADDR = 16
OFF_IMAGE_SIZE = 20
OFF_CRC32 = 36


def fail(msg):
    print(f'nrf_app_pack: error: {msg}', file=sys.stderr)
    sys.exit(1)


def intel_hex(data, base):
    """Return Intel HEX text for `data` placed at address `base`."""
    def record(rtype, addr, payload):
        raw = bytes([len(payload), (addr >> 8) & 0xFF, addr & 0xFF, rtype]) + payload
        csum = (-sum(raw)) & 0xFF
        return ':' + (raw + bytes([csum])).hex().upper()

    lines = []
    upper = None
    for off in range(0, len(data), 16):
        addr = base + off
        if addr >> 16 != upper:
            upper = addr >> 16
            lines.append(record(0x04, 0, struct.pack('>H', upper)))
        lines.append(record(0x00, addr & 0xFFFF, data[off:off + 16]))
    lines.append(record(0x01, 0, b''))
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('raw', help='raw binary from objcopy -O binary')
    parser.add_argument('--bin', required=True, help='output .bin')
    parser.add_argument('--hex', required=True, help='output .hex')
    args = parser.parse_args()

    with open(args.raw, 'rb') as f:
        image = bytearray(f.read())

    if len(image) < HEADER_SIZE:
        fail('image smaller than the app header')

    magic = struct.unpack_from('<I', image, 0)[0]
    if magic != NRF_APP_MAGIC:
        fail(f'bad magic 0x{magic:08x}: .app_header is not at the start of the image')

    header_size = struct.unpack_from('<I', image, OFF_HEADER_SIZE)[0]
    if header_size != HEADER_SIZE:
        fail(f'header_size {header_size} != {HEADER_SIZE}: ABI header out of sync with this script')

    # objcopy may pad the end; pad to a multiple of 4 like the linker does.
    while len(image) % 4:
        image.append(0)

    load_addr = struct.unpack_from('<I', image, OFF_LOAD_ADDR)[0]
    struct.pack_into('<I', image, OFF_IMAGE_SIZE, len(image))
    crc = zlib.crc32(bytes(image[header_size:])) & 0xFFFFFFFF
    struct.pack_into('<I', image, OFF_CRC32, crc)

    with open(args.bin, 'wb') as f:
        f.write(image)
    with open(args.hex, 'w') as f:
        f.write(intel_hex(bytes(image), load_addr))


if __name__ == '__main__':
    main()
