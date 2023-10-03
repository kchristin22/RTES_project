# import matplotlib.pyplot as plt
# import plotly.express as px
import statistics
import csv
# import pandas as pd

import subprocess

interval_pattern = r"Time spent in the queue of Timer with period (\d+) us: (\d+) us"

def run_test(P, Q, QUEUESIZE, seconds_to_run=60, save_folder="data"):
    cmd = ["./prod-cons", "t=1", f"p={P}", f"q={Q}", f"n={QUEUESIZE}"]


    file_name = f"{save_folder}/{P}_{Q}_{QUEUESIZE}.txt"

    # after 4 seconds, kill the process
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        outs, errs = proc.communicate(timeout=seconds_to_run)
    except subprocess.TimeoutExpired:
        proc.kill()
        outs, errs = proc.communicate()
    with open(file_name, "w") as f:
        f.write(outs.decode("utf-8"))

    return file_name

P_SIZES = [1, 6, 10]
Q_SIZES = [2, 6, 10]
QUEUESIZE_SIZES = [1, 10, 100]


for P in P_SIZES:
    for Q in Q_SIZES:
        for QUEUESIZE in QUEUESIZE_SIZES:
            run_test(P, Q, QUEUESIZE)

data_dict = []

for P in P_SIZES:
    for Q in Q_SIZES:
        for QUEUESIZE in QUEUESIZE_SIZES:
            file_name = f"data/{P}_{Q}_{QUEUESIZE}.txt"
            with open(file_name, "r") as f:
                lines = f.readlines()
                data = [line.split()[2] for line in lines]
                # Convert the data to floats if needed
                times = [float(line) for line in data]
                mean_time = statistics.mean(times)
                data_dict.append({"P": P, "Q": Q, "QUEUESIZE": QUEUESIZE, "mean_time": mean_time})
              

# Specify the output file name
output_file = "output.csv"

# Open the CSV file for writing
with open(output_file, mode='w', newline='') as file:
    # Define the CSV column names based on the keys in the dictionaries
    fieldnames = ['P', 'Q', 'QUEUESIZE', 'mean_time']

    # Create a CSV writer
    writer = csv.DictWriter(file, fieldnames=fieldnames)

    # Write the header row
    writer.writeheader()

    # Write the data rows
    for row in data_dict:
        writer.writerow(row)

print(f"Data has been written to {output_file}")

#############################################################

# input_file = "output.csv"

# # Initialize an empty list to store the data
# data_dict = []

# # Open the CSV file for reading
# with open(input_file, mode='r') as file:
#     # Create a CSV reader
#     reader = csv.DictReader(file)

#     # Iterate through the rows in the CSV file
#     for row in reader:
#         # Convert the row to a dictionary and append it to the list
#         data_dict.append({key: int(value) if key != 'mean_time' else float(value) for key, value in row.items()})

# # Print the data in the original format
# for item in data_dict:
#     print(item)

# df = pd.DataFrame(data_dict)

# # 4D plot
# fig = plt.figure(figsize=(10, 8))
# ax = fig.add_subplot(111, projection='3d')

# fig = px.scatter_3d(df, x="P", y="Q", z="QUEUESIZE", color="mean_time", size="mean_time",
#                     hover_data=["mean_time"], labels={"mean_time": "Mean Time"},
#                     title="4D Plot of Parameters with Hover Tooltips")
# fig.update_layout(scene=dict(zaxis=dict(type='log', tickvals=[])), width=800, height=600)
# fig.show()
