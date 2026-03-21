#!/usr/bin/env python3
"""
Visualize jk_botti waypoints and BSP geometry around a specific area.

Generates an SVG with:
- BSP world vertices (floor/wall geometry) as dots
- Waypoints as labeled circles
- Path connections between waypoints
- Color-coded by Z height

Usage:
    python3 wpt_visualize.py <map.wpt> [map.matrix] [map.bsp] [options]

Options:
    --center X,Y,Z     Center point (default: auto from --wpt)
    --wpt N             Center on waypoint N
    --radius R          Radius around center to show (default: 500)
    --output FILE       Output SVG file (default: stdout)
    --z-range LOW,HIGH  Z range filter for BSP vertices (default: auto)
    --top               Top-down view XY (default)
    --side              Side view XZ
"""

import struct
import gzip
import math
import sys
import argparse


def read_wpt(path):
    """Parse jk_botti .wpt file (possibly gzipped)."""
    try:
        f = gzip.open(path, 'rb')
        f.read(1)
        f.seek(0)
    except gzip.BadGzipFile:
        f = open(path, 'rb')

    hdr = f.read(56)
    filetype = hdr[:8]
    version, subversion, flags, num_wpts = struct.unpack_from('<iiii', hdr, 8)
    mapname = hdr[24:56].split(b'\x00')[0].decode('ascii', errors='replace')

    waypoints = []
    for i in range(num_wpts):
        data = f.read(32)
        wf, x, y, z, ifl = struct.unpack('<ifffi12x', data)
        waypoints.append({'idx': i, 'flags': wf, 'x': x, 'y': y, 'z': z, 'itemflags': ifl})

    f.close()
    return {'mapname': mapname, 'version': version, 'waypoints': waypoints}


def read_matrix(path, num_wpts):
    """Parse jk_botti .matrix file (gzipped)."""
    try:
        f = gzip.open(path, 'rb')
        f.read(1)
        f.seek(0)
    except gzip.BadGzipFile:
        f = open(path, 'rb')

    magic_a = f.read(16)
    assert magic_a == b'jkbotti_matrixA\x00', f'Bad magic A: {magic_a}'

    array_size = num_wpts * num_wpts
    sp_data = f.read(array_size * 2)
    shortest_path = struct.unpack(f'<{array_size}H', sp_data)

    magic_b = f.read(16)
    assert magic_b == b'jkbotti_matrixB\x00', f'Bad magic B: {magic_b}'

    ft_data = f.read(array_size * 2)
    from_to = struct.unpack(f'<{array_size}H', ft_data)

    f.close()
    return {'shortest_path': shortest_path, 'from_to': from_to, 'num_wpts': num_wpts}


def read_bsp(path):
    """Read vertices and edges from GoldSrc BSP v30 file."""
    with open(path, 'rb') as f:
        version = struct.unpack('<I', f.read(4))[0]
        assert version == 30, f'Not GoldSrc BSP: version {version}'

        lumps = []
        for i in range(15):
            offset, length = struct.unpack('<II', f.read(8))
            lumps.append((offset, length))

        # Vertices (lump 3)
        f.seek(lumps[3][0])
        vert_data = f.read(lumps[3][1])
        num_verts = len(vert_data) // 12
        vertices = []
        for i in range(num_verts):
            x, y, z = struct.unpack_from('<3f', vert_data, i * 12)
            vertices.append((x, y, z))

        # Edges (lump 12)
        f.seek(lumps[12][0])
        edge_data = f.read(lumps[12][1])
        num_edges = len(edge_data) // 4
        edges = []
        for i in range(num_edges):
            v0, v1 = struct.unpack_from('<HH', edge_data, i * 4)
            edges.append((v0, v1))

        # Surfedges (lump 13)
        f.seek(lumps[13][0])
        surfedge_data = f.read(lumps[13][1])
        num_surfedges = len(surfedge_data) // 4
        surfedges = list(struct.unpack(f'<{num_surfedges}i', surfedge_data))

        # Faces (lump 7)
        f.seek(lumps[7][0])
        face_data = f.read(lumps[7][1])
        num_faces = len(face_data) // 20
        faces = []
        for i in range(num_faces):
            planenum, side, firstedge, numedges, texinfo = struct.unpack_from('<hhihh', face_data, i * 20)
            faces.append({'firstedge': firstedge, 'numedges': numedges})

    # Build unique edge set from faces
    edge_set = set()
    for face in faces:
        fe = face['firstedge']
        ne = face['numedges']
        for j in range(ne):
            se = surfedges[fe + j]
            if se >= 0:
                v0, v1 = edges[se]
            else:
                v1, v0 = edges[-se]
            edge_set.add((min(v0, v1), max(v0, v1)))

    # Convert to coordinate pairs
    bsp_edges = []
    for v0i, v1i in edge_set:
        bsp_edges.append((vertices[v0i], vertices[v1i]))

    return vertices, bsp_edges


