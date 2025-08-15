import numpy as np
import os
import glob
import sys

def process_file(input_file, output_file):
    data_list = []

    with open(input_file, 'r') as file:
        for line in file:
            if line.strip():
                values =  np.fromstring(line.strip().rstrip(';'), dtype=np.int32, sep=',')
                if len(values) >= 6:
                    data_list.append(values[:6])

    if not data_list:
        print(f"No valid data found in {input_file}")
        return False
    
    data = np.array(data_list)

    #print(f"Accel range: {data[:, 0:3].min()} to {data[:, 0:3].max()}")
    #print(f"Gyro range: {data[:, 3:6].min()} to {data[:, 3:6].max()}")

    data[:, 0:3] = data[:, 0:3] << 3

    #print(f"Aftter shifting, Accel range: {data[:, 0:3].min()} to {data[:, 0:3].max()}")

    num_samples = data.shape[0]
    timestamps = np.arange(0, num_samples * 5, 5, dtype=np.int32)
    data_timestamp = np.column_stack((timestamps, data))

    header = "timestamp,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z"

    np.savetxt(output_file, data_timestamp, delimiter=',', fmt='%d', header=header, comments='')

    return True

def process_all_files(dataset_folder):
    txt_files = glob.glob(os.path.join(dataset_folder, "**", "*.txt"), recursive=True)

    valid_files = [file for file in txt_files if os.path.basename(file).startswith(('F', 'D'))]

    if not valid_files:
        print("No valid .txt files found in the dataset folder")
        return

    total_files = len(valid_files)
    print(f"Found {total_files} .txt files in the dataset folder")

    processed_files_count = 0

    for txt_file in valid_files:
        filename = os.path.basename(txt_file)
        filename_without_ext = os.path.splitext(filename)[0]

        first_letter = filename_without_ext[0]
        if first_letter == 'F':
            output_dir = os.path.join("Processed_dataset", "Fall")
        elif first_letter == 'D':
            output_dir = os.path.join("Processed_dataset", "Daily")

        os.makedirs(output_dir, exist_ok=True)

        output_file = os.path.join(output_dir, filename_without_ext + '.csv')

        try:
            if process_file(txt_file, output_file):
                #print(f"Processed {txt_file} to {output_file}")
                processed_files_count += 1
                print(f"Processed count: {processed_files_count} of {total_files}")
        except Exception as e:
            print(f"Error processing {txt_file}: {e}")

    print(f"Processed {processed_files_count} files of {total_files} total files.")

if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("Usage: python preprocess_dataset_script.py <dataset_folder>")
        sys.exit(1)

    dataset_folder = sys.argv[1]

    if not os.path.exists(dataset_folder):
        print(f"Dataset folder '{dataset_folder}' does not exist.")
        sys.exit(1)

    process_all_files(dataset_folder)