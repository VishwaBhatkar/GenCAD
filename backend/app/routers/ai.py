from fastapi import APIRouter, HTTPException
from typing import Dict, Any, List

from app.schemas.ai import (
    AiGenerateRequest,
    AiProposalResponse,
    AiAnalyzeRequest,
    AiAnalyzeResponse,
    AiDimensionRequest,
    AiCleanupRequest,
    AiRagRequest,
    AiRagResponse,
    AiRagResultItem
)
from app.services.ai_service import (
    generate_cad,
    analyze_drawing,
    cleanup_duplicates,
    auto_dimension
)
from app.services.rag_service import (
    search_knowledge_base,
    answer_question,
    KNOWLEDGE_BASE
)

router = APIRouter(prefix="/ai", tags=["AI CAD Assistant"])

@router.post("/generate", response_model=AiProposalResponse)
def generate_endpoint(req: AiGenerateRequest):
    """
    Translates natural-language CAD prompts into structured GenCAD DSL operations.
    """
    try:
        proposal = generate_cad(req.prompt, req.document)
        return proposal
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@router.post("/analyze", response_model=AiAnalyzeResponse)
def analyze_endpoint(req: AiAnalyzeRequest):
    """
    Analyzes GenCAD document geometry for duplicate elements, overlaps,
    layer inconsistencies, wire length, and bounding box metrics.
    """
    try:
        analysis = analyze_drawing(req.document)
        return analysis
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@router.post("/cleanup", response_model=AiProposalResponse)
def cleanup_endpoint(req: AiCleanupRequest):
    """
    Generates an atomic cleanup proposal deleting duplicate CAD entities.
    """
    try:
        proposal = cleanup_duplicates(req.document)
        return proposal
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@router.post("/dimension", response_model=AiProposalResponse)
def dimension_endpoint(req: AiDimensionRequest):
    """
    Generates engineering dimension entities according to standard offset rules.
    """
    try:
        proposal = auto_dimension(req.document, req.target_ids, req.offset or 25.0)
        return proposal
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@router.post("/rag/query", response_model=AiRagResponse)
def rag_query_endpoint(req: AiRagRequest):
    """
    Answers drafting questions and retrieves CAD standards & GenCAD manual sections.
    """
    try:
        results = search_knowledge_base(req.query, req.max_results or 3)
        answer = answer_question(req.query)
        items = [
            AiRagResultItem(
                title=r["title"],
                category=r["category"],
                content=r["content"],
                score=r["score"]
            )
            for r in results
        ]
        return AiRagResponse(query=req.query, answer=answer, results=items)
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@router.get("/templates")
def list_templates():
    """
    Returns available parametric component generators.
    """
    return {
        "templates": [
            {
                "name": "Architectural Room",
                "description": "Generates double-wall rooms with interior cutout, label, and dimensions",
                "sample_prompts": ["Create a 5m x 4m room", "room 6000 x 4000 with 200mm walls"]
            },
            {
                "name": "Architectural Door",
                "description": "Generates 90-degree swing arc and leaf line on A-DOOR layer",
                "sample_prompts": ["Add a 900mm door on south wall", "door 800mm"]
            },
            {
                "name": "Architectural Windows",
                "description": "Generates multi-pane window openings on A-GLAZ layer",
                "sample_prompts": ["Create four windows", "window 1200mm"]
            },
            {
                "name": "Staircase Flight",
                "description": "Generates stair stringers, treads, and UP direction arrow",
                "sample_prompts": ["Create a staircase with 12 steps", "stair 10 steps width 1000"]
            },
            {
                "name": "Mechanical Bolt Pattern",
                "description": "Generates pitch circle, central bore, and circular bolt hole pattern",
                "sample_prompts": ["Bolt circle with 6 holes radius 100", "flange with 8 bolts"]
            }
        ]
    }

@router.get("/standards")
def list_standards():
    """
    Returns CAD standards catalog (ISO 128, ANSI Y14.5, AIA Layer standards).
    """
    return {
        "standards": [art["title"] for art in KNOWLEDGE_BASE if art["category"] == "Standards"]
    }
