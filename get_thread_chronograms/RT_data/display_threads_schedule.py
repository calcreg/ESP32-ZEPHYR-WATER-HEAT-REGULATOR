import struct
import re
import matplotlib.pyplot as plt
import sys

target_threads = ['tracing_thread', 'shell_uart', 'main', 'idle','thread1_id','thread2_id','unknown']

#Retrieve file_path
if len(sys.argv) > 1 :
    file_path = sys.argv[1]
    print('Reading : ' + file_path +'\n')
else:
    file_path = '14_oct_thread1_2_dump_ram_4.txt'
    print('Loading : 14_oct_thread1_2_dump_ram_4.txt')


# 1. Load clean unified raw bytes (addresses & pipes stripped)
full_bytes = bytearray()
try:
    with open(file_path, 'r') as f:
        for line in f:
            if '|' in line and ':' in line:
                _, hex_and_ascii = line.split(':', 1)
                hex_part = hex_and_ascii.split('|')[0]
                tokens = re.findall(r'[0-9a-fA-F]{2}', hex_part)
                for token in tokens:
                    full_bytes.append(int(token, 16))
except FileNotFoundError:
    print(f"Error: Could not find '{file_path}' in the current directory.")
    exit(1)

print(f"Loaded {len(full_bytes)} clean bytes successfully.")

# 2. Extract events using the true Offset -8 uint32 timestamp
events = []
for name in target_threads:
    name_bytes = name.encode('latin-1')
    pos = 0
    while True:
        pos = full_bytes.find(name_bytes, pos)
        if pos == -1:
            break
        
        # Unpack the true 32-bit hardware timestamp at Offset -8
        timestamp = pos
        if pos >= 8:
            try:
                timestamp = struct.unpack_from('<I', full_bytes, pos - 8)[0]
            except struct.error:
                pass

        events.append((timestamp, name))
        pos += len(name_bytes)

# 3. Sort chronologically by the true hardware timestamps
events.sort(key=lambda x: x[0])
timestamps = [e[0] for e in events]
threads = [e[1] for e in events]

print(f"\nSuccessfully extracted {len(events)} timestamped events:")
#for ts, th in zip(timestamps, threads):
#    print(f"Timestamp: {ts} -> Thread: {th}")

# 4. Render the Real-Time Chronogram
if threads:
    plt.figure(figsize=(14, 5))
    base_time = timestamps[0]
    # Use actual relative clock ticks for the x-axis
    relative_ticks = [(t - base_time) for t in timestamps]
    
    plt.step(relative_ticks, threads, where='mid', marker='o', linewidth=2, color='darkcyan')
    plt.yticks(fontsize=11, fontweight='bold')
    plt.xlabel('Hardware Clock Ticks (Real-Time Progression)', fontsize=12)
    plt.ylabel('Active Thread', fontsize=12)
    plt.title('Zephyr Real-Time Thread Activation Chronogram (ESP32 RAM Buffer)', fontsize=14, fontweight='bold')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.show()
else:
    print("No events found to plot.")
