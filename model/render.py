import sys, numpy as np, trimesh
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
m = trimesh.load(sys.argv[1])
fig = plt.figure(figsize=(12,6), dpi=130)
def draw(ax, elev, azim, mesh, title):
    tri = mesh.triangles
    n = mesh.face_normals
    light = np.array([0.4,-0.6,0.7]); light/=np.linalg.norm(light)
    sh = np.clip(n@light,0,1)*0.7+0.3
    cols = np.stack([sh*0.55, sh*0.8, sh*0.7, np.ones_like(sh)],1)
    pc = Poly3DCollection(tri, facecolors=cols, edgecolors='none')
    ax.add_collection3d(pc)
    b = mesh.bounds; c=(b[0]+b[1])/2; r=(b[1]-b[0]).max()/2
    ax.set_xlim(c[0]-r,c[0]+r); ax.set_ylim(c[1]-r,c[1]+r); ax.set_zlim(c[2]-r,c[2]+r)
    ax.view_init(elev, azim); ax.set_axis_off(); ax.set_title(title)
ax1 = fig.add_subplot(121, projection='3d'); draw(ax1, 18, -60, m, "Assembled (3/4 view)")
# cross-section: cut away y<0 half for an inside look
half = m.slice_plane([0,0,0],[0,1,0], cap=True)
ax2 = fig.add_subplot(122, projection='3d'); draw(ax2, 0, -90, half, "Cut at center (front half removed)")
plt.tight_layout(); plt.savefig(sys.argv[2])
