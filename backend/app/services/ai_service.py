import json
import re
import math
from typing import Dict, Any, List, Optional

def parse_point(pt_data) -> Optional[Dict[str, float]]:
    if isinstance(pt_data, dict) and "x" in pt_data and "y" in pt_data:
        return {"x": float(pt_data["x"]), "y": float(pt_data["y"])}
    if isinstance(pt_data, (list, tuple)) and len(pt_data) >= 2:
        return {"x": float(pt_data[0]), "y": float(pt_data[1])}
    return None

def points_equal(p1: Dict[str, float], p2: Dict[str, float], tol: float = 1e-3) -> bool:
    return math.hypot(p1["x"] - p2["x"], p1["y"] - p2["y"]) <= tol

def generate_room(width: float, height: float, wall_thickness: float = 200.0, origin: Optional[Dict[str, float]] = None) -> Dict[str, Any]:
    orig = origin or {"x": 0.0, "y": 0.0}
    ops = [
        {"type": "add_layer", "layer_name": "A-WALL", "layer_color": "#F0F0F0"},
        {"type": "add_layer", "layer_name": "A-ANNO-TEXT", "layer_color": "#FFD700"},
        {"type": "add_layer", "layer_name": "A-ANNO-DIMS", "layer_color": "#00B4FF"},
        # Outer walls
        {
            "type": "create_rectangle",
            "top_left": orig,
            "width": width,
            "height": height,
            "layer": "A-WALL"
        }
    ]

    # Inner walls
    if wall_thickness > 0 and width > 2 * wall_thickness and height > 2 * wall_thickness:
        ops.append({
            "type": "create_rectangle",
            "top_left": {"x": orig["x"] + wall_thickness, "y": orig["y"] + wall_thickness},
            "width": width - 2 * wall_thickness,
            "height": height - 2 * wall_thickness,
            "layer": "A-WALL"
        })

    # Room text label
    ops.append({
        "type": "create_text",
        "position": {"x": orig["x"] + width / 2.0 - 400.0, "y": orig["y"] + height / 2.0},
        "text": f"ROOM {width/1000:.1f}m x {height/1000:.1f}m\nAREA: {(width*height)/1e6:.1f} m²",
        "height": 150.0,
        "layer": "A-ANNO-TEXT"
    })

    # Dimensions
    ops.append({
        "type": "create_dimension",
        "p1": orig,
        "p2": {"x": orig["x"] + width, "y": orig["y"]},
        "dim_point": {"x": orig["x"] + width / 2.0, "y": orig["y"] - 300.0},
        "layer": "A-ANNO-DIMS"
    })
    ops.append({
        "type": "create_dimension",
        "p1": orig,
        "p2": {"x": orig["x"], "y": orig["y"] + height},
        "dim_point": {"x": orig["x"] - 300.0, "y": orig["y"] + height / 2.0},
        "layer": "A-ANNO-DIMS"
    })

    return {
        "version": "1.0",
        "description": f"Architectural Room ({width/1000:.1f}m x {height/1000:.1f}m)",
        "prompt": f"Create a {width/1000:.1f}m x {height/1000:.1f}m room",
        "operations": ops
    }

def generate_door(width: float = 900.0, position: Optional[Dict[str, float]] = None) -> Dict[str, Any]:
    pos = position or {"x": 1000.0, "y": 0.0}
    ops = [
        {"type": "add_layer", "layer_name": "A-DOOR", "layer_color": "#FFB300"},
        {
            "type": "create_line",
            "start": pos,
            "end": {"x": pos["x"], "y": pos["y"] + width},
            "layer": "A-DOOR"
        },
        {
            "type": "create_arc",
            "center": pos,
            "radius": width,
            "start_angle": 0.0,
            "sweep_angle": 90.0,
            "layer": "A-DOOR"
        }
    ]
    return {
        "version": "1.0",
        "description": f"Architectural Door ({width:.0f} mm swing)",
        "prompt": f"Add a {width:.0f}mm door",
        "operations": ops
    }

