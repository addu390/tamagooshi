.PHONY: help hub up down logs hub-sim hub-test sim sim-live brand macos dmg

BROKER ?= localhost:1883
export TAMA_BRAND ?= gooshi
export TAMA_DEV ?=

help:
	@echo "Run it (a flashed device and a Mac is all you need):"
	@echo "  make hub       # run the hub, pairs with your device over BLE"
	@echo "  make up        # self-hosted stack: broker + hub with dashboard (docker compose)"
	@echo "                 #   dashboard at http://localhost:8000, change with TAMA_PORT=<port>"
	@echo ""
	@echo "Develop (simulator, MQTT stack, tests):"
	@echo "  make sim       # desktop simulator, no board or data needed"
	@echo "                 #   TAMA_BRAND=<id> picks a brand, TAMA_DEV=<id> bakes in a developer name"
	@echo "  make sim-live  # simulator against the dev stack's broker"
	@echo "  make down      # stop the dev stack"
	@echo "  make logs      # tail dev stack hub logs"
	@echo "  make hub-sim TAMA_BRAND=<id>  # hub over MQTT for the simulator (needs a broker on $(BROKER))"
	@echo "  make hub-test  # hub unit tests"
	@echo "  make brand TAMA_BRAND=<id>  # generate a brand's firmware headers into firmware/.gen/current"
	@echo "  make macos     # build the Mac menu bar app into build/Tamagooshi.app"
	@echo "  make dmg       # package the app as build/Tamagooshi-<version>.dmg"

hub:
	cd hub/backend && TAMA_TRANSPORT=ble:gatt python -m src

up:
	docker compose up -d --build

down:
	docker compose down

logs:
	docker compose logs -f hub

hub-sim:
	cd hub/backend && TAMA_TRANSPORT=wifi:mqtt TAMA_BROKER=$(BROKER) TAMA_DEVICE_ID=sim python -m src

hub-test:
	cd hub/backend && python -m pytest

sim:
	cd firmware && pio run -e native_sim -t exec

sim-live:
	cd firmware && TAMA_BROKER=$(BROKER) pio run -e native_sim -t exec

brand:
	cd firmware/tools && python3 -m gen brand $(TAMA_BRAND)

macos:
	hub/macos/scripts/build.sh

dmg: macos
	hub/macos/scripts/dmg.sh
