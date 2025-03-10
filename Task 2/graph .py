import pandas as pd
import matplotlib.pyplot as plt
data =pd.read_csv("connections2.txt", delim_whitespace=True, names=["Source IP", "Dest IP", "Source Port", "Dest Port"])
plt.hist(data.index, bins = 50, alpha = 0.6, label="TCP Connections")
plt.axvline(x=20, color='g', linestyle='--', label="Attack Start")
plt.axvline(x=120, color='b', linestyle='--', label="Attack End")
plt.legend()
plt.xlabel("Time (seconds)")
plt.ylabel("Connections")
plt.title("SYN Flood Attack - Connection Start Times")
plt.show()
