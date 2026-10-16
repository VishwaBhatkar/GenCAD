from fastapi import APIRouter, Depends, HTTPException, status, Response
from sqlalchemy.orm import Session
from typing import List, Optional
from ..database import get_db
from ..models.user import User
from ..models.project import Project
from ..models.drawing import Drawing
from ..models.version import DrawingVersion
from ..schemas.drawing import (
    DrawingCreate, DrawingUpdate, DrawingResponse,
    DrawingDetailResponse, DrawingVersionResponse
)
from ..services.auth_service import get_current_user

router = APIRouter(prefix="/drawings", tags=["Drawings"])

@router.get("/", response_model=List[DrawingResponse])
def list_drawings(
    project_id: Optional[int] = None,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    query = db.query(Drawing).join(Project).filter(Project.owner_id == current_user.id)
    if project_id is not None:
        query = query.filter(Drawing.project_id == project_id)
    return query.all()

@router.post("/", response_model=DrawingDetailResponse, status_code=status.HTTP_201_CREATED)
def create_drawing(
    drawing_in: DrawingCreate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    # Verify project belongs to current user
    project = db.query(Project).filter(Project.id == drawing_in.project_id, Project.owner_id == current_user.id).first()
    if not project:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Project not found")

    drawing = Drawing(
        project_id=drawing_in.project_id,
        name=drawing_in.name,
        format=drawing_in.format,
        content=drawing_in.content,
        description=drawing_in.description,
        version=1
    )
    db.add(drawing)
    db.commit()
    db.refresh(drawing)

    # Create initial version entry
    initial_version = DrawingVersion(
        drawing_id=drawing.id,
        version_number=1,
        content=drawing.content,
        commit_message="Initial creation"
    )
    db.add(initial_version)
    db.commit()
    db.refresh(drawing)

    return drawing

@router.get("/{drawing_id}", response_model=DrawingDetailResponse)
def get_drawing(
    drawing_id: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    drawing = db.query(Drawing).join(Project).filter(
        Drawing.id == drawing_id,
        Project.owner_id == current_user.id
    ).first()
    if not drawing:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Drawing not found")
    return drawing

@router.put("/{drawing_id}", response_model=DrawingDetailResponse)
def update_drawing(
    drawing_id: int,
    drawing_in: DrawingUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    drawing = db.query(Drawing).join(Project).filter(
        Drawing.id == drawing_id,
        Project.owner_id == current_user.id
    ).first()
    if not drawing:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Drawing not found")

    content_changed = False
    if drawing_in.content is not None and drawing_in.content != drawing.content:
        drawing.content = drawing_in.content
        drawing.version += 1
        content_changed = True

    if drawing_in.name is not None:
        drawing.name = drawing_in.name
    if drawing_in.description is not None:
        drawing.description = drawing_in.description

    db.commit()

    if content_changed:
        version_entry = DrawingVersion(
            drawing_id=drawing.id,
            version_number=drawing.version,
            content=drawing.content,
            commit_message=drawing_in.commit_message or f"Revision {drawing.version}"
        )
        db.add(version_entry)
        db.commit()

    db.refresh(drawing)
    return drawing

@router.delete("/{drawing_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_drawing(
    drawing_id: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    drawing = db.query(Drawing).join(Project).filter(
        Drawing.id == drawing_id,
        Project.owner_id == current_user.id
    ).first()
    if not drawing:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Drawing not found")

    db.delete(drawing)
    db.commit()
    return None

@router.get("/{drawing_id}/versions", response_model=List[DrawingVersionResponse])
def get_drawing_versions(
    drawing_id: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    drawing = db.query(Drawing).join(Project).filter(
        Drawing.id == drawing_id,
        Project.owner_id == current_user.id
    ).first()
    if not drawing:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Drawing not found")

    return drawing.versions

@router.post("/{drawing_id}/revert/{version_number}", response_model=DrawingDetailResponse)
def revert_drawing(
    drawing_id: int,
    version_number: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    drawing = db.query(Drawing).join(Project).filter(
        Drawing.id == drawing_id,
        Project.owner_id == current_user.id
    ).first()
    if not drawing:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Drawing not found")

    target_version = db.query(DrawingVersion).filter(
        DrawingVersion.drawing_id == drawing.id,
        DrawingVersion.version_number == version_number
    ).first()
    if not target_version:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail=f"Version {version_number} not found")

    drawing.content = target_version.content
    drawing.version += 1
    db.commit()

    revert_version_entry = DrawingVersion(
        drawing_id=drawing.id,
        version_number=drawing.version,
        content=drawing.content,
        commit_message=f"Reverted to version {version_number}"
    )
    db.add(revert_version_entry)
    db.commit()
    db.refresh(drawing)

    return drawing

@router.get("/{drawing_id}/download")
def download_drawing(
    drawing_id: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    drawing = db.query(Drawing).join(Project).filter(
        Drawing.id == drawing_id,
        Project.owner_id == current_user.id
    ).first()
    if not drawing:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Drawing not found")

    ext = ".dxf" if drawing.format == "dxf" else ".gencad"
    media_type = "application/dxf" if drawing.format == "dxf" else "application/json"
    filename = f"{drawing.name}{ext}"

    return Response(
        content=drawing.content,
        media_type=media_type,
        headers={"Content-Disposition": f'attachment; filename="{filename}"'}
    )
