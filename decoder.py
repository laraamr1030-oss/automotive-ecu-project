def decode_signal(frame_bytes, start_bit, length, scale, offset):
    frame_int = 0
    for byte_idx, byte_val in enumerate(frame_bytes):
        frame_int |= (byte_val & 0xFF) << (byte_idx * 8)
    
    mask = (1 << length) - 1
    raw_value = (frame_int >> start_bit) & mask
    
    return float((raw_value * float(scale)) + float(offset))

if __name__ == "__main__":
    frame_bytes = [0xE8, 0x03, 0x00, 0x00, 0x96, 0x64, 0x01, 0x00]
    
    print("EngineRPM:  ", decode_signal(frame_bytes, 0, 16, 0.25, 0))
    print("CoolantTemp:", decode_signal(frame_bytes, 32, 8, 0.5, -40))
    print("ThrottlePos:", decode_signal(frame_bytes, 40, 8, 0.4, 0))
