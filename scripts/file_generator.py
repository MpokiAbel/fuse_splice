import os


def generate_files():
    # Define the range of file sizes in bytes (4KB to 1MB)
    gb = 1024 * 1024 * 1024
    file_sizes = [gb, gb * 10]

    # Create a directory to store the files
    if not os.path.exists("generated_files"):
        os.makedirs("generated_files")

    # Generate files with increasing sizes
    for size in file_sizes:
        file_name = f"generated_files/file_{size/1024}KB.out"
        with open(file_name, "w") as file:
            # Generate some sample text content
            content = "This is a sample text content.\n" * (size // 30)
            file.write(content)

        print(f"Generated {size/1024}KB file: {file_name}")


if __name__ == "__main__":
    generate_files()
