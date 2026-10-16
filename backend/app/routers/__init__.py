from .auth import router as auth_router
from .projects import router as projects_router
from .drawings import router as drawings_router
from .ai import router as ai_router

__all__ = ["auth_router", "projects_router", "drawings_router", "ai_router"]
