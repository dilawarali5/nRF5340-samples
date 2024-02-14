import serial
import time
import os
import sys
import binascii
import struct

#---------------------------------#
# Global Variables
#---------------------------------#
fileName = "net_core_app_update.bin"
# fileName = "app_update.bin"
file_size = 0
packet = 1
global ser

#---------------------------------#
# Functions
#---------------------------------#
def ReadFileSize():
    global file_size
    file_size = os.stat(fileName).st_size 
    print("file_size: ",file_size)
    return 0

def getOTAHeaderPacketBytes(packet_number_int, file_size_int):
    print("building OTA header packet.")
        #   <0x24> <4-bytes Packet number> <0x02> <x00, 0x00, 0x00, 0x04> <4-Bytes File Size> <0x23>
    header_bytes = bytes([0x24])
    packet_number = struct.pack('<I', packet_number_int)
    header_bytes = header_bytes + packet_number
    header_bytes = header_bytes + bytes([0x02,0x04, 0x00, 0x00, 0x00])
    file_size = struct.pack('<I', file_size_int)
    header_bytes = header_bytes + file_size
    header_bytes = header_bytes + bytes([0x23])

    return header_bytes

def getDataFileChunks(chunk_size):
    file_chunks_array = []
    try:
        with open(fileName, "rb") as f:
            byte = f.read(chunk_size)
            while byte:
                file_chunks_array.append(byte)
                byte = f.read(chunk_size)
            f.close()
        return file_chunks_array
                
    except IOError:
        print('Error While Opening the file!')
        return 0

def getOTABodyPacketBytes(packet_number_int, chunk):
    print("building OTA body packet")
    #           <0x24> <4-bytes Packet number> <0x03> <4-bytes data size> <data> <0x23>
    body_bytes = bytes([0x24])
    packet_number = struct.pack('<I', packet_number_int)
    body_bytes = body_bytes + packet_number
    body_bytes = body_bytes + bytes([0x03])
    data_size = struct.pack('<I', len(chunk))
    body_bytes = body_bytes + data_size
    body_bytes = body_bytes + chunk
    body_bytes = body_bytes + bytes([0x23])

    return body_bytes

def wait_for_ack():
    while ser.in_waiting == 0:
        pass
    received_data = ser.read(ser.in_waiting)
    print("Received data:", received_data)
    if received_data == b'\x06':
        print("ACK received ...")
        return 0
    else:
        print("ACK not received ...")
        return 1
#---------------------------------#
# Main Code
#---------------------------------#
ser = serial.Serial(port='COM13', baudrate=115200)
ReadFileSize()
print("file_size: ",file_size)
header_bytes = getOTAHeaderPacketBytes(packet, file_size)
print("Header: ", binascii.hexlify(header_bytes))

# send header
ser.write(header_bytes)
# wait for ACK
if wait_for_ack() == 1:
    print("Error: ACK not received ...")
    ser.close()
    sys.exit()

# Get current time
start_time = time.time()
print("Start time: ", start_time)

# send body
for chunk in getDataFileChunks(128):
    # print("chunk:",binascii.hexlify(chunk))
    # print("chunk size:",len(chunk))

    packet = packet + 1
    body_bytes = getOTABodyPacketBytes(packet, chunk)
    print("packet: ",packet)
    print("body: ",binascii.hexlify(body_bytes))
    print("body size: ",len(body_bytes))
    ser.write(body_bytes)

    # wait for ACK
    if wait_for_ack() == 1:
        print("Error: ACK not received ...")
        ser.close()
        sys.exit()
    # if packet == 10:
    #     break

# Get current time
end_time = time.time()

# Calculate time difference
time_diff = end_time - start_time
print("Time difference: ", time_diff)
ser.close()
print("Done ...")