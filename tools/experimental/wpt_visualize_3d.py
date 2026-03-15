#!/usr/bin/env python3
"""
3D interactive visualization of jk_botti waypoints and BSP geometry.

Generates an HTML file with a canvas-based 3D renderer (no dependencies).
Mouse drag to rotate, scroll to zoom.

Usage:
    python3 wpt_visualize_3d.py <map.wpt> [map.matrix] [map.bsp] --wpt N [options]

Options:
    --wpt N             Center on waypoint N (required)
    --radius R          Radius around center (default: 300)
    --output FILE       Output HTML file (default: stdout)
"""

import struct
import gzip
import math
import sys
import json
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
    version, subversion, flags, num_wpts = struct.unpack_from('<iiii', hdr, 8)
    mapname = hdr[24:56].split(b'\x00')[0].decode('ascii', errors='replace')

    waypoints = []
    for i in range(num_wpts):
        data = f.read(32)
        wf, x, y, z, ifl = struct.unpack('<ifffi12x', data)
        waypoints.append({'idx': i, 'flags': wf, 'x': x, 'y': y, 'z': z, 'itemflags': ifl})

    f.close()
    return {'mapname': mapname, 'waypoints': waypoints}


def read_matrix(path, num_wpts):
    """Parse jk_botti .matrix file (gzipped)."""
    try:
        f = gzip.open(path, 'rb')
        f.read(1)
        f.seek(0)
    except gzip.BadGzipFile:
        f = open(path, 'rb')

    magic_a = f.read(16)
    assert magic_a == b'jkbotti_matrixA\x00'
    array_size = num_wpts * num_wpts
    f.read(array_size * 2)  # skip shortest_path
    magic_b = f.read(16)
    assert magic_b == b'jkbotti_matrixB\x00'
    ft_data = f.read(array_size * 2)
    from_to = struct.unpack(f'<{array_size}H', ft_data)
    f.close()
    return {'from_to': from_to, 'num_wpts': num_wpts}


def read_bsp_edges(path):
    """Read edges and faces from GoldSrc BSP v30 file."""
    with open(path, 'rb') as f:
        version = struct.unpack('<I', f.read(4))[0]
        assert version == 30

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

        # Edges (lump 12): unsigned short[2] = 4 bytes each
        f.seek(lumps[12][0])
        edge_data = f.read(lumps[12][1])
        num_edges = len(edge_data) // 4
        edges = []
        for i in range(num_edges):
            v0, v1 = struct.unpack_from('<HH', edge_data, i * 4)
            edges.append((v0, v1))

        # Surfedges (lump 13): int = 4 bytes each
        f.seek(lumps[13][0])
        surfedge_data = f.read(lumps[13][1])
        num_surfedges = len(surfedge_data) // 4
        surfedges = list(struct.unpack(f'<{num_surfedges}i', surfedge_data))

        # Faces (lump 7): 20 bytes each
        f.seek(lumps[7][0])
        face_data = f.read(lumps[7][1])
        num_faces = len(face_data) // 20
        faces = []
        for i in range(num_faces):
            planenum, side, firstedge, numedges, texinfo = struct.unpack_from('<hhihh', face_data, i * 20)
            faces.append({'firstedge': firstedge, 'numedges': numedges})

    return vertices, edges, surfedges, faces


def get_face_edges(faces, surfedges, edges):
    """Get unique edge vertex pairs from faces."""
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
            edge_pair = (min(v0, v1), max(v0, v1))
            edge_set.add(edge_pair)
    return edge_set


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


def generate_html(waypoints, matrix, bsp_data, center, radius, output,
                  highlight_wpt=None):
    """Generate interactive 3D HTML visualization."""
    cx, cy, cz = center
    r2 = radius * radius

    def in_range(x, y, z):
        return (x - cx)**2 + (y - cy)**2 + (z - cz)**2 <= r2 * 3  # slightly larger sphere

    # Filter waypoints
    vis_wpts = []
    for w in waypoints:
        if (w['x'] - cx)**2 + (w['y'] - cy)**2 <= r2:
            vis_wpts.append(w)

    # Build path connections
    path_lines = []
    if matrix:
        vis_idx = set(w['idx'] for w in vis_wpts)
        drawn = set()
        for w in vis_wpts:
            for w2 in vis_wpts:
                if w['idx'] >= w2['idx']:
                    continue
                nh = matrix['from_to'][w['idx'] * matrix['num_wpts'] + w2['idx']]
                if nh == w2['idx']:
                    path_lines.append([w['x'], w['y'], w['z'], w2['x'], w2['y'], w2['z']])

    # Filter BSP edges
    bsp_lines = []
    if bsp_data:
        vertices, edges, surfedges, faces = bsp_data
        face_edges = get_face_edges(faces, surfedges, edges)

        for v0i, v1i in face_edges:
            x0, y0, z0 = vertices[v0i]
            x1, y1, z1 = vertices[v1i]
            # Both vertices must be near the center
            if in_range(x0, y0, z0) and in_range(x1, y1, z1):
                bsp_lines.append([x0, y0, z0, x1, y1, z1])

    # Prepare waypoint data for JS
    wpt_js = []
    for w in vis_wpts:
        color = '#00cc44'
        if w['flags'] & (1 << 7): color = '#00ff00'  # health
        if w['flags'] & (1 << 9): color = '#ffaa00'  # ammo
        if w['flags'] & (1 << 10): color = '#ff00ff'  # weapon
        if w['flags'] & (1 << 3): color = '#00ffff'  # ladder

        highlight = (highlight_wpt is not None and w['idx'] == highlight_wpt)
        label = f"{w['idx']}"
        fstr = wpt_flag_str(w['flags'])
        if fstr:
            label += f" [{fstr}]"

        wpt_js.append({
            'x': w['x'], 'y': w['y'], 'z': w['z'],
            'color': color, 'highlight': highlight,
            'label': label, 'zlabel': f"z={w['z']:.0f}"
        })

    # Z range for BSP edge coloring
    all_z = [w['z'] for w in vis_wpts]
    for line in bsp_lines:
        all_z.extend([line[2], line[5]])
    z_min = min(all_z) if all_z else -800
    z_max = max(all_z) if all_z else -400

    html = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Waypoint 3D - {highlight_wpt}</title>
