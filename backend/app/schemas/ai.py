from pydantic import BaseModel, Field
from typing import List, Optional, Dict, Any

class AiGenerateRequest(BaseModel):
    prompt: str = Field(..., description="Natural language CAD prompt")
    document: Optional[str] = Field(None, description="Current CAD drawing JSON")
    mode: Optional[str] = Field("standard", description="Generation mode: standard, fast, creative")

class AiOperationModel(BaseModel):
    type: str
    target_id: Optional[str] = None
    layer: Optional[str] = "0"
    color: Optional[str] = None
    start: Optional[Dict[str, float]] = None
    end: Optional[Dict[str, float]] = None
    center: Optional[Dict[str, float]] = None
    radius: Optional[float] = None
    top_left: Optional[Dict[str, float]] = None
    width: Optional[float] = None
    height: Optional[float] = None
    start_angle: Optional[float] = None
    sweep_angle: Optional[float] = None
    points: Optional[List[Dict[str, float]]] = None
    closed: Optional[bool] = None
    text: Optional[str] = None
    p1: Optional[Dict[str, float]] = None
    p2: Optional[Dict[str, float]] = None
    dim_point: Optional[Dict[str, float]] = None
    delta: Optional[Dict[str, float]] = None
    origin: Optional[Dict[str, float]] = None
    angle_deg: Optional[float] = None
    layer_name: Optional[str] = None
    layer_color: Optional[str] = None

class AiProposalResponse(BaseModel):
    version: str = "1.0"
    description: str
    prompt: Optional[str] = None
    operations: List[Dict[str, Any]]

class AiAnalyzeRequest(BaseModel):
    document: str = Field(..., description="GenCAD document JSON")

class AiAnalyzeResponse(BaseModel):
    total_entities: int
    total_wire_length: float
    has_issues: bool
    entity_counts: Dict[str, int]
    bounding_box: Dict[str, float]
    duplicates: List[Dict[str, Any]]
    overlaps: List[Dict[str, Any]]
    layer_issues: List[Dict[str, Any]]
    suggestions: List[str]

class AiDimensionRequest(BaseModel):
    document: str = Field(..., description="GenCAD document JSON")
    target_ids: Optional[List[str]] = Field(default_factory=list)
    offset: Optional[float] = 25.0

class AiCleanupRequest(BaseModel):
    document: str = Field(..., description="GenCAD document JSON")

class AiRagRequest(BaseModel):
    query: str
    max_results: Optional[int] = 3

class AiRagResultItem(BaseModel):
    title: str
    category: str
    content: str
    score: float

class AiRagResponse(BaseModel):
    query: str
    answer: str
    results: List[AiRagResultItem]
