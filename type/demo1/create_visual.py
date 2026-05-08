import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle
import numpy as np

# Museum-quality figure setup
fig, ax = plt.subplots(1, 1, figsize=(20, 14), dpi=300)
ax.set_xlim(0, 100)
ax.set_ylim(0, 100)
ax.axis('off')
fig.patch.set_facecolor('#FCFDFE')

# Refined color palette - more sophisticated and harmonious
color_foundation = '#64748B'   # Slate gray for starter
color_elevated = '#2563EB'     # Vibrant blue for professional
color_accent = '#0F172A'       # Near-black for enterprise
color_bg_light = '#F8FAFC'     # Subtle background
color_text_primary = '#0F172A' # Primary text
color_text_secondary = '#64748B' # Secondary text
color_border = '#E2E8F0'       # Subtle borders

# Geometric precision with golden ratio influences
tier_width = 20
tier_spacing = 8
start_x = (100 - (3 * tier_width + 2 * tier_spacing)) / 2

# Heights following visual hierarchy
heights = [48, 58, 52]
base_y = 22

# Tier definitions
tiers = [
    {
        'x': start_x,
        'height': heights[0],
        'color': color_foundation,
        'bg': color_bg_light,
        'label': 'STARTER',
        'price': '29',
        'lift': 0,
        'border_weight': 1.2
    },
    {
        'x': start_x + tier_width + tier_spacing,
        'height': heights[1],
        'color': color_elevated,
        'bg': '#EFF6FF',
        'label': 'PROFESSIONAL',
        'price': '79',
        'lift': -2,
        'border_weight': 2.5
    },
    {
        'x': start_x + 2 * (tier_width + tier_spacing),
        'height': heights[2],
        'color': color_accent,
        'bg': color_bg_light,
        'label': 'ENTERPRISE',
        'price': '199',
        'lift': 0,
        'border_weight': 1.2
    }
]

# Draw tiers with refined shadows and depth
for i, tier in enumerate(tiers):
    # Subtle shadow for depth perception
    if i == 1:  # Emphasized shadow for recommended tier
        shadow = FancyBboxPatch(
            (tier['x'] + 0.3, base_y + tier['lift'] - 0.3),
            tier_width, tier['height'],
            boxstyle="round,pad=0.4",
            facecolor='#0000000A',
            edgecolor='none',
            zorder=1
        )
        ax.add_patch(shadow)

    # Background fill
    bg_rect = FancyBboxPatch(
        (tier['x'], base_y + tier['lift']),
        tier_width, tier['height'],
        boxstyle="round,pad=0.4",
        facecolor=tier['bg'],
        edgecolor='none',
        zorder=2
    )
    ax.add_patch(bg_rect)

    # Border with precise weight
    border = FancyBboxPatch(
        (tier['x'], base_y + tier['lift']),
        tier_width, tier['height'],
        boxstyle="round,pad=0.4",
        facecolor='none',
        edgecolor=tier['color'],
        linewidth=tier['border_weight'],
        zorder=3
    )
    ax.add_patch(border)

    # Outer glow for recommended tier
    if i == 1:
        glow = FancyBboxPatch(
            (tier['x'] - 0.4, base_y + tier['lift'] - 0.4),
            tier_width + 0.8, tier['height'] + 0.8,
            boxstyle="round,pad=0.4",
            facecolor='none',
            edgecolor=color_elevated,
            linewidth=0.8,
            alpha=0.25,
            zorder=1
        )
        ax.add_patch(glow)

    # Price - sculptural numeral with refined typography
    price_y = base_y + tier['lift'] + tier['height'] * 0.48

    # Currency symbol - small and elevated
    ax.text(
        tier['x'] + tier_width / 2 - 3.5, price_y + 4,
        '$',
        fontsize=16,
        fontweight='300',
        ha='right', va='bottom',
        color=tier['color'],
        alpha=0.7,
        zorder=4,
        family='sans-serif'
    )

    # Main price
    ax.text(
        tier['x'] + tier_width / 2, price_y,
        tier['price'],
        fontsize=52 if i == 1 else 46,
        fontweight='200',
        ha='center', va='center',
        color=tier['color'],
        zorder=4,
        family='sans-serif'
    )

    # Label above - refined spacing
    ax.text(
        tier['x'] + tier_width / 2,
        base_y + tier['lift'] + tier['height'] + 3.2,
        tier['label'],
        fontsize=6.5,
        fontweight='600',
        ha='center', va='bottom',
        color=color_text_primary,
        zorder=4,
        family='sans-serif'
    )

    # Period descriptor
    ax.text(
        tier['x'] + tier_width / 2, price_y - 10,
        '/month',
        fontsize=7,
        fontweight='300',
        ha='center', va='top',
        color=color_text_secondary,
        zorder=4,
        family='sans-serif'
    )

