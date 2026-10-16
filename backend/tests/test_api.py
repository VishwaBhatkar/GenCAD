import os
import sys
import pytest
from fastapi.testclient import TestClient

# Add backend directory to sys.path
backend_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if backend_dir not in sys.path:
    sys.path.insert(0, backend_dir)

# Use test SQLite database
os.environ["GENCAD_DATABASE_URL"] = "sqlite:///./test_gencad.db"

from main import app
from app.database import Base, engine

client = TestClient(app)

@pytest.fixture(scope="module", autouse=True)
def setup_database():
    Base.metadata.drop_all(bind=engine)
    Base.metadata.create_all(bind=engine)
    yield
    Base.metadata.drop_all(bind=engine)
    if os.path.exists("./test_gencad.db"):
        try:
            os.remove("./test_gencad.db")
        except Exception:
            pass

def test_health_check():
    response = client.get("/health")
    assert response.status_code == 200
    data = response.json()
    assert data["status"] == "healthy"
    assert "version" in data

def test_api_info():
    response = client.get("/api/v1/info")
    assert response.status_code == 200
    data = response.json()
    assert "features" in data
    assert len(data["features"]) >= 3

def test_user_flow():
    # 1. Register
    reg_payload = {
        "username": "cad_architect",
        "email": "architect@gencad.io",
        "password": "StrongPassword123!"
    }
    res = client.post("/api/v1/auth/register", json=reg_payload)
    assert res.status_code == 201
    user_data = res.json()
    assert user_data["username"] == "cad_architect"
    assert user_data["email"] == "architect@gencad.io"
    assert "hashed_password" not in user_data

    # Duplicate register rejected
    res_dup = client.post("/api/v1/auth/register", json=reg_payload)
    assert res_dup.status_code == 400

    # 2. Login
    login_payload = {
        "username": "cad_architect",
        "password": "StrongPassword123!"
    }
    res_login = client.post("/api/v1/auth/login", json=login_payload)
    assert res_login.status_code == 200
    token_data = res_login.json()
    assert "access_token" in token_data
    token = token_data["access_token"]

    # Invalid login rejected
    res_bad = client.post("/api/v1/auth/login", json={"username": "cad_architect", "password": "WrongPassword"})
    assert res_bad.status_code == 401

    headers = {"Authorization": f"Bearer {token}"}

    # 3. Get /me
    res_me = client.get("/api/v1/auth/me", headers=headers)
    assert res_me.status_code == 200
    assert res_me.json()["username"] == "cad_architect"

def test_project_and_drawing_workflow():
    # Authenticate
    res_login = client.post("/api/v1/auth/login", json={"username": "cad_architect", "password": "StrongPassword123!"})
    token = res_login.json()["access_token"]
    headers = {"Authorization": f"Bearer {token}"}

    # 1. Create Project
    proj_payload = {
        "name": "Commercial Complex Building",
        "description": "Floor plan and architectural design"
    }
    res_proj = client.post("/api/v1/projects/", json=proj_payload, headers=headers)
    assert res_proj.status_code == 201
    project = res_proj.json()
    project_id = project["id"]
    assert project["name"] == "Commercial Complex Building"

    # 2. List Projects
    res_list = client.get("/api/v1/projects/", headers=headers)
    assert res_list.status_code == 200
    assert len(res_list.json()) >= 1

    # 3. Create Drawing in Project
    cad_json = '{"format":"gencad","version":"2.0","entities":[{"type":"Line","x1":0,"y1":0,"x2":100,"y2":0}]}'
    dwg_payload = {
        "project_id": project_id,
        "name": "Floor_1_Layout",
        "format": "gencad",
        "content": cad_json,
        "description": "Ground floor layout drawing"
    }
    res_dwg = client.post("/api/v1/drawings/", json=dwg_payload, headers=headers)
    assert res_dwg.status_code == 201
    drawing = res_dwg.json()
    drawing_id = drawing["id"]
    assert drawing["name"] == "Floor_1_Layout"
    assert drawing["version"] == 1
    assert len(drawing["versions"]) == 1

    # 4. Update Drawing (Create Version 2)
    cad_json_v2 = '{"format":"gencad","version":"2.0","entities":[{"type":"Line","x1":0,"y1":0,"x2":100,"y2":0},{"type":"Circle","cx":50,"cy":50,"r":25}]}'
    update_payload = {
        "content": cad_json_v2,
        "commit_message": "Added circular pillar"
    }
    res_upd = client.put(f"/api/v1/drawings/{drawing_id}", json=update_payload, headers=headers)
    assert res_upd.status_code == 200
    drawing_v2 = res_upd.json()
    assert drawing_v2["version"] == 2

    # 5. List Versions
    res_ver = client.get(f"/api/v1/drawings/{drawing_id}/versions", headers=headers)
    assert res_ver.status_code == 200
    versions = res_ver.json()
    assert len(versions) == 2

    # 6. Revert to Version 1
    res_revert = client.post(f"/api/v1/drawings/{drawing_id}/revert/1", headers=headers)
    assert res_revert.status_code == 200
    drawing_v3 = res_revert.json()
    assert drawing_v3["version"] == 3
    assert drawing_v3["content"] == cad_json

    # 7. Download Drawing
    res_dl = client.get(f"/api/v1/drawings/{drawing_id}/download", headers=headers)
    assert res_dl.status_code == 200
    assert "attachment" in res_dl.headers.get("content-disposition", "")
    assert res_dl.text == cad_json

    # 8. Delete Drawing
    res_del_dwg = client.delete(f"/api/v1/drawings/{drawing_id}", headers=headers)
    assert res_del_dwg.status_code == 204

    # 9. Delete Project
    res_del_proj = client.delete(f"/api/v1/projects/{project_id}", headers=headers)
    assert res_del_proj.status_code == 204
