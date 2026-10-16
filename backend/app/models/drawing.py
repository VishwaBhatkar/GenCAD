import datetime
from sqlalchemy import Column, Integer, String, Text, DateTime, ForeignKey
from sqlalchemy.orm import relationship
from ..database import Base

class Drawing(Base):
    __tablename__ = "drawings"

    id = Column(Integer, primary_key=True, index=True)
    project_id = Column(Integer, ForeignKey("projects.id"), nullable=False)
    name = Column(String(128), index=True, nullable=False)
    format = Column(String(16), default="gencad") # "gencad" or "dxf"
    version = Column(Integer, default=1)
    content = Column(Text, nullable=False) # JSON string or DXF text
    description = Column(String(256), nullable=True)
    created_at = Column(DateTime, default=datetime.datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.datetime.utcnow, onupdate=datetime.datetime.utcnow)

    project = relationship("Project", back_populates="drawings")
    versions = relationship("DrawingVersion", back_populates="drawing", cascade="all, delete-orphan", order_by="desc(DrawingVersion.version_number)")
