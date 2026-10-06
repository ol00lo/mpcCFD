import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
import math

# Fill boxes ========================================================
np.random.seed(0)
n_boxes = 4

# [N, 5] -- ibox x (x,y,hx,hy,angle)
boxes = np.concatenate(
    [
        np.random.uniform(-1, 1, (n_boxes, 2)),  # координаты центра: 0, 1
        np.random.uniform(0.10, 0.30, (n_boxes, 1)),  # полуширина по x (hx): 2
        np.random.uniform(0.03, 0.15, (n_boxes, 1)),  # полуширина по y (hy): 3
        np.random.uniform(0, np.pi, (n_boxes, 1)),  # угол поворота оси: 4
    ],
    axis=1,
)
print(boxes)

print("===================")


# Save pic ===============================================================
def obb_corners(center, half, angle):
    """half = (hx, hy)"""
    c, s = np.cos(angle), np.sin(angle)
    u = np.array([c, s])
    v = np.array([-s, c])
    hx, hy = half
    return np.array(
        [
            center + hx * u + hy * v,
            center - hx * u + hy * v,
            center - hx * u - hy * v,
            center + hx * u - hy * v,
        ]
    )


fig, ax = plt.subplots(figsize=(8, 8))
for i, (cx, cy, hx, hy, ang) in enumerate(boxes):
    corners = obb_corners(np.array([cx, cy]), (hx, hy), ang)
    ax.add_patch(Polygon(corners, closed=True, fill=False, edgecolor="C0"))
    ax.text(cx, cy, str(i), ha="center", va="center", fontsize=9, color="red")

ax.set_xlim(-1.3, 1.3)
ax.set_ylim(-1.3, 1.3)
ax.set_aspect("equal")
ax.set_title("Boxes")
plt.savefig("boxes.png")
plt.close()


# Loop algorithm ===================================================
class Box:
    def __init__(self, x, y, hx, hy, angle):
        self.x = x  # центр по X
        self.y = y  # центр по Y
        self.angle = angle  # поворот оси u относительно X против часовой (рад)
        self.hx = hx  # полуширина (вдоль оси u)
        self.hy = hy  # полувысота (вдоль оси v)


def is_intersected(a: Box, b: Box) -> bool:
    # --- ортонормированные оси бокса A ---
    ca, sa = math.cos(a.angle), math.sin(a.angle)
    uA = (ca, sa)  # ось U бокса A
    vA = (-sa, ca)  # ось V бокса A

    # --- ортонормированные оси бокса B ---
    cb, sb = math.cos(b.angle), math.sin(b.angle)
    uB = (cb, sb)
    vB = (-sb, cb)

    # --- вектор между центрами: d = B.center - A.center ---
    dx = b.x - a.x
    dy = b.y - a.y

    # --- проверяем 4 оси ---
    for nx, ny in (uA, vA, uB, vB):
        # проекция d на ось n
        proj_d = abs(dx * nx + dy * ny)

        # радиус проекции A на ось n
        rA = a.hx * abs(uA[0] * nx + uA[1] * ny) + a.hy * abs(vA[0] * nx + vA[1] * ny)

        # радиус проекции B на ось n
        rB = b.hx * abs(uB[0] * nx + uB[1] * ny) + b.hy * abs(vB[0] * nx + vB[1] * ny)

        # нашли разделяющую ось — пересечения нет
        if proj_d > rA + rB:
            return False

    # ни одна ось не разделила — пересекаются
    return True


overlap = np.eye(n_boxes, n_boxes, dtype=int)
for i in range(n_boxes):
    for j in range(i + 1, n_boxes):
        a = Box(*boxes[i])
        b = Box(*boxes[j])
        if is_intersected(a, b):
            overlap[i, j] = 1
            overlap[j, i] = 1
print("Loop algorithm")
print(overlap)
