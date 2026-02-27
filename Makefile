.PHONY: configure build test run clean

configure:
	cmake -S . -B build

build: configure
	cmake --build build

test: build
	ctest --test-dir build --output-on-failure

run: build
	./build/furry_app

clean:
	rm -rf build