# Waypoint flag names
W_FL_NAMES = {
    1 << 1: 'crouch', 1 << 2: 'jump', 1 << 3: 'ladder',
    1 << 5: 'door', 1 << 6: 'longjump', 1 << 7: 'health',
    1 << 8: 'armor', 1 << 9: 'ammo', 1 << 10: 'weapon',
    1 << 12: 'aiming', 1 << 13: 'spawnadd',
    1 << 14: 'lift_start', 1 << 15: 'lift_end',
}


def wpt_flag_str(flags):
    parts = []
    for bit, name in W_FL_NAMES.items():
        if flags & bit:
            parts.append(name)
    return ','.join(parts) if parts else ''


def z_to_color(z, z_min, z_max):
    """Map Z value to a color: blue(low) -> green(mid) -> red(high)."""
    if z_max == z_min:
        return '#888888'
    t = (z - z_min) / (z_max - z_min)
    t = max(0.0, min(1.0, t))

    if t < 0.5:
        # blue to green
        r = 0
        g = int(255 * (t * 2))
        b = int(255 * (1 - t * 2))
    else:
        # green to red
        r = int(255 * ((t - 0.5) * 2))
        g = int(255 * (1 - (t - 0.5) * 2))
        b = 0

    return f'#{r:02x}{g:02x}{b:02x}'


