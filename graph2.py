import matplotlib.pyplot as plt
import plotly.express as px
import numpy as np
import pandas as pd

import subprocess

def run_test(P, Q, QUEUESIZE, seconds_to_run=0.2, save_folder="data"):
    cmd = ["./run", str(P), str(Q), str(QUEUESIZE)]

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

P_Q_SIZES = [1, 20, 40, 60, 80, 100]
QUEUESIZE_SIZES = [1, 10, 100]

for P in P_Q_SIZES:
    for Q in P_Q_SIZES:
        for QUEUESIZE in QUEUESIZE_SIZES:
            run_test(P, Q, QUEUESIZE)

df = pd.DataFrame(columns=["P", "Q", "QUEUESIZE", "mean_time"])

for P in P_Q_SIZES:
    for Q in P_Q_SIZES:
        for QUEUESIZE in QUEUESIZE_SIZES:
            file_name = f"data/{P}_{Q}_{QUEUESIZE}.txt"
            with open(file_name, "r") as f:
                lines = f.readlines()
                data = [line.split()[2] for line in lines]
                # Convert the data to floats if needed
                times = [float(line) for line in data]
                mean_time = np.mean(times)

                df = df._append({
                    "P": P,
                    "Q": Q,
                    "QUEUESIZE": QUEUESIZE,
                    "mean_time": mean_time
                }, ignore_index=True)

df.to_csv("data.csv")


df = pd.read_csv("data.csv")

# 4D plot
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')

fig = px.scatter_3d(df, x="P", y="Q", z="QUEUESIZE", color="mean_time", size="mean_time",
                    hover_data=["mean_time"], labels={"mean_time": "Mean Time"},
                    title="4D Plot of Parameters with Hover Tooltips")
fig.update_layout(scene=dict(zaxis=dict(type='log', tickvals=[])), width=800, height=600)
fig.show()

# # Log scale 3D plot
# fig = plt.figure(figsize=(10, 8))
# ax = fig.add_subplot(111, projection='3d')

# ax.scatter(df["P"], df["Q"], df["QUEUESIZE"], c=df["mean_time"], s=df["mean_time"], cmap='viridis')

# ax.set_xlabel('P')
# ax.set_ylabel('Q')
# ax.set_zlabel('QUEUESIZE')
# ax.set_title('Log Scale 3D Plot of Parameters')

# ax.set_zscale('log')

# tooltips = mplcursors.cursor(scatter, hover=True)

# @tooltips.connect("add")
# def on_add(sel):
#     ind = sel.target.index
#     mean_time = df.loc[ind, "mean_time"]
#     x = df.loc[ind, "P"]
#     y = df.loc[ind, "Q"]
#     z = df.loc[ind, "QUEUESIZE"]
#     sel.annotation.set_text(f"Mean Time: {mean_time:.2f}\n(P, Q, QUEUESIZE): ({x}, {y}, {z})")


# plt.show()

# 3D PQ plot with mean_time as z
# df_100 = df[df["QUEUESIZE"] == 100.0]

# fig = plt.figure(figsize=(10, 8))
# ax = fig.add_subplot(111, projection='3d')

# ax.scatter(df_100["P"], df_100["Q"], df_100["mean_time"], c=df_100["mean_time"], s=100, cmap='viridis)
