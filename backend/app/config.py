import os
from pydantic import BaseModel

class Settings(BaseModel):
    PROJECT_NAME: str = "GenCAD Cloud Backend"
    VERSION: str = "3.0.0"
    API_V1_STR: str = "/api/v1"
    SECRET_KEY: str = os.getenv("GENCAD_SECRET_KEY", "gencad-insecure-secret-key-change-in-production-123456789")
    ALGORITHM: str = "HS256"
    ACCESS_TOKEN_EXPIRE_MINUTES: int = 60 * 24 # 24 hours
    
    # Database URL: default SQLite for zero-config; can be set to mysql+pymysql://...
    DATABASE_URL: str = os.getenv("GENCAD_DATABASE_URL", "sqlite:///./gencad_cloud.db")

settings = Settings()
