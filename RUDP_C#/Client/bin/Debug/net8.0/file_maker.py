import os

# Define the file name and size
file_name = "firmware.bin"
file_size = 10 * 1024# * 1024  # 10 MB in bytes

# Open the file in write-binary mode
with open(file_name, "wb") as f:
    # Write random bytes to the file until it reaches the desired size
    f.write(os.urandom(file_size))

print(f"{file_size} bytes written to {file_name}.")
