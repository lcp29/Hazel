PROJECT_NAME := Test
SCRIPT_DIR := Assets/Scripts
BIN_DIR := Assets/Scripts/Binaries
OUTPUT := Assets/Scripts/Binaries/Test.dll
SCRIPT_CORE := Resources/Scripts/Hazel-ScriptCore.dll
CS_FILES := $(shell find $(SCRIPT_DIR) -name '*.cs')

all: $(OUTPUT)

$(OUTPUT): $(CS_FILES)
	@mkdir -p $(BIN_DIR)
	mcs -target:library -sdk:4.5 -r:$(SCRIPT_CORE) -out:$(OUTPUT) $(CS_FILES)

clean:
	rm -f $(OUTPUT)
