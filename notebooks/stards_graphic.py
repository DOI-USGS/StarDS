"""
Generate a professional graphic for STARDS library
"""
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Circle
import numpy as np

# Set up the figure with a modern color scheme
fig = plt.figure(figsize=(16, 10), facecolor='#0a0e27')
ax = fig.add_subplot(111)
ax.set_xlim(0, 16)
ax.set_ylim(0, 10)
ax.axis('off')

# Color palette - modern tech colors
primary_blue = '#00d4ff'
secondary_purple = '#8b5cf6'
accent_green = '#10b981'
dark_bg = '#1a1f3a'
light_text = '#e5e7eb'
mid_text = '#9ca3af'

# Title with modern styling
title_text = ax.text(8, 9.2, 'STARDS',
                     ha='center', va='top',
                     fontsize=72, fontweight='bold',
                     color=primary_blue,
                     fontfamily='sans-serif')

subtitle_text = ax.text(8, 8.5, 'High-Performance N-Dimensional Array Storage',
                        ha='center', va='top',
                        fontsize=20,
                        color=light_text,
                        fontfamily='sans-serif')

# Main description
desc = ax.text(8, 8.0, 'Fast, compressed, cloud-native array storage for scientific computing',
               ha='center', va='top',
               fontsize=14,
               color=mid_text,
               style='italic',
               fontfamily='sans-serif')

# Feature boxes - arranged in a grid
features = [
    {
        'title': '⚡ Lightning Fast',
        'desc': 'LZ4 compression\nParallel I/O\nMemory-efficient',
        'color': primary_blue,
        'pos': (1.5, 5.5)
    },
    {
        'title': '☁️ Cloud Native',
        'desc': 'S3 support\nHTTP streaming\nVirtual file systems',
        'color': secondary_purple,
        'pos': (5.5, 5.5)
    },
    {
        'title': '🎯 Smart Slicing',
        'desc': 'Partial array reads\nZero-copy views\nLazy loading',
        'color': accent_green,
        'pos': (9.5, 5.5)
    },
    {
        'title': '🔧 Multi-Language',
        'desc': 'C++ native\nPython bindings\nSWIG interface',
        'color': '#f59e0b',
        'pos': (13.5, 5.5)
    }
]

for feature in features:
    # Draw feature box with gradient effect
    box = FancyBboxPatch(
        (feature['pos'][0] - 1.5, feature['pos'][1] - 1.2),
        3.0, 2.4,
        boxstyle="round,pad=0.15",
        facecolor=dark_bg,
        edgecolor=feature['color'],
        linewidth=2.5,
        alpha=0.9
    )
    ax.add_patch(box)

    # Title
    ax.text(feature['pos'][0], feature['pos'][1] + 0.8,
            feature['title'],
            ha='center', va='center',
            fontsize=16, fontweight='bold',
            color=feature['color'],
            fontfamily='sans-serif')

    # Description
    ax.text(feature['pos'][0], feature['pos'][1] - 0.2,
            feature['desc'],
            ha='center', va='center',
            fontsize=11,
            color=light_text,
            fontfamily='sans-serif',
            linespacing=1.6)

# Data flow visualization
flow_y = 2.8

# Storage formats
storage_box = FancyBboxPatch(
    (0.5, flow_y - 0.5), 3.5, 1.2,
    boxstyle="round,pad=0.1",
    facecolor=dark_bg,
    edgecolor=primary_blue,
    linewidth=2,
    alpha=0.9
)
ax.add_patch(storage_box)

ax.text(2.25, flow_y + 0.3, '📦 Storage',
        ha='center', va='center',
        fontsize=14, fontweight='bold',
        color=primary_blue,
        fontfamily='sans-serif')

ax.text(2.25, flow_y - 0.15, 'Local • S3 • HTTP',
        ha='center', va='center',
        fontsize=10,
        color=light_text,
        fontfamily='sans-serif')

# Arrow
arrow1 = FancyArrowPatch(
    (4.2, flow_y + 0.1), (5.8, flow_y + 0.1),
    arrowstyle='->,head_width=0.4,head_length=0.4',
    color=primary_blue,
    linewidth=2.5,
    mutation_scale=20
)
ax.add_patch(arrow1)

# STARDS processing
stards_box = FancyBboxPatch(
    (5.8, flow_y - 0.5), 4.4, 1.2,
    boxstyle="round,pad=0.1",
    facecolor=dark_bg,
    edgecolor=secondary_purple,
    linewidth=2,
    alpha=0.9
)
ax.add_patch(stards_box)

ax.text(8.0, flow_y + 0.3, '🚀 STARDS Engine',
        ha='center', va='center',
        fontsize=14, fontweight='bold',
        color=secondary_purple,
        fontfamily='sans-serif')

ax.text(8.0, flow_y - 0.15, 'Compress • Index • Cache',
        ha='center', va='center',
        fontsize=10,
        color=light_text,
        fontfamily='sans-serif')

# Arrow
arrow2 = FancyArrowPatch(
    (10.4, flow_y + 0.1), (12.0, flow_y + 0.1),
    arrowstyle='->,head_width=0.4,head_length=0.4',
    color=secondary_purple,
    linewidth=2.5,
    mutation_scale=20
)
ax.add_patch(arrow2)

# Your application
app_box = FancyBboxPatch(
    (12.0, flow_y - 0.5), 3.5, 1.2,
    boxstyle="round,pad=0.1",
    facecolor=dark_bg,
    edgecolor=accent_green,
    linewidth=2,
    alpha=0.9
)
ax.add_patch(app_box)

ax.text(13.75, flow_y + 0.3, '💻 Your App',
        ha='center', va='center',
        fontsize=14, fontweight='bold',
        color=accent_green,
        fontfamily='sans-serif')

ax.text(13.75, flow_y - 0.15, 'NumPy Arrays',
        ha='center', va='center',
        fontsize=10,
        color=light_text,
        fontfamily='sans-serif')

# Stats box at bottom
stats_box = FancyBboxPatch(
    (1, 0.3), 14, 1.2,
    boxstyle="round,pad=0.15",
    facecolor=dark_bg,
    edgecolor=primary_blue,
    linewidth=2,
    alpha=0.7
)
ax.add_patch(stats_box)

stats = [
    ('🎯 Zero-Copy', '2.5'),
    ('⚡ Fast I/O', '5.5'),
    ('📊 Any Shape', '8.5'),
    ('🗜️ Compressed', '11.5'),
    ('🔐 Thread-Safe', '14.5')
]

for stat, x_pos in stats:
    if isinstance(x_pos, str):
        x_pos = float(x_pos)
    ax.text(x_pos, 0.9,
            stat,
            ha='center', va='center',
            fontsize=13, fontweight='bold',
            color=light_text,
            fontfamily='sans-serif')

# Version badge
version_box = FancyBboxPatch(
    (14.5, 9.0), 1.3, 0.5,
    boxstyle="round,pad=0.08",
    facecolor=accent_green,
    edgecolor='none',
    alpha=0.9
)
ax.add_patch(version_box)

ax.text(15.15, 9.25, 'v0.2.0',
        ha='center', va='center',
        fontsize=12, fontweight='bold',
        color='white',
        fontfamily='sans-serif')

plt.tight_layout()
plt.savefig('/Users/krodriguez/repos/stards/notebooks/stards_graphic.png',
            dpi=300,
            facecolor='#0a0e27',
            bbox_inches='tight',
            pad_inches=0.3)
print("✅ Graphic saved to: notebooks/stards_graphic.png")
plt.show()
