from pydantic import BaseModel, ConfigDict
from typing import Optional, List
from datetime import datetime

class DrawingBase(BaseModel):
    name: str
    format: str = "gencad" # "gencad" or "dxf"
    description: Optional[str] = None

class DrawingCreate(DrawingBase):
    project_id: int
    content: str

class DrawingUpdate(BaseModel):
    name: Optional[str] = None
    content: Optional[str] = None
    description: Optional[str] = None
    commit_message: Optional[str] = "Update drawing"

class DrawingVersionResponse(BaseModel):
    id: int
    version_number: int
    commit_message: Optional[str]
    created_at: datetime
    model_config = ConfigDict(from_attributes=True)

class DrawingResponse(DrawingBase):
    id: int
    project_id: int
    version: int
    created_at: datetime
    updated_at: datetime
    model_config = ConfigDict(from_attributes=True)

class DrawingDetailResponse(DrawingResponse):
    content: str
    versions: List[DrawingVersionResponse] = []
    model_config = ConfigDict(from_attributes=True)
