import ctypes
import psutil
import os
import sys
from ctypes import wintypes

# Constants for access rights and memory allocation
PROCESS_ALL_ACCESS = 0x1F0FFF
MEM_COMMIT = 0x1000
MEM_RESERVE = 0x2000
PAGE_READWRITE = 0x04
LOAD_LIBRARY_A = b"LoadLibraryA"

# Function prototypes
kernel32 = ctypes.windll.kernel32
user32 = ctypes.windll.user32

# Function to scan processes and find matching ones by substring
def find_process_by_name(substring):
    matching_processes = []
    for proc in psutil.process_iter(['pid', 'name']):
        try:
            if substring.lower() in proc.info['name'].lower():
                matching_processes.append(proc.info)
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return matching_processes

# Function to inject the DLL
def inject_dll(pid, dll_path):
    # Open the target process with all access rights
    h_process = kernel32.OpenProcess(PROCESS_ALL_ACCESS, False, pid)
    if not h_process:
        print(f"Failed to open process {pid}.")
        return False

    # Allocate memory in the target process for the DLL path
    dll_len = len(dll_path) + 1
    remote_mem = kernel32.VirtualAllocEx(h_process, 0, dll_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
    if not remote_mem:
        print(f"Failed to allocate memory for DLL in process {pid}.")
        return False

    # Write the DLL path into the allocated memory
    written = ctypes.c_int(0)
    kernel32.WriteProcessMemory(h_process, remote_mem, dll_path.encode(), dll_len, ctypes.byref(written))

    # Get the address of LoadLibraryA function from kernel32.dll
    load_library_addr = kernel32.GetProcAddress(kernel32.GetModuleHandleW(b"kernel32.dll"), LOAD_LIBRARY_A)

    # Create a remote thread to call LoadLibraryA with the DLL path as argument
    thread_id = ctypes.c_ulong(0)
    kernel32.CreateRemoteThread(h_process, None, 0, load_library_addr, remote_mem, 0, ctypes.byref(thread_id))

    print(f"DLL injected into process {pid} with thread ID: {thread_id.value}")
    return True

# Main function to scan processes and inject DLL
def main(dll_path, process_name_substring):
    shouldbreak = False
    print("waiting for process")
    while True: 
        # Find processes containing the given substring in their name
        matching_processes = find_process_by_name(process_name_substring)

        for proc in matching_processes:
            pid = proc['pid']
            print(f"Injecting into process {proc['name']} (PID: {pid})")
            inject_dll(pid, dll_path)
            shouldbreak = True
        if not matching_processes:
            print(f"No processes found with name containing '{process_name_substring}'.")
            shouldbreak = False
        if shouldbreak:
            return
        

if __name__ == "__main__":
    # Path to the DLL to inject (make sure to update this path)
    # set dll_path to be current file location
    dll_path = os.path.dirname(os.path.abspath(__file__)) + "\\DLLHooks\\Release\\DLLHooks.dll"
    process_name_substring = "Lockdown"

    # Run the injection
    main(dll_path, process_name_substring)