def generate_stairs(step_count: int = 12, tread_depth: float = 280.0, stair_width: float = 1000.0) -> Dict[str, Any]:
    total_len = step_count * tread_depth
    ops = [
        {"type": "add_layer", "layer_name": "A-FLOR", "layer_color": "#C8C8C8"},
        {"type": "create_line", "start": {"x": 0.0, "y": 0.0}, "end": {"x": 0.0, "y": total_len}, "layer": "A-FLOR"},
        {"type": "create_line", "start": {"x": stair_width, "y": 0.0}, "end": {"x": stair_width, "y": total_len}, "layer": "A-FLOR"}
    ]
    for i in range(step_count + 1):
        y = i * tread_depth
        ops.append({
            "type": "create_line",
            "start": {"x": 0.0, "y": y},
            "end": {"x": stair_width, "y": y},
            "layer": "A-FLOR"
        })
    ops.append({
        "type": "create_line",
        "start": {"x": stair_width / 2.0, "y": 100.0},
        "end": {"x": stair_width / 2.0, "y": total_len - 200.0},
        "layer": "A-FLOR"
    })
    ops.append({
        "type": "create_text",
        "position": {"x": stair_width / 2.0 - 40.0, "y": total_len - 100.0},
        "text": "UP",
        "height": 80.0,
        "layer": "A-FLOR"
    })
    return {
        "version": "1.0",
        "description": f"Staircase Flight ({step_count} steps, {stair_width:.0f} mm width)",
        "prompt": f"Create staircase with {step_count} steps",
        "operations": ops
    }

def generate_bolt_circle(hole_count: int = 6, pitch_radius: float = 100.0, hole_radius: float = 8.0) -> Dict[str, Any]:
    ops = [
        {"type": "add_layer", "layer_name": "Mechanical", "layer_color": "#00C864"},
        {"type": "create_circle", "center": {"x": 0.0, "y": 0.0}, "radius": pitch_radius, "layer": "Mechanical"},
        {"type": "create_circle", "center": {"x": 0.0, "y": 0.0}, "radius": pitch_radius * 0.4, "layer": "Mechanical"}
    ]
    step = 2.0 * math.pi / max(1, hole_count)
    for i in range(hole_count):
        angle = i * step
        hx = pitch_radius * math.cos(angle)
        hy = pitch_radius * math.sin(angle)
        ops.append({
            "type": "create_circle",
            "center": {"x": hx, "y": hy},
            "radius": hole_radius,
            "layer": "Mechanical"
        })
    return {
        "version": "1.0",
        "description": f"Bolt Circle Pattern ({hole_count} holes, pitch radius {pitch_radius} mm)",
        "prompt": f"Bolt circle with {hole_count} holes",
        "operations": ops
    }

def generate_cad(prompt: str, document_json: Optional[str] = None) -> Dict[str, Any]:
    lower = prompt.lower().strip()

    # Clean / Remove Duplicates
    if "clean" in lower or "remove duplicate" in lower:
        return cleanup_duplicates(document_json or "{}")

    # Add Dimensions
    if "dimension" in lower or "add dim" in lower:
        return auto_dimension(document_json or "{}")

    # Room regex: "5m x 4m room", "room 5000 x 3000", "room 5x4"
    room_match = re.search(r'(?:room\s*(?:of\s*)?)?(\d+(?:\.\d+)?)\s*(m|mm)?\s*(?:x|by|\*)\s*(\d+(?:\.\d+)?)\s*(m|mm)?(?:\s*room)?', lower)
    if "room" in lower and room_match:
        w = float(room_match.group(1))
        u1 = room_match.group(2)
        h = float(room_match.group(3))
        u2 = room_match.group(4)
        if u1 == 'm' or (not u1 and w <= 50): w *= 1000
        if u2 == 'm' or (not u2 and h <= 50): h *= 1000

        wall_match = re.search(r'(\d+(?:\.\d+)?)\s*(m|mm)?\s*walls?', lower)
        thick = 200.0
        if wall_match:
            thick = float(wall_match.group(1))
            if wall_match.group(2) == 'm': thick *= 1000
        return generate_room(w, h, thick)

    # Door
    if "door" in lower:
        w = 900.0
        m = re.search(r'(\d+(?:\.\d+)?)\s*(mm|m)?', lower)
        if m:
            val = float(m.group(1))
            if m.group(2) == 'm': val *= 1000
            w = val
        return generate_door(w)

    # Staircase
    if "stair" in lower:
        steps = 12
        m = re.search(r'(\d+)\s*steps?', lower)
        if m: steps = int(m.group(1))
        return generate_stairs(steps)

    # Bolt circle / pattern
    if "bolt" in lower or "flange" in lower:
        holes = 6
        m = re.search(r'(\d+)\s*holes?', lower)
        if m: holes = int(m.group(1))
        r = 100.0
        rm = re.search(r'radius\s*(\d+(?:\.\d+)?)', lower)
        if rm: r = float(rm.group(1))
        return generate_bolt_circle(holes, r)

    # Rectangle: "Create a rectangle 500 mm x 300 mm" (README 4.1)
    rect_match = re.search(r'(?:rect|rectangle)\s*(?:of\s*)?(\d+(?:\.\d+)?)\s*(mm|m)?\s*(?:x|by|\*)\s*(\d+(?:\.\d+)?)\s*(mm|m)?', lower)
    if rect_match:
        w = float(rect_match.group(1))
        if rect_match.group(2) == 'm': w *= 1000
        h = float(rect_match.group(3))
        if rect_match.group(4) == 'm': h *= 1000
        return {
            "version": "1.0",
            "description": f"Rectangle {w} x {h} mm",
            "prompt": prompt,
            "operations": [{
                "type": "create_rectangle",
                "top_left": {"x": 0.0, "y": 0.0},
                "width": w,
                "height": h,
                "layer": "0"
            }]
        }

    # Circle: "circle radius 50"
    if "circle" in lower:
        r = 50.0
        m = re.search(r'(?:radius|r)\s*(\d+(?:\.\d+)?)', lower)
        if m: r = float(m.group(1))
        return {
            "version": "1.0",
            "description": f"Circle radius {r} mm",
            "prompt": prompt,
            "operations": [{
                "type": "create_circle",
                "center": {"x": 0.0, "y": 0.0},
                "radius": r,
                "layer": "0"
            }]
        }

    # Line: "line 100mm"
    if "line" in lower:
        len_val = 100.0
        m = re.search(r'(\d+(?:\.\d+)?)\s*(mm|m)?', lower)
        if m:
            len_val = float(m.group(1))
            if m.group(2) == 'm': len_val *= 1000
        return {
            "version": "1.0",
            "description": f"Line {len_val} mm",
            "prompt": prompt,
            "operations": [{
                "type": "create_line",
                "start": {"x": 0.0, "y": 0.0},
                "end": {"x": len_val, "y": 0.0},
                "layer": "0"
            }]
        }

    return {
        "version": "1.0",
        "description": f"Unrecognized prompt: '{prompt}'",
        "prompt": prompt,
        "operations": []
    }

