import re
import statistics
import subprocess

# Create dictionaries to store data for drift and interval
drift_data = {}
interval_data = {}

# Regular expressions to match the desired lines
drift_pattern = r"Drift for Timer (\d+): (\d+) us"
interval_pattern = r"Time spent in the queue of Timer (\d+): (\d+) us"

# Function to extract data from a line and update statistics
def process_line(line):
    drift_match = re.match(drift_pattern, line)
    interval_match = re.match(interval_pattern, line)
    if drift_match:
        timer_id, sleep = drift_match.groups()
        drift_data.setdefault(timer_id, []).append(int(sleep))
    elif interval_match:
        timer_id, interval = interval_match.groups()
        interval_data.setdefault(timer_id, []).append(int(interval))

# Run the external program './prod-cons' with arguments 't=1' and 't=3'
try:
    process = subprocess.Popen(["./prod-cons", "t=1"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    # process2 = subprocess.Popen(["./prod-cons", "t=3"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

    # Continuously read and process lines from the program's output
    while True:
        line = process.stdout.readline()
        if not line:
            break  # Reached the end of output
        process_line(line)

    # while True:
    #     line = process2.stdout.readline()
    #     if not line:
    #         break  # Reached the end of output
    #     process_line(line)

except KeyboardInterrupt:
    for timer_id in drift_data.keys():
        drift_values = drift_data.get(timer_id, [])
        interval_values = interval_data.get(timer_id, [])

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