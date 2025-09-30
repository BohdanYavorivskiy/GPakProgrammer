import tkinter as tk
from tkinter import filedialog, messagebox
import binascii
import os


def intel_hex_to_bin(source_file, destination_file):
    try:
        with open(source_file, 'r') as src:
            lines = src.readlines()
        
        binary_data = bytearray()
        max_bit_index = 0
        
        for line in lines:
            # Remove comments and strip whitespace
            if '//' in line:
                line = line[:line.index('//')]
            line = line.strip()
            
            # Skip empty lines
            if not line:
                continue
            
            # Split by tabs and filter out empty parts
            parts = [part.strip() for part in line.split('\t') if part.strip()]
            if len(parts) < 2:
                continue
            
            try:
                # Parse bit index and bit value
                bit_index = int(parts[0])
                bit_value = int(parts[1])
                
                # Validate bit value (should be 0 or 1)
                if bit_value not in [0, 1]:
                    continue
                
                # Calculate byte index and bit position within the byte
                byte_index = bit_index // 8
                bit_position = bit_index % 8
                
                # Extend binary_data if necessary
                if len(binary_data) <= byte_index:
                    binary_data.extend(b'\x00' * (byte_index + 1 - len(binary_data)))
                
                # Set the bit value
                if bit_value == 1:
                    binary_data[byte_index] |= (1 << bit_position)
                else:
                    binary_data[byte_index] &= ~(1 << bit_position)
                
                max_bit_index = max(max_bit_index, bit_index)
                
            except (ValueError, IndexError):
                # Skip lines that don't match the expected format
                continue
        
        # Calculate CRC as sum of all bytes with uint8 overflow
        crc = sum(binary_data) & 0xFF
        
        # Add CRC byte to the end
        binary_data.append(crc)
        
        # Display binary data in the text view
        display_binary_data(binary_data, crc)
        
        with open(destination_file, 'wb') as dst:
            dst.write(binary_data)
        
        messagebox.showinfo("Success", f"Binary file saved as:\n{destination_file}\nProcessed {max_bit_index + 1} bits")
    except Exception as e:
        messagebox.showerror("Error", str(e))


def browse_source():
    file_path = filedialog.askopenfilename(filetypes=[("Bit format files", "*.txt"), ("All files", "*.*")])
    if file_path:
        entry_source.delete(0, tk.END)
        entry_source.insert(0, file_path)


def browse_destination():
    file_path = filedialog.asksaveasfilename(
        defaultextension=".bin", 
        filetypes=[("Binary files", "*.bin")]
    )
    if file_path:
        entry_dest.delete(0, tk.END)
        entry_dest.insert(0, file_path)


def display_binary_data(binary_data, crc):
    """Display binary data in the text view"""
    # Clear previous data
    text_view.delete(1.0, tk.END)
    
    # Create hex dump
    hex_dump = f"Binary data ({len(binary_data)} bytes):\n"
    for i, byte in enumerate(binary_data):
        if i % 16 == 0:  # New line every 16 bytes
            hex_dump += f"{i:04X}: "
        hex_dump += f"{byte:02X} "
        if i % 16 == 15:  # End of line after 16 bytes
            hex_dump += "\n"
    if len(binary_data) % 16 != 0:  # Final newline if needed
        hex_dump += "\n"
    
    hex_dump += f"\nCRC byte: {crc:02X}\n"
    hex_dump += f"Total bytes with CRC: {len(binary_data)}"
    
    # Insert into text view
    text_view.insert(1.0, hex_dump)


def convert():
    source = entry_source.get()
    dest = entry_dest.get()
    
    if not source or not dest:
        messagebox.showwarning("Warning", "Please select both source and destination files.")
        return
    
    intel_hex_to_bin(source, dest)


# GUI Setup
root = tk.Tk()
root.title("Bit Format to Binary Converter")
root.geometry("650x600")

# File selection frame
file_frame = tk.Frame(root)
file_frame.grid(row=0, column=0, columnspan=3, padx=10, pady=10, sticky="ew")

tk.Label(file_frame, text="Source Bit File:").grid(row=0, column=0, padx=5, pady=5, sticky="e")
entry_source = tk.Entry(file_frame, width=40)
entry_source.grid(row=0, column=1, padx=5, pady=5)
tk.Button(file_frame, text="Browse", command=browse_source).grid(row=0, column=2, padx=5, pady=5)

tk.Label(file_frame, text="Destination BIN File:").grid(row=1, column=0, padx=5, pady=5, sticky="e")
entry_dest = tk.Entry(file_frame, width=40)
entry_dest.grid(row=1, column=1, padx=5, pady=5)
tk.Button(file_frame, text="Browse", command=browse_destination).grid(row=1, column=2, padx=5, pady=5)

tk.Button(file_frame, text="Convert", command=convert, width=15).grid(row=2, column=1, pady=10)

# Binary data display frame
display_frame = tk.Frame(root)
display_frame.grid(row=1, column=0, columnspan=3, padx=10, pady=10, sticky="nsew")

tk.Label(display_frame, text="Binary Data Output:", font=("Arial", 10, "bold")).grid(row=0, column=0, sticky="w", pady=(0, 5))

# Text view with scrollbar
text_frame = tk.Frame(display_frame)
text_frame.grid(row=1, column=0, sticky="nsew")

text_view = tk.Text(text_frame, height=20, width=80, font=("Courier", 9), wrap=tk.NONE)
scrollbar_v = tk.Scrollbar(text_frame, orient=tk.VERTICAL, command=text_view.yview)
scrollbar_h = tk.Scrollbar(text_frame, orient=tk.HORIZONTAL, command=text_view.xview)

text_view.configure(yscrollcommand=scrollbar_v.set, xscrollcommand=scrollbar_h.set)

text_view.grid(row=0, column=0, sticky="nsew")
scrollbar_v.grid(row=0, column=1, sticky="ns")
scrollbar_h.grid(row=1, column=0, sticky="ew")

# Configure grid weights for resizing
root.grid_rowconfigure(1, weight=1)
root.grid_columnconfigure(0, weight=1)
display_frame.grid_rowconfigure(1, weight=1)
display_frame.grid_columnconfigure(0, weight=1)
text_frame.grid_rowconfigure(0, weight=1)
text_frame.grid_columnconfigure(0, weight=1)

root.mainloop()



