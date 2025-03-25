import numpy as np
import matplotlib.pyplot as plt
import matplotlib

cmap = matplotlib.colors.ListedColormap(["white", "red", "blue"], name="krgb", N=None)

cells = np.zeros((10, 10))

cells[5, 5] = 1
cells[4, 4] = 2
cells[4, 5] = 2
cells[3, 3] = 2
cells[6, 6] = 2
plt.pcolormesh(cells, cmap=cmap, edgecolors="k", linewidth=0.5)
ax = plt.gca()
plt.axis("off")
ax.set_aspect("equal")
plt.savefig("cells.pdf", dpi=600, bbox_inches="tight")
