import os
import json
import pathlib

# Get the path to the openvrpaths.vrpath file
user_name = os.getlogin()  # Get the username
file_path = f"C:\\Users\\{user_name}\\AppData\\Local\\openvr\\openvrpaths.vrpath"

# Check if the file exists
if not os.path.exists(file_path):
    print(f"File not found: {file_path}")
    exit(1)

# Read the contents of the file
with open(file_path, 'r') as file:
    data = json.load(file)

# Get the absolute path to the oculus_dk1_steamVR directory
oculus_dk1_path = pathlib.Path("oculus_dk1_steamVR").resolve()

# Check if the 'external_drivers' key exists and initialize it if necessary
if 'external_drivers' not in data or data['external_drivers'] is None:
    data['external_drivers'] = []

# Check if the path is already in the external_drivers array
if str(oculus_dk1_path) not in data['external_drivers']:
    data['external_drivers'].append(str(oculus_dk1_path))

# Write the changes back to the file
with open(file_path, 'w') as file:
    json.dump(data, file, indent=4)

print("File successfully updated.")