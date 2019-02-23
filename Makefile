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

COMPILE=$(CC) $(CFLAGS) -c $^ -o $@
LINK_EXE=$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Create lists of src and object files for src dir
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)									# Get .c files in source
SRC_OBJS=$(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o, $(SRC_FILES))		# Get name of .o files in source

EXAMPLES_FILES=$(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLES_OBJS=$(patsubst $(EXAMPLES_DIR)/%.c,$(BIN_EXAMPLES_DIR)/%.o, $(EXAMPLES_FILES))
EXAMPLES_EXES=$(patsubst %.o,%, $(EXAMPLES_OBJS))
EXAMPLES_TARGETS=$(patsubst $(EXAMPLES_DIR)/%.c,$(BIN_EXAMPLES_DIR)/%, $(EXAMPLES_FILES))

# Create lists of src, object and exe files for examples dir
EXAMPLES_FFMPEG_FILES=$(wildcard $(EXAMPLES_FFMPEG_DIR)/*.c)
EXAMPLES_FFMPEG_OBJS=$(patsubst $(EXAMPLES_FFMPEG_DIR)/%.c,$(BIN_EXAMPLES_FFMPEG_DIR)/%.o, $(EXAMPLES_FFMPEG_FILES))
EXAMPLES_FFMPEG_EXES=$(patsubst $(EXAMPLES_FFMPEG_DIR)/%.c,$(BIN_EXAMPLES_FFMPEG_DIR)/%, $(EXAMPLES_FFMPEG_FILES))

# Create bin directories if they dont exist
$(shell if [ ! -d "${BIN_EXAMPLES_FFMPEG_DIR}" ]; then mkdir -p ${BIN_EXAMPLES_FFMPEG_DIR}; fi;)

# Create src object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(COMPILE)

# Create examples object files
$(BIN_EXAMPLES_DIR)/%.o: $(EXAMPLES_DIR)/%.c
	$(COMPILE)

# Create examples-ffmpeg object files
$(BIN_EXAMPLES_FFMPEG_DIR)/%.o: $(EXAMPLES_FFMPEG_DIR)/%.c
	$(COMPILE)

# A phony target is one that is not really the name of a file;
# rather it is just a name for a recipe to be executed when you make an explicit request
# All targets that generate files should have target name = name of file
# so that make can correctly track if we need to rebuild the target
.phony: all src examples clean clean-src clean-examples-ffmpeg clean-examples

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


# Create executables!
# For new exe's follow this format:
# OBJS_BASE are the object files that the executable needs to run
# Must change target name to $(DBE){name of exe}
DBE=$(BIN_EXAMPLES_DIR)/
.SECONDEXPANSION:

OBJS_BASE=VideoContext Timebase Clip
$(DBE)test-clip: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

OBJS_BASE=Sequence Clip LinkedListAPI VideoContext Timebase OutputContext \
			SequenceEncode SequenceDecode ClipDecode Util
$(DBE)test-sequence: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

OBJS_BASE=VideoContext Timebase Clip ClipDecode
$(DBE)test-clip-decode: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

OBJS_BASE=VideoContext Timebase Clip ClipDecode Sequence LinkedListAPI \
			SequenceDecode Util
$(DBE)test-sequence-decode: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

OBJS_BASE=VideoContext Clip ClipDecode ClipEncode OutputContext Timebase \
 			Sequence LinkedListAPI SequenceEncode SequenceDecode Util
$(DBE)test-clip-encode: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

OBJS_BASE=	VideoContext Clip ClipDecode OutputContext Timebase \
			Sequence LinkedListAPI SequenceEncode SequenceDecode \
			Util
$(DBE)test-sequence-encode: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

OBJS_BASE=Sequence LinkedListAPI Clip Util VideoContext Timebase \
			OutputContext SequenceEncode SequenceDecode ClipDecode
$(DBE)random-splice: $$(call EXE_OBJS,$$@,$(OBJS_BASE))
	$(LINK_EXE)

# $(1) = name of exe
# $(2) = the list of basename object files that the executable needs to run, without .o
define EXE_OBJS
	$(patsubst %, %.o, $(1)) $(patsubst %, $(BIN_DIR)/%.o, $(2))
endef