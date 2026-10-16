import re
from typing import List, Dict, Any

KNOWLEDGE_BASE = [
    {
        "title": "GenCAD 2D Drawing Tools",
        "category": "Tools",
        "content": (
            "GenCAD provides core 2D drawing primitives:\n"
            "• Line (L): Creates straight line segments between two points.\n"
            "• Circle (C): Creates circles specified by center point and radius.\n"
            "• Rectangle (REC): Creates rectangles given two diagonal corners or width/height.\n"
            "• Arc (A): Creates circular arcs specified by center, radius, and angular sweep.\n"
            "• Polyline (PL): Creates continuous multi-segment polylines (open or closed).\n"
            "• Text (T): Adds single-line text annotations with customizable font height and rotation.\n"
            "• Dimension (D): Generates engineering dimensions measuring distance between two points."
        ),
        "keywords": ["line", "circle", "rectangle", "arc", "polyline", "text", "dimension", "draw", "primitives"]
    },
    {
        "title": "GenCAD Modify Tools",
        "category": "Tools",
        "content": (
            "Modify commands transform existing CAD geometry:\n"
            "• Move (M): Translates selected entities by a specified displacement vector (dx, dy).\n"
            "• Rotate (RO): Rotates entities about a chosen pivot point by an angle in degrees.\n"
            "• Mirror (MI): Mirrors entities across an arbitrary reflection line with copy option.\n"
            "• Copy (CO): Duplicates selected entities at target locations.\n"
            "• Offset (O): Generates parallel contours at a set distance.\n"
            "• Trim (TR): Cuts entities back to intersection boundaries.\n"
            "• Extend (EX): Extends entities forward to boundary edges."
        ),
        "keywords": ["move", "rotate", "mirror", "copy", "offset", "trim", "extend", "transform", "edit"]
    },
    {
        "title": "ISO 128 Technical Drafting Standards",
        "category": "Standards",
        "content": (
            "ISO 128 specifies general principles of presentation in technical drawings:\n"
            "• Continuous Thick Line: Visible outlines, edges, crests, and exterior boundaries.\n"
            "• Continuous Thin Line: Dimension lines, extension lines, hatching, leaders.\n"
            "• Dashed Line: Hidden outlines and hidden edges.\n"
            "• Dash-Dot Thin Line: Centerlines, pitch circles, and axes of symmetry.\n"
            "Paper formats follow ISO 216: A0, A1, A2, A3, A4 (210x297mm)."
        ),
        "keywords": ["iso", "iso 128", "standards", "line weight", "centerline", "hidden line", "sheet size"]
    },
    {
        "title": "ANSI / ASME Y14.5 Dimensioning Guidelines",
        "category": "Standards",
        "content": (
            "ANSI/ASME Y14.5 dictates geometric dimensioning and tolerancing:\n"
            "• Each feature must be dimensioned completely and clearly.\n"
            "• Do not place redundant or duplicate dimensions.\n"
            "• Extension lines must clear the geometry outline by 1.5mm to 2mm.\n"
            "• Dimension line text should be placed centered above or in the break of the line.\n"
            "• Dimension lines should avoid crossing other dimension lines."
        ),
        "keywords": ["ansi", "asme", "y14.5", "dimensioning", "tolerance", "annotation", "drafting"]
    },
    {
        "title": "AIA CAD Layer Standards",
        "category": "Standards",
        "content": (
            "The American Institute of Architects (AIA) defines standard architectural layers:\n"
            "• A-WALL: Interior and exterior wall outlines.\n"
            "• A-DOOR: Door panels, swings, frames, and hardware.\n"
            "• A-GLAZ: Glazing, window frames, and glass partitions.\n"
            "• A-FLOR: Floor finishes, stair stringers, and treads.\n"
            "• A-ANNO-DIMS: Engineering dimensions and callouts.\n"
            "• A-ANNO-TEXT: Room names, titles, and notes.\n"
            "• Mechanical / Electrical: M-HVAC, E-POWR."
        ),
        "keywords": ["aia", "layer", "layers", "architecture", "wall", "door", "window", "naming"]
    },
    {
        "title": "GenCAD Keyboard Shortcuts",
        "category": "Shortcuts",
        "content": (
            "Accelerate drafting with GenCAD shortcuts:\n"
            "• L - Line\n• C - Circle\n• REC - Rectangle\n• A - Arc\n• PL - Polyline\n"
            "• M - Move\n• RO - Rotate\n• MI - Mirror\n• CO - Copy\n• TR - Trim\n• EX - Extend\n"
            "• D - Dimension\n• Ctrl+Z - Undo\n• Ctrl+Y - Redo\n• F3 - Object Snap\n• F8 - Ortho\n• F7 - Grid"
        ),
        "keywords": ["shortcuts", "hotkeys", "keyboard", "commands", "f3", "f8", "undo", "redo"]
    }
]

def search_knowledge_base(query: str, max_results: int = 3) -> List[Dict[str, Any]]:
    tokens = [t.lower() for t in re.split(r'[\s,;.!?/\-]+', query) if len(t) >= 2]
    scored = []

    for article in KNOWLEDGE_BASE:
        score = 0.0
        title_lower = article["title"].lower()
        content_lower = article["content"].lower()

        for token in tokens:
            for kw in article["keywords"]:
                if kw == token:
                    score += 5.0
                elif token in kw:
                    score += 2.0
            if token in title_lower:
                score += 4.0
            if token in content_lower:
                score += 1.0

        if score > 0.0:
            scored.append({
                "title": article["title"],
                "category": article["category"],
                "content": article["content"],
                "score": score
            })

    scored.sort(key=lambda x: x["score"], reverse=True)
    return scored[:max_results]

def answer_question(query: str) -> str:
    results = search_knowledge_base(query, max_results=2)
    if not results:
        return (
            f"No specific CAD documentation found matching '{query}'. "
            "Try asking about drawing tools (Line, Circle, Rectangle), modify tools (Trim, Extend), "
            "standards (ISO 128, ANSI Y14.5, AIA Layers), or keyboard shortcuts."
        )

    ans = f"### {results[0]['title']}\n\n{results[0]['content']}"
    if len(results) > 1 and results[1]["score"] > 3.0:
        ans += f"\n\n---\n**Related: {results[1]['title']}**\n{results[1]['content']}"
    return ans
