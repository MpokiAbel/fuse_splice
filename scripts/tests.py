import subprocess
import time


def measure_cat_execution_time(file_path):
    start_time = time.time()
    try:
        # Run the cat command using subprocess
        subprocess.run(["cat", file_path], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")
        return None

    end_time = time.time()
    execution_time = end_time - start_time
    return execution_time

# /home/mpokiabel/Documents/fuse/fuse_fs/mnt/file_1.out

if __name__ == "__main__":
    # Replace 'file.txt' with the path to the file you want to read using the cat command
    file_path = "/home/mpokiabel/Documents/fuse/fuse_fs/mnt/home/mpokiabel/Documents/fuse/fuse_fs/tests_files/file_1024.0KB.txt"
    execution_time = measure_cat_execution_time(file_path)

    if execution_time is not None:
        print(f"Execution time: {execution_time:.5f} seconds")