def generate_svg(waypoints, matrix, bsp_data, center, radius, view, output,
                 highlight_wpt=None):
    """Generate SVG visualization."""
    cx, cy, cz = center

    # Filter waypoints in range
    def in_range_2d(x, y):
        return (x - cx) ** 2 + (y - cy) ** 2 <= radius ** 2

    if view == 'top':
        get_xy = lambda x, y, z: (x, y)
        axis_labels = ('X', 'Y')
    else:  # side
        get_xy = lambda x, y, z: (y, z)
        axis_labels = ('Y', 'Z')

    # Collect visible waypoints
    vis_wpts = []
    for w in waypoints:
        if in_range_2d(w['x'], w['y']):
            vis_wpts.append(w)

    # Collect visible BSP edges
    vis_edges = []
    if bsp_data:
        bsp_verts, bsp_edges = bsp_data
        z_range = (cz - radius, cz + radius)
        for (x0, y0, z0), (x1, y1, z1) in bsp_edges:
            if (in_range_2d(x0, y0) and z_range[0] <= z0 <= z_range[1] and
                in_range_2d(x1, y1) and z_range[0] <= z1 <= z_range[1]):
                vis_edges.append(((x0, y0, z0), (x1, y1, z1)))

    # Determine Z range for coloring
    all_z = [w['z'] for w in vis_wpts]
    for (x0,y0,z0),(x1,y1,z1) in vis_edges:
        all_z.extend([z0, z1])
    z_min = min(all_z) if all_z else -100
    z_max = max(all_z) if all_z else 100
    if z_min == z_max:
        z_min -= 50
        z_max += 50

    # SVG coordinates: map world coords to SVG
    svg_size = 800
    margin = 60
    scale = (svg_size - 2 * margin) / (2 * radius)

    def world_to_svg(x, y, z):
        sx, sy = get_xy(x, y, z)
        svg_x = margin + (sx - (cx if view == 'top' else cy) + radius) * scale
        svg_y = margin + (-(sy - (cy if view == 'top' else cz)) + radius) * scale
        return svg_x, svg_y

    lines = []
    lines.append(f'<svg xmlns="http://www.w3.org/2000/svg" '
                 f'width="{svg_size}" height="{svg_size}" '
                 f'style="background:#1a1a2e">')

    # Title
    title = f'Waypoints around ({cx:.0f},{cy:.0f},{cz:.0f}) r={radius}'
    if highlight_wpt is not None:
        title += f' [wpt {highlight_wpt}]'
    lines.append(f'<text x="{svg_size//2}" y="20" text-anchor="middle" '
                 f'fill="#ccc" font-size="14" font-family="monospace">{title}</text>')

    # Z color legend
    for i in range(10):
        t = i / 9.0
        lz = z_min + t * (z_max - z_min)
        lx = svg_size - 45
        ly = margin + (1.0 - t) * (svg_size - 2 * margin)
        color = z_to_color(lz, z_min, z_max)
        lines.append(f'<rect x="{lx}" y="{ly-5}" width="12" height="10" fill="{color}"/>')
        if i % 3 == 0:
            lines.append(f'<text x="{lx+16}" y="{ly+4}" fill="#888" '
                         f'font-size="9" font-family="monospace">{lz:.0f}</text>')

    # BSP edges
    for (x0,y0,z0),(x1,y1,z1) in vis_edges:
        sx0, sy0 = world_to_svg(x0, y0, z0)
        sx1, sy1 = world_to_svg(x1, y1, z1)
        avg_z = (z0 + z1) / 2
        color = z_to_color(avg_z, z_min, z_max)
        lines.append(f'<line x1="{sx0:.1f}" y1="{sy0:.1f}" x2="{sx1:.1f}" y2="{sy1:.1f}" '
                     f'stroke="{color}" stroke-width="0.5" opacity="0.35"/>')

    # Path connections between visible waypoints
    vis_idx_set = set(w['idx'] for w in vis_wpts)
    if matrix:
        drawn_edges = set()
        for w in vis_wpts:
            for w2 in vis_wpts:
                if w['idx'] == w2['idx']:
                    continue
                # Check if w routes directly to w2 (next hop)
                nh = matrix['from_to'][w['idx'] * matrix['num_wpts'] + w2['idx']]
                if nh == w2['idx']:
                    edge = (min(w['idx'], w2['idx']), max(w['idx'], w2['idx']))
                    if edge not in drawn_edges:
                        drawn_edges.add(edge)
                        sx1, sy1 = world_to_svg(w['x'], w['y'], w['z'])
                        sx2, sy2 = world_to_svg(w2['x'], w2['y'], w2['z'])
                        lines.append(f'<line x1="{sx1:.1f}" y1="{sy1:.1f}" '
                                     f'x2="{sx2:.1f}" y2="{sy2:.1f}" '
                                     f'stroke="#555" stroke-width="1" opacity="0.6"/>')

    # Waypoints
    for w in vis_wpts:
        sx, sy = world_to_svg(w['x'], w['y'], w['z'])
        color = z_to_color(w['z'], z_min, z_max)
        r = 6
        stroke = '#fff'
        stroke_w = 1

        # Highlight the target waypoint
        if highlight_wpt is not None and w['idx'] == highlight_wpt:
            stroke = '#ff0'
            stroke_w = 3
            r = 9

        # Flag-based styling
        flags = w['flags']
        if flags & (1 << 3):  # ladder
            stroke = '#0ff'
            stroke_w = 2
        if flags & (1 << 7):  # health
            color = '#00ff00'
        if flags & (1 << 9):  # ammo
            color = '#ffaa00'
        if flags & (1 << 10):  # weapon
            color = '#ff00ff'

        lines.append(f'<circle cx="{sx:.1f}" cy="{sy:.1f}" r="{r}" '
                     f'fill="{color}" stroke="{stroke}" stroke-width="{stroke_w}" opacity="0.9"/>')

        # Label
        flag_str = wpt_flag_str(w['flags'])
        label = f"{w['idx']}"
        if flag_str:
            label += f" [{flag_str}]"
        lines.append(f'<text x="{sx:.1f}" y="{sy - r - 3:.1f}" text-anchor="middle" '
                     f'fill="#ddd" font-size="9" font-family="monospace">{label}</text>')
        # Z label
        lines.append(f'<text x="{sx:.1f}" y="{sy + r + 10:.1f}" text-anchor="middle" '
                     f'fill="#999" font-size="8" font-family="monospace">z={w["z"]:.0f}</text>')

    # Scale bar
    bar_world = 100  # 100 units
    bar_px = bar_world * scale
    bx = margin
    by = svg_size - 25
    lines.append(f'<line x1="{bx}" y1="{by}" x2="{bx + bar_px}" y2="{by}" '
                 f'stroke="#888" stroke-width="2"/>')
    lines.append(f'<text x="{bx + bar_px / 2}" y="{by - 5}" text-anchor="middle" '
                 f'fill="#888" font-size="10" font-family="monospace">{bar_world} units</text>')

    lines.append('</svg>')

    svg = '\n'.join(lines)

    if output == '-':
        sys.stdout.write(svg)
    else:
        with open(output, 'w') as f:
            f.write(svg)
        print(f'Written: {output}', file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description='Visualize jk_botti waypoints + BSP geometry')
    parser.add_argument('wpt_file', help='Path to .wpt file')
    parser.add_argument('matrix_file', nargs='?', help='Path to .matrix file')
    parser.add_argument('bsp_file', nargs='?', help='Path to .bsp file')
    parser.add_argument('--center', help='Center point X,Y,Z')
    parser.add_argument('--wpt', type=int, help='Center on waypoint N')
    parser.add_argument('--radius', type=float, default=500, help='Radius (default 500)')
    parser.add_argument('--output', '-o', default='-', help='Output SVG file (default stdout)')
    parser.add_argument('--side', action='store_true', help='Side view (XZ) instead of top-down')

    args = parser.parse_args()

    # Load waypoints
    wpt_data = read_wpt(args.wpt_file)
    waypoints = wpt_data['waypoints']
    print(f"Loaded {len(waypoints)} waypoints from {wpt_data['mapname']}", file=sys.stderr)

    # Load matrix
    matrix = None
    if args.matrix_file:
        matrix = read_matrix(args.matrix_file, len(waypoints))
        print(f"Loaded path matrix ({matrix['num_wpts']}x{matrix['num_wpts']})", file=sys.stderr)

    # Load BSP
    bsp_data = None
    if args.bsp_file:
        bsp_verts, bsp_edges = read_bsp(args.bsp_file)
        bsp_data = (bsp_verts, bsp_edges)
        print(f"Loaded {len(bsp_verts)} vertices, {len(bsp_edges)} edges", file=sys.stderr)

    # Determine center
    highlight = args.wpt
    if args.center:
        cx, cy, cz = [float(v) for v in args.center.split(',')]
    elif args.wpt is not None:
        w = waypoints[args.wpt]
        cx, cy, cz = w['x'], w['y'], w['z']
    else:
        parser.error('Specify --center X,Y,Z or --wpt N')

    view = 'side' if args.side else 'top'

    generate_svg(waypoints, matrix, bsp_data, (cx, cy, cz), args.radius,
                 view, args.output, highlight_wpt=highlight)


if __name__ == '__main__':
    main()
