FROM python:3.12-slim

WORKDIR /app

COPY pyproject.toml ./
COPY src ./src
RUN pip install --no-cache-dir .

ENV TAMA_HUB_HOST=0.0.0.0 \
    TAMA_HUB_PORT=8000

EXPOSE 8000

CMD ["python", "-m", "src"]
