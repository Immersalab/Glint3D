#!/usr/bin/env python3
"""
Simple script to create a basic ICO file from PNG data.
This creates a minimal ICO that Windows can use.
"""
import struct
import os

def create_basic_ico(png_path, ico_path):
    """Create a basic ICO file that references the PNG data"""
    try:
        # Read the PNG file
        with open(png_path, 'rb') as f:
            png_data = f.read()
        
        # ICO file format:
        # ICONDIR (6 bytes)
        # ICONDIRENTRY (16 bytes per image)
        # Image data
        
        ico_header = struct.pack('<HHH', 0, 1, 1)  # Reserved=0, Type=1(ICO), Count=1
        
        # ICONDIRENTRY - we'll embed the PNG directly
        width = height = 64  # Standard icon size
        ico_entry = struct.pack('<BBBBHHLL', 
            width,           # Width (0 = 256)
            height,          # Height (0 = 256)  
            0,               # Color count (0 = true color)
            0,               # Reserved
            1,               # Color planes
            32,              # Bits per pixel
            len(png_data),   # Size of image data
            22               # Offset to image data (6 + 16)
        )
        
        # Write ICO file
        with open(ico_path, 'wb') as f:
            f.write(ico_header)
            f.write(ico_entry)
            f.write(png_data)
            
        print(f"Created ICO file: {ico_path}")
        return True
        
    except Exception as e:
        print(f"Error creating ICO: {e}")
        return False

if __name__ == "__main__":
    png_path = "../engine/assets/img/Glint3DIcon.png"
    ico_path = "../engine/resources/glint3d.ico"
    
    if os.path.exists(png_path):
        create_basic_ico(png_path, ico_path)
    else:
        print(f"PNG file not found: {png_path}")