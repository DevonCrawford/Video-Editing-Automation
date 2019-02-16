# Configure file directories
BIN_DIR=bin
SRC_DIR=src
INCLUDE_DIR=include
EXAMPLES_DIR=examples
EXAMPLES_FFMPEG_DIR=$(EXAMPLES_DIR)/ffmpeg
BIN_EXAMPLES_DIR=$(BIN_DIR)/$(EXAMPLES_DIR)
BIN_EXAMPLES_FFMPEG_DIR=$(BIN_EXAMPLES_DIR)/ffmpeg

# use pkg-config for getting CFLAGS and LDLIBS
FFMPEG_LIBS=libavcodec		\
			libavformat		\
			libavutil		\
			libswscale		\
			libswresample

CFLAGS += -Wall -g -I$(INCLUDE_DIR)/
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)

# Create lists of src and object files for src dir
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)									# Get .c files in source
SRC_OBJS=$(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o, $(SRC_FILES))		# Get name of .o files in source

EXAMPLES_FILES=$(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLES_OBJS=$(patsubst $(EXAMPLES_DIR)/%.c,$(BIN_EXAMPLES_DIR)/%.o, $(EXAMPLES_FILES))
EXAMPLES_EXES=$(patsubst %.o,%, $(EXAMPLES_OBJS))
EXAMPLES_TARGETS=$(patsubst $(EXAMPLES_DIR)/%.c,%, $(EXAMPLES_FILES))

# Create lists of src, object and exe files for examples dir
EXAMPLES_FFMPEG_FILES=$(wildcard $(EXAMPLES_FFMPEG_DIR)/*.c)
EXAMPLES_FFMPEG_OBJS=$(patsubst $(EXAMPLES_FFMPEG_DIR)/%.c,$(BIN_EXAMPLES_FFMPEG_DIR)/%.o, $(EXAMPLES_FFMPEG_FILES))
EXAMPLES_FFMPEG_EXES=$(patsubst $(EXAMPLES_FFMPEG_DIR)/%.c,$(BIN_EXAMPLES_FFMPEG_DIR)/%, $(EXAMPLES_FFMPEG_FILES))

# Create bin directories if they dont exist
$(shell if [ ! -d "${BIN_EXAMPLES_FFMPEG_DIR}" ]; then mkdir -p ${BIN_EXAMPLES_FFMPEG_DIR}; fi;)

# Create src object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

# Create examples object files
$(BIN_EXAMPLES_DIR)/%.o: $(EXAMPLES_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

# Create examples-ffmpeg object files
$(BIN_EXAMPLES_FFMPEG_DIR)/%.o: $(EXAMPLES_FFMPEG_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

.phony: all clean-test clean

all: src examples-ffmpeg examples

src: $(SRC_OBJS)

examples: $(EXAMPLES_TARGETS)

# The only reason we can automatically create executables is that we are
# linking ffmpeg libs.. which contain all necessary object files.
# Maybe I'll do this in the future for my own examples once I have my own library
examples-ffmpeg: $(EXAMPLES_FFMPEG_OBJS) $(EXAMPLES_FFMPEG_EXES)

clean-examples:
	$(RM) $(EXAMPLES_EXES) $(EXAMPLES_OBJS)

clean-examples-ffmpeg:
	$(RM) $(EXAMPLES_FFMPEG_EXES) $(EXAMPLES_FFMPEG_OBJS)

clean-src:
	$(RM) $(SRC_OBJS)

clean:
	rm -rf $(BIN_DIR)/*

DBE=$(BIN_EXAMPLES_DIR)
.SECONDEXPANSION:
# EXAMPLES
# No need to change target, just change first assignment of CUT_OBJS
# So long as target is the basename of an example file.. it will include all required .o files.
TEST_CLIP_OBJS=VideoContext Timebase Clip
test-clip: $(patsubst %, $(DBE)/%.o, $$@) $(patsubst %, $(BIN_DIR)/%.o, $(TEST_CLIP_OBJS))
	$(CC) $(CFLAGS) -o $(DBE)/$@ $^ $(LDLIBS)

TEST_SEQ_OBJS=Sequence Clip LinkedListAPI VideoContext Timebase
test-sequence: $(patsubst %, $(DBE)/%.o, $$@) $(patsubst %, $(BIN_DIR)/%.o, $(TEST_SEQ_OBJS))
	$(CC) $(CFLAGS) -o $(DBE)/$@ $^ $(LDLIBS)