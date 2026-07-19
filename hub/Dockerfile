FROM python:3.12-slim

WORKDIR /app

COPY hub/pyproject.toml ./
COPY hub/src ./src
RUN pip install --no-cache-dir .

COPY app/ui ./ui
COPY docs/css/tokens.css ./ui/tokens.css

ENV TAMA_HUB_HOST=0.0.0.0 \
    TAMA_HUB_PORT=8000 \
    TAMA_UI_DIR=/app/ui \
    TAMA_DATA_DIR=/data

EXPOSE 8000

CMD ["python", "-m", "src"]
