.PHONY: help hub up down logs hub-test sim brand

export TAMA_BRAND ?= gooshi

help:
	@echo "Run it (a flashed device and a Mac is all you need):"
	@echo "  make hub       # run the hub, pairs with your device over BLE"
	@echo "  make up        # self-hosted hub with dashboard (docker compose)"
	@echo "                 #   dashboard at http://localhost:8000, change with TAMA_PORT=<port>"
	@echo ""
	@echo "Develop (simulator, tests):"
	@echo "  make sim       # desktop simulator, no board or data needed"
	@echo "                 #   TAMA_BRAND=<id> picks a brand"
	@echo "  make down      # stop the docker hub"
	@echo "  make logs      # tail docker hub logs"
	@echo "  make hub-test  # hub unit tests"
	@echo "  make brand TAMA_BRAND=<id>  # generate a brand's firmware headers into firmware/.gen/current"

hub:
	cd hub/backend && TAMA_TRANSPORT=ble:gatt python -m src

up:
	docker compose up -d --build

down:
	docker compose down

logs:
	docker compose logs -f hub

hub-test:
	cd hub/backend && python -m pytest

sim:
	cd firmware && pio run -e native_sim -t exec

brand:
	cd firmware/tools && python3 -m gen brand $(TAMA_BRAND)
