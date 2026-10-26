import struct
import re

file_path = '14_oct_thread_dump_ram.txt'

# Load clean unified raw bytes
full_bytes = bytearray()
with open(file_path, 'r') as f:
    for line in f:
        if '|' in line and ':' in line:
            _, hex_and_ascii = line.split(':', 1)
            hex_part = hex_and_ascii.split('|')[0]
            tokens = re.findall(r'[0-9a-fA-F]{2}', hex_part)
            for token in tokens:
                full_bytes.append(int(token, 16))

target_threads = ['tracing_thread', 'shell_uart', 'main', 'idle']

print("--- Inspecting Preceding Bytes for Timestamps ---")
for name in target_threads:
    name_bytes = name.encode('latin-1')
    pos = 0
    while True:
        pos = full_bytes.find(name_bytes, pos)
        if pos == -1:
            break
        print(f"\nThread '{name}' found at byte offset {pos}:")
        # Check integers at various offsets backward
        for offset in [4, 8, 12, 16, 20, 24, 28, 32]:
            if pos >= offset:
                val_64 = struct.unpack_from('<Q', full_bytes, pos - offset)[0]
                val_32 = struct.unpack_from('<I', full_bytes, pos - offset)[0]
                print(f"  Offset -{offset:2d} | uint64: {val_64} (0x{val_64:x}) | uint32: {val_32} (0x{val_32:x})")
        pos += len(name_bytes)