<style>
body {{ margin:0; background:#111; overflow:hidden; font-family:monospace; color:#ccc; }}
canvas {{ display:block; }}
#info {{ position:absolute; top:8px; left:8px; font-size:12px; pointer-events:none; }}
#help {{ position:absolute; bottom:8px; left:8px; font-size:11px; opacity:0.5; }}
</style>
</head>
<body>
<div id="info">Waypoint {highlight_wpt} area (r={radius})</div>
<div id="help">Drag: rotate | Scroll: zoom | Shift+drag: pan</div>
<canvas id="c"></canvas>
<script>
const W = {json.dumps(wpt_js)};
const P = {json.dumps(path_lines)};
const B = {json.dumps(bsp_lines)};
const CX={cx}, CY={cy}, CZ={cz};
const ZMIN={z_min}, ZMAX={z_max};

const canvas = document.getElementById('c');
const ctx = canvas.getContext('2d');

let rotX = -0.6, rotY = 0.4, zoom = 1.5, panX = 0, panY = 0;
let dragging = false, lastMX, lastMY, shiftKey = false;

function resize() {{
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    draw();
}}
window.addEventListener('resize', resize);

canvas.addEventListener('mousedown', e => {{
    dragging = true;
    lastMX = e.clientX; lastMY = e.clientY;
    shiftKey = e.shiftKey;
}});
window.addEventListener('mouseup', () => dragging = false);
window.addEventListener('mousemove', e => {{
    if (!dragging) return;
    const dx = e.clientX - lastMX, dy = e.clientY - lastMY;
    lastMX = e.clientX; lastMY = e.clientY;
    if (shiftKey || e.shiftKey) {{
        panX += dx; panY += dy;
    }} else {{
        rotY += dx * 0.005;
        rotX += dy * 0.005;
        rotX = Math.max(-Math.PI/2, Math.min(Math.PI/2, rotX));
    }}
    draw();
}});
canvas.addEventListener('wheel', e => {{
    zoom *= e.deltaY > 0 ? 0.9 : 1.1;
    zoom = Math.max(0.2, Math.min(10, zoom));
    draw();
    e.preventDefault();
}});

function zColor(z) {{
    let t = (z - ZMIN) / (ZMAX - ZMIN || 1);
    t = Math.max(0, Math.min(1, t));
    let r, g, b;
    if (t < 0.5) {{
        r = 0; g = Math.floor(255 * t * 2); b = Math.floor(255 * (1 - t * 2));
    }} else {{
        r = Math.floor(255 * (t - 0.5) * 2); g = Math.floor(255 * (1 - (t - 0.5) * 2)); b = 0;
    }}
    return `rgb(${{r}},${{g}},${{b}})`;
}}

function project(x, y, z) {{
    // Center on waypoint
    let dx = x - CX, dy = y - CY, dz = z - CZ;
    // Rotate Y axis
    let cosY = Math.cos(rotY), sinY = Math.sin(rotY);
    let tx = dx * cosY - dy * sinY;
    let ty = dx * sinY + dy * cosY;
    // Rotate X axis
    let cosX = Math.cos(rotX), sinX = Math.sin(rotX);
    let tz = dz * cosX - ty * sinX;
    ty = dz * sinX + ty * cosX;

    let scale = zoom * Math.min(canvas.width, canvas.height) / 600;
    let sx = canvas.width / 2 + tx * scale + panX;
    let sy = canvas.height / 2 - tz * scale + panY;
    return [sx, sy, ty]; // ty used as depth
}}

function draw() {{
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Collect all drawable items with depth for sorting
    let items = [];

    // BSP edges
    for (const l of B) {{
        let [sx0,sy0,d0] = project(l[0],l[1],l[2]);
        let [sx1,sy1,d1] = project(l[3],l[4],l[5]);
        let avgZ = (l[2] + l[5]) / 2;
        let depth = (d0 + d1) / 2;
        items.push({{type:'bsp', sx0,sy0,sx1,sy1, z:avgZ, depth}});
    }}

    // Path connections
    for (const l of P) {{
        let [sx0,sy0,d0] = project(l[0],l[1],l[2]);
        let [sx1,sy1,d1] = project(l[3],l[4],l[5]);
        let depth = (d0 + d1) / 2;
        items.push({{type:'path', sx0,sy0,sx1,sy1, depth}});
    }}

    // Waypoints
    for (const w of W) {{
        let [sx,sy,depth] = project(w.x, w.y, w.z);
        items.push({{type:'wpt', sx, sy, depth, w}});
    }}

    // Sort back-to-front
    items.sort((a,b) => b.depth - a.depth);

    for (const it of items) {{
        if (it.type === 'bsp') {{
            ctx.strokeStyle = zColor(it.z);
            ctx.globalAlpha = 0.35;
            ctx.lineWidth = 0.5;
            ctx.beginPath();
            ctx.moveTo(it.sx0, it.sy0);
            ctx.lineTo(it.sx1, it.sy1);
            ctx.stroke();
            ctx.globalAlpha = 1;
        }} else if (it.type === 'path') {{
            ctx.strokeStyle = '#888';
            ctx.globalAlpha = 0.5;
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(it.sx0, it.sy0);
            ctx.lineTo(it.sx1, it.sy1);
            ctx.stroke();
            ctx.globalAlpha = 1;
        }} else if (it.type === 'wpt') {{
            const w = it.w;
            const r = w.highlight ? 8 : 5;
            ctx.fillStyle = w.color;
            ctx.beginPath();
            ctx.arc(it.sx, it.sy, r, 0, Math.PI * 2);
            ctx.fill();
            if (w.highlight) {{
                ctx.strokeStyle = '#ff0';
                ctx.lineWidth = 3;
                ctx.stroke();
            }} else {{
                ctx.strokeStyle = '#fff';
                ctx.lineWidth = 1;
                ctx.stroke();
            }}
            // Label
            ctx.fillStyle = '#ddd';
            ctx.font = '10px monospace';
            ctx.textAlign = 'center';
            ctx.fillText(w.label, it.sx, it.sy - r - 4);
            ctx.fillStyle = '#999';
            ctx.font = '8px monospace';
            ctx.fillText(w.zlabel, it.sx, it.sy + r + 10);
        }}
    }}

    // Axes indicator
    const ax = 60, ay = canvas.height - 60;
    const alen = 30;
    for (const [lbl, dx, dy, dz, col] of [['X',alen,0,0,'#f44'],['Y',0,alen,0,'#4f4'],['Z',0,0,alen,'#44f']]) {{
        let [ex,ey] = project(CX+dx, CY+dy, CZ+dz);
        let [ox,oy] = project(CX, CY, CZ);
        let ddx = (ex-ox), ddy = (ey-oy);
        let len = Math.sqrt(ddx*ddx+ddy*ddy) || 1;
        ddx = ddx/len*alen; ddy = ddy/len*alen;
        ctx.strokeStyle = col;
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(ax, ay);
        ctx.lineTo(ax+ddx, ay+ddy);
        ctx.stroke();
        ctx.fillStyle = col;
        ctx.font = '10px monospace';
        ctx.fillText(lbl, ax+ddx*1.3, ay+ddy*1.3+4);
    }}
}}

resize();
</script>
</body>
</html>"""

    if output == '-':
        sys.stdout.write(html)
    else:
        with open(output, 'w') as f:
            f.write(html)
        print(f'Written: {output}', file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description='3D waypoint + BSP visualization')
    parser.add_argument('wpt_file', help='Path to .wpt file')
    parser.add_argument('matrix_file', nargs='?', help='Path to .matrix file')
    parser.add_argument('bsp_file', nargs='?', help='Path to .bsp file')
    parser.add_argument('--wpt', type=int, required=True, help='Center on waypoint N')
    parser.add_argument('--radius', type=float, default=300, help='Radius (default 300)')
    parser.add_argument('--output', '-o', default='-', help='Output HTML file')

    args = parser.parse_args()

    wpt_data = read_wpt(args.wpt_file)
    waypoints = wpt_data['waypoints']
    print(f"Loaded {len(waypoints)} waypoints from {wpt_data['mapname']}", file=sys.stderr)

    matrix = None
    if args.matrix_file:
        matrix = read_matrix(args.matrix_file, len(waypoints))
        print(f"Loaded path matrix", file=sys.stderr)

    bsp_data = None
    if args.bsp_file:
        bsp_data = read_bsp_edges(args.bsp_file)
        print(f"Loaded {len(bsp_data[0])} vertices, {len(bsp_data[1])} edges, {len(bsp_data[3])} faces", file=sys.stderr)

    w = waypoints[args.wpt]
    center = (w['x'], w['y'], w['z'])

    generate_html(waypoints, matrix, bsp_data, center, args.radius, args.output,
                  highlight_wpt=args.wpt)


if __name__ == '__main__':
    main()
