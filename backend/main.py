from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.config import settings
from app.database import init_db
from app.routers import auth_router, projects_router, drawings_router, ai_router

# Initialize database schema on startup
init_db()

app = FastAPI(
    title=settings.PROJECT_NAME,
    version=settings.VERSION,
    description="GenCAD Cloud Backend REST API for CAD project and drawing workspace management",
    docs_url="/docs",
    redoc_url="/redoc"
)

# Enable CORS for cross-origin and local desktop client requests
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Mount API Routers
app.include_router(auth_router, prefix=settings.API_V1_STR)
app.include_router(projects_router, prefix=settings.API_V1_STR)
app.include_router(drawings_router, prefix=settings.API_V1_STR)
app.include_router(ai_router, prefix=settings.API_V1_STR)

@app.get("/health")
def health_check():
    return {
        "status": "healthy",
        "service": "GenCAD Cloud Backend",
        "version": settings.VERSION
    }

@app.get(f"{settings.API_V1_STR}/info")
def get_info():
    return {
        "service": "GenCAD Cloud REST API",
        "version": settings.VERSION,
        "features": [
            "User Authentication & JWT Tokens",
            "Project Workspace Management",
            "CAD Drawing Storage (.gencad JSON & .dxf)",
            "Automated Revision History & Revert",
            "Cloud Sync & Backup"
        ]
    }

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="127.0.0.1", port=8000, reload=True)
