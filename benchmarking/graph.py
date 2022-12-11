import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import os

PATH = "results"

df = pd.DataFrame()

for file in os.listdir(PATH):
	data = np.genfromtxt(f"{PATH}/{file}", delimiter=";")
	df[file.split(".")[0]] = data

print(df.head())

with pd.option_context('display.max_rows', None, 'display.max_columns', None):  # more options can be specified also
	# print(df.mean().sort_values())
	table = df.describe().round(3).T[["mean", "max", "min"]]
	print(table.to_latex())

mean = df.mean()

titles = [
	"Initial",
	"Master-Worker",
	"Master-Worker with Overlapping Communication"
]

ranges = [2 ** np.arange(6), 2 ** np.arange(1, 6), 2 ** np.arange(1, 6)]

par = ["parallel" + str(x) for x in ranges[0]]
mw = ["master-worker" + str(x) for x in ranges[1]]
mw_async = ["master-worker-async" + str(x) for x in ranges[2]]

for i, dframe in enumerate([par, mw, mw_async]):
	plt.title(f"Average execution time by number of nodes\n for '{titles[i]}' version")
	df = mean[dframe]
	plt.plot(df.index, df, ".k--")
	plt.xlabel("# Nodes")
	plt.ylabel("Time / s")
	plt.xticks(df.index, ranges[i])
	plt.savefig(titles[i].replace(" ", ""))
	plt.clf()