# Refined title
ax.text(
    50, 92,
    'VALUE ARCHITECTURE',
    fontsize=8,
    fontweight='400',
    ha='center', va='center',
    color=color_text_secondary,
    alpha=0.6,
    zorder=4,
    family='sans-serif'
)

# Recommended badge - more refined
badge_x = tiers[1]['x'] + tier_width / 2
badge_y = base_y + tiers[1]['lift'] + tiers[1]['height'] + 7.5
badge_width = 14
badge_height = 2.8

badge_rect = FancyBboxPatch(
    (badge_x - badge_width/2, badge_y),
    badge_width, badge_height,
    boxstyle="round,pad=0.2",
    facecolor=color_elevated,
    edgecolor='none',
    zorder=5
)
ax.add_patch(badge_rect)

ax.text(
    badge_x, badge_y + badge_height/2,
    'MOST POPULAR',
    fontsize=5.5,
    fontweight='600',
    ha='center', va='center',
    color='white',
    zorder=6,
    family='sans-serif'
)

# Geometric feature indicators - more refined
feature_y_base = base_y - 2
for i, tier in enumerate(tiers):
    num_dots = [3, 5, 5][i]
    dot_size = 0.5
    dot_spacing = 1.5

    total_height = (num_dots - 1) * dot_spacing
    start_y = feature_y_base - total_height / 2

    for j in range(num_dots):
        dot_y = start_y + j * dot_spacing

        # Outer ring for emphasis
        if i == 1:
            outer = plt.Circle(
                (tier['x'] + tier_width / 2, dot_y),
                dot_size * 0.7,
                facecolor='none',
                edgecolor=tier['color'],
                linewidth=0.8,
                alpha=0.3,
                zorder=2
            )
            ax.add_patch(outer)

        # Main dot
        dot = plt.Circle(
            (tier['x'] + tier_width / 2, dot_y),
            dot_size * 0.4,
            facecolor=tier['color'],
            edgecolor='none',
            alpha=0.85,
            zorder=3
        )
        ax.add_patch(dot)

# Subtle baseline for visual grounding
baseline_y = 12
baseline_length = 65
baseline_x = (100 - baseline_length) / 2

ax.plot(
    [baseline_x, baseline_x + baseline_length],
    [baseline_y, baseline_y],
    color=color_border,
    linewidth=0.8,
    alpha=0.5,
    zorder=1
)

# Add subtle corner marks for framing (museum-quality detail)
corner_size = 1.5
corner_offset = 5
corners = [
    (corner_offset, 100 - corner_offset),  # Top left
    (100 - corner_offset, 100 - corner_offset),  # Top right
]

for cx, cy in corners:
    # Horizontal mark
    ax.plot([cx - corner_size, cx + corner_size], [cy, cy],
            color=color_border, linewidth=0.6, alpha=0.4, zorder=1)
    # Vertical mark
    ax.plot([cx, cx], [cy - corner_size, cy + corner_size],
            color=color_border, linewidth=0.6, alpha=0.4, zorder=1)

plt.tight_layout(pad=0)
plt.savefig('pricing-visual-refined.png', dpi=300, bbox_inches='tight',
            facecolor='#FCFDFE', edgecolor='none')
print("Refined visual created: pricing-visual-refined.png")
plt.close()
