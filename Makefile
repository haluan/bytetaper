FORMAT_FILES := $(shell find src include tests -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cpp' \))
DOCKER_COMPOSE ?= docker compose
LOCAL_UID ?= $(shell id -u)
LOCAL_GID ?= $(shell id -g)
COMPOSE_ENV := LOCAL_UID=$(LOCAL_UID) LOCAL_GID=$(LOCAL_GID)

.PHONY: format test smoke-test
format:
	@if [ -z "$(FORMAT_FILES)" ]; then \
		echo "No C++ files found to format."; \
	else \
		clang-format -i $(FORMAT_FILES); \
		echo "Formatted $(words $(FORMAT_FILES)) file(s)."; \
	fi

test:
	@rm -rf build
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-unit-test

smoke-test:
	@rm -rf build
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-smoke-test

benchmark:
	@rm -rf build
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-benchmark

all-tests:
	@rm -rf build
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) build bytetaper-dev
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-unit-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-cache-hit-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-cache-e2e-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-smoke-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-integration-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-pagination-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-max-limit-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-compression-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-coalescing-burst-test
	@$(COMPOSE_ENV) $(DOCKER_COMPOSE) run --rm bytetaper-coalescing-e2e-test
