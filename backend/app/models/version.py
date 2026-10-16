import datetime
from sqlalchemy import Column, Integer, String, Text, DateTime, ForeignKey
from sqlalchemy.orm import relationship
from ..database import Base

class DrawingVersion(Base):
    __tablename__ = "drawing_versions"

    id = Column(Integer, primary_key=True, index=True)
    drawing_id = Column(Integer, ForeignKey("drawings.id"), nullable=False)
    version_number = Column(Integer, nullable=False)
    content = Column(Text, nullable=False)
    commit_message = Column(String(256), default="Update drawing")
    created_at = Column(DateTime, default=datetime.datetime.utcnow)

    drawing = relationship("Drawing", back_populates="versions")
