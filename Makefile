FORMAT_FILES := $(shell find src include tests -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cpp' \))
DOCKER_COMPOSE ?= docker compose

.PHONY: format test
format:
	@if [ -z "$(FORMAT_FILES)" ]; then \
		echo "No C++ files found to format."; \
	else \
		clang-format -i $(FORMAT_FILES); \
		echo "Formatted $(words $(FORMAT_FILES)) file(s)."; \
	fi

test:
	@$(DOCKER_COMPOSE) run --rm bytetaper-unit-test
