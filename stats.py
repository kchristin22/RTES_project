import re
import statistics
import subprocess
import time
# import matplotlib.pyplot as plt
import sys

# Create dictionaries to store data for drift and interval
drift_data = {}
interval_data = {}

# Regular expressions to match the desired lines
drift_pattern = r"Drift for Timer with period (\d+) us: (\d+) us"
interval_pattern = r"Time spent in the queue of Timer with period (\d+) us: (\d+) us"

# Function to extract data from a line and update statistics
def process_line(line, args):
    drift_match = re.match(drift_pattern, line)
    interval_match = re.match(interval_pattern, line)
    if drift_match:
        period, sleep = drift_match.groups()
        drift_data.setdefault(period, {}).setdefault(tuple(args), []).append(int(sleep))
    elif interval_match:
        period, interval = interval_match.groups()
        interval_data.setdefault(period, {}).setdefault(tuple(args), []).append(int(interval))

# Function to run the external program with arguments and collect data for one hour
def run_and_collect_data(args, run_time=10):
    try:
        process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

        start_time = time.time()
        while time.time() - start_time < run_time:
            line = process.stdout.readline()
            if not line:
                break  # Reached the end of output
            process_line(line, args)

    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"An error occurred: {str(e)}")
    finally:
        process.terminate()

# Run the program with different arguments for one hour each
arguments_list = [
    # ["./prod-cons", "t=1", "p=6", "q=6", "n=100"],
    ["./prod-cons", "t=1", "p=1", "q=2", "n=1"]
]


for args in arguments_list:

    if (len(sys.argv) > 1):
        run_time = int(sys.argv[1])
    else:
        run_time = 10

    print(f"Running with arguments: {' '.join(args)} for {run_time} seconds...")
    run_and_collect_data(args, run_time)

    for period in drift_data.keys():
        drift_values = drift_data.get(period, {}).get(tuple(args), [])
        interval_values = interval_data.get(period, {}).get(tuple(args), [])

        if drift_values and interval_values:
            min_drift = min(drift_values)
            max_drift = max(drift_values)
            mean_drift = statistics.mean(drift_values)
            median_drift = statistics.median(drift_values)
            std_deviation_drift = statistics.stdev(drift_values)

            min_interval = min(interval_values)
            max_interval = max(interval_values)
            mean_interval = statistics.mean(interval_values)
            median_interval = statistics.median(interval_values)
            std_deviation_interval = statistics.stdev(interval_values)

            print(f"\nStatistics for Timer with period {period} us:")
            print(f"Drift - Min: {min_drift}, Max: {max_drift}, Mean: {mean_drift}, Median: {median_drift}, Std Dev: {std_deviation_drift}")
            print(f"Interval - Min: {min_interval}, Max: {max_interval}, Mean: {mean_interval}, Median: {median_interval}, Std Dev: {std_deviation_interval}")

        else:
            print(f"No data found for Timer with period {period} us")

# print(drift_data)    

with open("drift_data_1_2_1.txt", "w") as file:
    for key, sub_dict in drift_data.items():
        file.write(f"{key}:\n")
        for sub_key, sub_value in sub_dict.items():
            file.write(f"  {sub_key}: {sub_value}\n")

# Load the data back into a dictionary
# loaded_data = {}

# with open("drift_data.txt", "r") as file:
#     current_key = None
#     for line in file:
#         line = line.strip()
#         if line.endswith(":"):
#             current_key = line[:-1]  # Remove the colon
#             loaded_data[current_key] = {}
#         elif current_key:
#             parts = line.split(": ")
#             sub_key = parts[0].strip()
#             sub_value = eval(parts[1])  # Use eval to convert the list representation back to a list
#             loaded_data[current_key][sub_key] = sub_value
# # Print the loaded data
# print(loaded_data)


# for period in loaded_data.keys():
#     plt.figure() # Create a new figure for each timer ID
#     for args in arguments_list:
#         args_str =  str(tuple(args)) 
#         print(args_str)
#         plt.plot(loaded_data[period].get(args_str,[]), label=f"Arguments: {' '.join(args)}")   
    
#     # Customize the plot
#     plt.xlabel("No. of measurement (Time)")
#     plt.ylabel("Drift Values (us)")
#     plt.legend()
#     float_period = int(period) / 1000000
#     plt.title(f"Drift Values Over Time (Timer with period {float_period})")
#     filename = f"{float_period: .2f}_plot.png"
#     plt.savefig(filename)


# plt.show()

