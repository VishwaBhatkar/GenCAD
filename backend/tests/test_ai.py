import os
import sys
import json
import pytest
from fastapi.testclient import TestClient

backend_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if backend_dir not in sys.path:
    sys.path.insert(0, backend_dir)

os.environ["GENCAD_DATABASE_URL"] = "sqlite:///./test_gencad_ai.db"

from main import app

client = TestClient(app)

def test_ai_generate_rectangle():
    response = client.post("/api/v1/ai/generate", json={
        "prompt": "Create a rectangle 500 mm x 300 mm"
    })
    assert response.status_code == 200
    data = response.json()
    assert data["version"] == "1.0"
    assert len(data["operations"]) == 1
    op = data["operations"][0]
    assert op["type"] == "create_rectangle"
    assert op["width"] == 500.0
    assert op["height"] == 300.0

def test_ai_generate_room():
    response = client.post("/api/v1/ai/generate", json={
        "prompt": "Create a 5m x 4m room with 200mm walls"
    })
    assert response.status_code == 200
    data = response.json()
    assert data["version"] == "1.0"
    # Should contain wall layers, outer rect, inner rect, text label, dimensions
    assert len(data["operations"]) >= 5
    types = [op["type"] for op in data["operations"]]
    assert "add_layer" in types
    assert "create_rectangle" in types
    assert "create_text" in types
    assert "create_dimension" in types

def test_ai_generate_door_and_stairs():
    resp_door = client.post("/api/v1/ai/generate", json={"prompt": "Add a 900mm door"})
    assert resp_door.status_code == 200
    door_ops = resp_door.json()["operations"]
    door_types = [op["type"] for op in door_ops]
    assert "create_line" in door_types
    assert "create_arc" in door_types

    resp_stairs = client.post("/api/v1/ai/generate", json={"prompt": "Create a staircase with 10 steps"})
    assert resp_stairs.status_code == 200
    stairs_ops = resp_stairs.json()["operations"]
    assert len(stairs_ops) > 10

def test_ai_generate_bolt_pattern():
    response = client.post("/api/v1/ai/generate", json={"prompt": "Bolt circle with 6 holes radius 100"})
    assert response.status_code == 200
    data = response.json()
    circles = [op for op in data["operations"] if op["type"] == "create_circle"]
    # Pitch circle + bore + 6 bolt holes = 8 circles
    assert len(circles) == 8

def test_ai_analyze_drawing():
    sample_doc = {
        "format": "gencad",
        "version": "2.0",
        "entities": [
            {
                "id": "11111111-1111-1111-1111-111111111111",
                "type": "line",
                "start": {"x": 0.0, "y": 0.0},
                "end": {"x": 100.0, "y": 0.0},
                "layer": "0"
            },
            {
                "id": "22222222-2222-2222-2222-222222222222",
                "type": "line",
                "start": {"x": 0.0, "y": 0.0},
                "end": {"x": 100.0, "y": 0.0},
                "layer": "0"
            },
            {
                "id": "33333333-3333-3333-3333-333333333333",
                "type": "rectangle",
                "top_left": {"x": 10.0, "y": 10.0},
                "width": 50.0,
                "height": 30.0,
                "layer": "0"
            }
        ]
    }
    response = client.post("/api/v1/ai/analyze", json={
        "document": json.dumps(sample_doc)
    })
    assert response.status_code == 200
    data = response.json()
    assert data["total_entities"] == 3
    assert len(data["duplicates"]) == 1
    assert data["duplicates"][0]["duplicate_id"] == "22222222-2222-2222-2222-222222222222"
    assert data["has_issues"] is True

def test_ai_cleanup_duplicates():
    sample_doc = {
        "format": "gencad",
        "version": "2.0",
        "entities": [
            {
                "id": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                "type": "circle",
                "center": {"x": 50.0, "y": 50.0},
                "radius": 25.0,
                "layer": "0"
            },
            {
                "id": "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb",
                "type": "circle",
                "center": {"x": 50.0, "y": 50.0},
                "radius": 25.0,
                "layer": "0"
            }
        ]
    }
    response = client.post("/api/v1/ai/cleanup", json={
        "document": json.dumps(sample_doc)
    })
    assert response.status_code == 200
    data = response.json()
    assert len(data["operations"]) == 1
    assert data["operations"][0]["type"] == "delete_entity"
    assert data["operations"][0]["target_id"] == "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"

def test_ai_auto_dimension():
    sample_doc = {
        "format": "gencad",
        "version": "2.0",
        "entities": [
            {
                "id": "cccccccc-cccc-cccc-cccc-cccccccccccc",
                "type": "rectangle",
                "top_left": {"x": 0.0, "y": 0.0},
                "width": 200.0,
                "height": 100.0,
                "layer": "0"
            }
        ]
    }
    response = client.post("/api/v1/ai/dimension", json={
        "document": json.dumps(sample_doc),
        "offset": 30.0
    })
    assert response.status_code == 200
    data = response.json()
    dims = [op for op in data["operations"] if op["type"] == "create_dimension"]
    assert len(dims) == 2

def test_ai_rag_query():
    response = client.post("/api/v1/ai/rag/query", json={
        "query": "How do I use Trim and Extend tools in GenCAD?"
    })
    assert response.status_code == 200
    data = response.json()
    assert len(data["results"]) > 0
    assert "Modify" in data["results"][0]["title"] or "Tools" in data["results"][0]["category"]
    assert "answer" in data
    assert len(data["answer"]) > 10

def test_ai_templates_and_standards():
    t_resp = client.get("/api/v1/ai/templates")
    assert t_resp.status_code == 200
    assert len(t_resp.json()["templates"]) >= 5

    s_resp = client.get("/api/v1/ai/standards")
    assert s_resp.status_code == 200
    assert len(s_resp.json()["standards"]) >= 2
