import random
import string


# Generate random data for each block
def generate(num_blocks, metadata_size, payload_size, footer_size):
    data = []
    for i in range(num_blocks):
        metadata = "".join(
            random.choices(string.ascii_letters + string.digits, k=metadata_size)
        )
        payload = "".join(
            random.choices(string.ascii_letters + string.digits, k=payload_size)
        )
        footer = "".join(
            random.choices(string.ascii_letters + string.digits, k=footer_size)
        )
        block = metadata + "\n\n" + payload + "\n\n" + footer + "\n\n"
        data.append(block)

    # Join all the blocks into a single string
    big_data = "".join(data)

    return big_data


# Set the number of files to generate
num_of_files_list = int(input("number of files to generate: "))
# Set the number of blocks to generate

num_blocks_list = list()
metadata_size_list = list()
payload_size_list = list()
footer_size_list = list()

for i in range(0, num_of_files_list):
    num_blocks_list.append(int(input("number of blocks for file " + str(i + 1) + ": ")))
    metadata_size_list.append(
        int(input("Enter Metadata size for file " + str(i + 1) + ": "))
    )
    payload_size_list.append(
        int(input("Enter payloadsize for file " + str(i + 1) + ": "))
    )
    footer_size_list.append(
        int(input("Enter footer size for file " + str(i + 1) + ": "))
    )

    # Write the data to a file
    with open("file_" + str(i + 1) + ".out", "w") as file:
        file.write(
            generate(
                num_blocks_list[i],
                metadata_size_list[i],
                payload_size_list[i],
                footer_size_list[i],
            )
        )
