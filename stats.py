import re
import statistics
import subprocess
import time
import matplotlib.pyplot as plt
import sys

# Create dictionaries to store data for drift and interval
drift_data = {}
interval_data = {}

# Regular expressions to match the desired lines
drift_pattern = r"Drift for Timer (\d+): (\d+) us"
interval_pattern = r"Time spent in the queue of Timer (\d+): (\d+) us"

# Function to extract data from a line and update statistics
def process_line(line, args):
    drift_match = re.match(drift_pattern, line)
    interval_match = re.match(interval_pattern, line)
    if drift_match:
        timer_id, sleep = drift_match.groups()
        drift_data.setdefault(timer_id, {}).setdefault(tuple(args), []).append(int(sleep))
    elif interval_match:
        timer_id, interval = interval_match.groups()
        interval_data.setdefault(timer_id, {}).setdefault(tuple(args), []).append(int(interval))

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

# Run the program with different arguments for one hour each
arguments_list = [
    ["./prod-cons", "t=1", "t=2"],
    ["./prod-cons", "t=1", "t=2", "t=3"],
]


for args in arguments_list:

    if (len(sys.argv) > 1):
        run_time = int(sys.argv[1])
    else:
        run_time = 10

    print(f"Running with arguments: {' '.join(args)} for {run_time} seconds...")
    run_and_collect_data(args, run_time)

    for timer_id in drift_data.keys():
        drift_values = drift_data.get(timer_id, {}).get(tuple(args), [])
        interval_values = interval_data.get(timer_id, {}).get(tuple(args), [])

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

            print(f"\nStatistics for Timer {timer_id}:")
            print(f"Drift - Min: {min_drift}, Max: {max_drift}, Mean: {mean_drift}, Median: {median_drift}, Std Dev: {std_deviation_drift}")
            print(f"Interval - Min: {min_interval}, Max: {max_interval}, Mean: {mean_interval}, Median: {median_interval}, Std Dev: {std_deviation_interval}")

        else:
            print(f"No data found for Timer {timer_id}")
        


for timer_id in drift_data.keys():
    plt.figure() # Create a new figure for each timer ID
    for args in arguments_list:
        plt.plot(drift_data[timer_id].get(tuple(args),[]), label=f"Arguments: {' '.join(args)}")   
    
    # Customize the plot
    plt.xlabel("Time (seconds)")
    plt.ylabel("Drift Values (us)")
    plt.legend()
    plt.title(f"Drift Values Over Time (Timer {timer_id})")
    filename = f"{timer_id}_plot.png"
    plt.savefig(filename)


# plt.show()