def analyze_drawing(document_json: str) -> Dict[str, Any]:
    try:
        data = json.loads(document_json) if isinstance(document_json, str) else document_json
    except Exception:
        data = {}

    entities = data.get("entities", [])
    total_entities = len(entities)
    counts: Dict[str, int] = {}
    total_wire_len = 0.0
    duplicates = []
    overlaps = []
    layer_issues = []
    suggestions = []

    min_x, min_y = float("inf"), float("inf")
    max_x, max_y = float("-inf"), float("-inf")

    marked_duplicates = set()

    for idx, ent in enumerate(entities):
        etype = ent.get("type", "unknown")
        counts[etype] = counts.get(etype, 0) + 1
        eid = ent.get("id", f"ent_{idx}")

        if ent.get("layer") == "0":
            layer_issues.append({
                "id": eid,
                "layer": "0",
                "description": f"Entity '{etype}' is assigned to default Layer '0'"
            })

        # Calculate bounding box & length
        if etype == "line":
            s = ent.get("start", {"x": 0, "y": 0})
            e = ent.get("end", {"x": 0, "y": 0})
            l = math.hypot(e["x"] - s["x"], e["y"] - s["y"])
            total_wire_len += l
            min_x = min(min_x, s["x"], e["x"])
            min_y = min(min_y, s["y"], e["y"])
            max_x = max(max_x, s["x"], e["x"])
            max_y = max(max_y, s["y"], e["y"])
        elif etype == "circle":
            c = ent.get("center", {"x": 0, "y": 0})
            r = ent.get("radius", 0.0)
            total_wire_len += 2 * math.pi * r
            min_x = min(min_x, c["x"] - r)
            min_y = min(min_y, c["y"] - r)
            max_x = max(max_x, c["x"] + r)
            max_y = max(max_y, c["y"] + r)
        elif etype == "rectangle":
            tl = ent.get("top_left", {"x": 0, "y": 0})
            w = ent.get("width", 0.0)
            h = ent.get("height", 0.0)
            total_wire_len += 2 * (w + h)
            min_x = min(min_x, tl["x"])
            min_y = min(min_y, tl["y"])
            max_x = max(max_x, tl["x"] + w)
            max_y = max(max_y, tl["y"] + h)

    # Detect duplicates
    for i in range(len(entities)):
        e1 = entities[i]
        id1 = e1.get("id", f"ent_{i}")
        if id1 in marked_duplicates:
            continue

        for j in range(i + 1, len(entities)):
            e2 = entities[j]
            id2 = e2.get("id", f"ent_{j}")
            if id2 in marked_duplicates or e1.get("type") != e2.get("type"):
                continue

            t = e1.get("type")
            is_dup = False
            if t == "line":
                s1, end1 = e1.get("start", {}), e1.get("end", {})
                s2, end2 = e2.get("start", {}), e2.get("end", {})
                direct = points_equal(s1, s2) and points_equal(end1, end2)
                reverse = points_equal(s1, end2) and points_equal(end1, s2)
                is_dup = direct or reverse
            elif t == "circle":
                c1, c2 = e1.get("center", {}), e2.get("center", {})
                r1, r2 = e1.get("radius", 0), e2.get("radius", 0)
                is_dup = points_equal(c1, c2) and abs(r1 - r2) < 1e-3
            elif t == "rectangle":
                tl1, tl2 = e1.get("top_left", {}), e2.get("top_left", {})
                w1, w2 = e1.get("width", 0), e2.get("width", 0)
                h1, h2 = e1.get("height", 0), e2.get("height", 0)
                is_dup = points_equal(tl1, tl2) and abs(w1 - w2) < 1e-3 and abs(h1 - h2) < 1e-3

            if is_dup:
                marked_duplicates.add(id2)
                duplicates.append({
                    "primary_id": id1,
                    "duplicate_id": id2,
                    "entity_type": t,
                    "description": f"Duplicate {t} detected"
                })

    if min_x == float("inf"):
        min_x = min_y = max_x = max_y = 0.0

    bbox = {
        "x": min_x,
        "y": min_y,
        "width": max(0.0, max_x - min_x),
        "height": max(0.0, max_y - min_y)
    }

    if duplicates:
        suggestions.append(f"Detected {len(duplicates)} duplicate entities. Run Auto-Clean to remove them.")
    if layer_issues:
        suggestions.append(f"{len(layer_issues)} entities reside on Layer '0'. Organize into functional layers.")
    if total_entities > 0 and counts.get("dimension", 0) == 0:
        suggestions.append("Drawing has geometric shapes but no dimensions. Consider generating dimensions.")

    return {
        "total_entities": total_entities,
        "total_wire_length": round(total_wire_len, 2),
        "has_issues": bool(duplicates or layer_issues),
        "entity_counts": counts,
        "bounding_box": bbox,
        "duplicates": duplicates,
        "overlaps": overlaps,
        "layer_issues": layer_issues,
        "suggestions": suggestions
    }

def cleanup_duplicates(document_json: str) -> Dict[str, Any]:
    analysis = analyze_drawing(document_json)
    ops = []
    for dup in analysis.get("duplicates", []):
        ops.append({
            "type": "delete_entity",
            "target_id": dup["duplicate_id"]
        })
    return {
        "version": "1.0",
        "description": f"Auto-Clean {len(ops)} duplicate entities",
        "prompt": "Clean duplicate entities",
        "operations": ops
    }

def auto_dimension(document_json: str, target_ids: Optional[List[str]] = None, offset: float = 25.0) -> Dict[str, Any]:
    analysis = analyze_drawing(document_json)
    bbox = analysis.get("bounding_box", {})
    ops = [
        {"type": "add_layer", "layer_name": "Dimensions", "layer_color": "#00B4FF"}
    ]

    if bbox.get("width", 0) > 0 and bbox.get("height", 0) > 0:
        bx = bbox["x"]
        by = bbox["y"]
        bw = bbox["width"]
        bh = bbox["height"]

        # Horizontal overall dimension (bottom)
        ops.append({
            "type": "create_dimension",
            "p1": {"x": bx, "y": by + bh},
            "p2": {"x": bx + bw, "y": by + bh},
            "dim_point": {"x": bx + bw / 2.0, "y": by + bh + offset},
            "layer": "Dimensions"
        })

        # Vertical overall dimension (left)
        ops.append({
            "type": "create_dimension",
            "p1": {"x": bx, "y": by + bh},
            "p2": {"x": bx, "y": by},
            "dim_point": {"x": bx - offset, "y": by + bh / 2.0},
            "layer": "Dimensions"
        })

    return {
        "version": "1.0",
        "description": "Auto-Generated CAD Dimensions",
        "prompt": "Add dimensions to drawing",
        "operations": ops
    }
