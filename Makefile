# Stellar Frontier port -- native build of the original program.
# Added 2026-10-12.  Produces static libraries and test executables; the
# Godot GDExtension (godot/) links the same libraries.
#
#   make                 # Linux host build into build/linux
#   make HOST=mingw      # Windows (x86_64) cross build into build/mingw
#
# Flags that reproduce the original Visual C++ build semantics:
#   -funsigned-char      MSVC -J (all original builds)
#   -fwrapv              signed overflow wraps, as with MSVC
#   -fno-strict-aliasing the 1997 code type-puns freely
#   -DS_DEBUGMEM_OFF     as in the original S_CC_OPT
#   -DDUMB_ARRAY         portcpp only, as in the original build.bat

HOST ?= linux
# The original programs are 32-bit code: they keep object pointers in 32-bit
# integers (ModuleClass::workPar[], ObjectUnion::objLong, order parameters).
# They are therefore always built for a 32-bit x86 target (ILP32, as Win32);
# the 64-bit Godot application runs each one as a child process (src/ipc).
ifeq ($(HOST),mingw)
  CXX      := i686-w64-mingw32-g++-posix
  CC       := i686-w64-mingw32-gcc-posix
  AR       := i686-w64-mingw32-ar
  EXE      := .exe
  ARCH     := -march=i686
  HOSTLIBS := -lws2_32 -static -lwinpthread
else
  CXX      := g++
  CC       := gcc
  AR       := ar
  EXE      :=
  ARCH     := -m32 -march=i686
  HOSTLIBS := -lpthread
endif

OPT      ?= -O2 -g
BUILD    := build/$(HOST)
SRC      := src
COMMON   := $(ARCH) -U_FORTIFY_SOURCE -DSFHOST -DS_DEBUGMEM_OFF -funsigned-char -fwrapv -fno-strict-aliasing \
            -std=gnu++17 -fpermissive -w $(OPT)
INCS     := -I$(SRC)/w32emu -I$(SRC)/host -I$(SRC)/slib -I$(SRC)/game/gui -I$(SRC)/game/portcpp
CXXFLAGS := $(COMMON) $(INCS) $(EXTRA) -MMD -MP

# slib classes the program uses (tools, tree views, toolbars, notebooks and
# other unused widgets are left out).
SLIB_NAMES := S_AudioDevice S_Bitmap S_BitmapData S_Button S_Canvas S_CanvasBuffer \
  S_CheckBox S_Client S_ComboBox S_DBRecord S_Database S_DebugMem S_Dialog S_DirList \
  S_Directory S_Event S_FileClient S_FileServer S_Font S_GBmpListBox S_GButton \
  S_GConsole S_GFont S_GListBox S_GProgressBar S_GScrollBar S_GSlider S_GTextBox \
  S_GWidget S_GWindow S_GroupBox S_IRCClient S_ListBox S_Logger S_MD5 S_Menu S_Midi \
  S_MsgBox S_Mutex S_OS S_Object S_Packet S_Palette S_PixelBuffer S_Process S_Record \
  S_RegCode S_Root S_ScrollBar S_SecureFileClient S_SecureFileServer S_Server \
  S_StaticText S_StdWindow S_StopWatch S_String S_StringArray S_SystemBitmap \
  S_SystemIcon S_TextBox S_TextEdit S_Thread S_Timer S_TokenArray S_Tree S_WaveData \
  S_Window S_ZArchive S_ZipArchive S_PortStubs

PORTCPP_NAMES := classes construct enginepp sound trek
GUI_MAINS     := sf server nexus client spawn stop log spy upgrade dbclient reg
GUI_NAMES     := $(basename $(notdir $(wildcard $(SRC)/game/gui/SF_*.cpp)))
W32_NAMES     := $(basename $(notdir $(wildcard $(SRC)/w32emu/*.cpp)))
HOST_NAMES    := $(basename $(notdir $(wildcard $(SRC)/host/*.cpp)))
ZLIB_NAMES    := adler32 compress crc32 deflate inffast inflate inftrees trees uncompr zutil

SLIB_OBJS    := $(addprefix $(BUILD)/slib/,$(addsuffix .o,$(SLIB_NAMES)))
PORTCPP_OBJS := $(addprefix $(BUILD)/portcpp/,$(addsuffix .o,$(PORTCPP_NAMES)))
GUI_OBJS     := $(addprefix $(BUILD)/gui/,$(addsuffix .o,$(GUI_NAMES)))
W32_OBJS     := $(addprefix $(BUILD)/w32emu/,$(addsuffix .o,$(W32_NAMES)))
HOST_OBJS    := $(addprefix $(BUILD)/host/,$(addsuffix .o,$(HOST_NAMES)))
ZLIB_OBJS    := $(addprefix $(BUILD)/zlib/,$(addsuffix .o,$(ZLIB_NAMES)))

LIBS := $(BUILD)/libsfgame.a $(BUILD)/libportcpp.a $(BUILD)/libslib.a $(BUILD)/libw32emu.a

.PHONY: all libs clean preview sfcore extension
all: libs $(BUILD)/sf$(EXE) $(BUILD)/server$(EXE) $(BUILD)/nexus$(EXE) $(BUILD)/zap$(EXE)
libs: $(LIBS)

$(BUILD)/libslib.a: $(SLIB_OBJS)
	@rm -f $@; $(AR) rcs $@ $^
$(BUILD)/libportcpp.a: $(PORTCPP_OBJS)
	@rm -f $@; $(AR) rcs $@ $^
$(BUILD)/libsfgame.a: $(GUI_OBJS)
	@rm -f $@; $(AR) rcs $@ $^
$(BUILD)/libw32emu.a: $(W32_OBJS) $(HOST_OBJS) $(ZLIB_OBJS)
	@rm -f $@; $(AR) rcs $@ $^

$(BUILD)/slib/%.o: $(SRC)/slib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILD)/portcpp/%.o: $(SRC)/game/portcpp/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DDUMB_ARRAY -c $< -o $@
$(BUILD)/gui/%.o: $(SRC)/game/gui/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(if $(filter $*,$(GUI_MAINS)),-Dmain=sf_program_main) -c $< -o $@
$(BUILD)/w32emu/%.o: $(SRC)/w32emu/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILD)/host/%.o: $(SRC)/host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(filter-out -DSFHOST,$(CXXFLAGS)) -Ithird_party/zlib -c $< -o $@
$(BUILD)/zlib/%.o: third_party/zlib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ARCH) -O2 -DZ_PREFIX -Ithird_party/zlib -c $< -o $@

# Native test executables: the original main() runs on a "game thread",
# the headless host (src/host/headless_main.cpp) plays the part of Godot.
$(BUILD)/%$(EXE): $(BUILD)/gui/%.o $(LIBS) $(BUILD)/headless/headless_main.o
	$(CXX) $(ARCH) -o $@ $(BUILD)/headless/headless_main.o $< -Wl,--start-group $(LIBS) -Wl,--end-group $(HOSTLIBS)

$(BUILD)/headless/headless_main.o: $(SRC)/headless/headless_main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(filter-out -DSFHOST,$(CXXFLAGS)) -c $< -o $@

# Windows preview host (src/winhost): a native window around the original
# program, used until the Godot application is ready.
WINHOST_LIBS := -lwinmm -lgdi32 -luser32 -lshell32 -mwindows
$(BUILD)/winhost/%.o: $(SRC)/winhost/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(filter-out -DSFHOST,$(CXXFLAGS)) -c $< -o $@
$(BUILD)/preview/frontier.exe: $(BUILD)/gui/sf.o $(LIBS) $(BUILD)/winhost/winhost_main.o
	@mkdir -p $(dir $@)
	$(CXX) $(ARCH) -o $@ $(BUILD)/winhost/winhost_main.o $< -Wl,--start-group $(LIBS) -Wl,--end-group $(WINHOST_LIBS) $(HOSTLIBS)
preview: $(BUILD)/preview/frontier.exe

# Programs as child processes of the Godot application (src/ipc):
# build/<host>/sfcore/{frontier,server,nexus}.exe, 32-bit.
IPC_CXXFLAGS = $(filter-out -DSFHOST,$(CXXFLAGS)) -I$(SRC)/ipc
ifeq ($(HOST),mingw)
  IPC_LIBS := -lwinmm
else
  IPC_LIBS := -lrt
endif
$(BUILD)/ipc/sf_ipc_os.o: $(SRC)/ipc/sf_ipc_os.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(IPC_CXXFLAGS) -c $< -o $@
$(BUILD)/ipc/child_frontier.o: $(SRC)/ipc/ipc_child_main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(IPC_CXXFLAGS) -DSF_EXE_NAME='"frontier.exe"' -DSF_ZAP_NAME='"frontier.zap"' -DSF_ICON_NAME='"frontier.ico"' -c $< -o $@
$(BUILD)/ipc/child_server.o: $(SRC)/ipc/ipc_child_main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(IPC_CXXFLAGS) -DSF_EXE_NAME='"server.exe"' -DSF_ZAP_NAME='"server.zap"' -DSF_ICON_NAME='"server.ico"' -c $< -o $@
$(BUILD)/ipc/child_nexus.o: $(SRC)/ipc/ipc_child_main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(IPC_CXXFLAGS) -DSF_EXE_NAME='"nexus.exe"' -DSF_ZAP_NAME='"nexus.zap"' -DSF_ICON_NAME='"server.ico"' -c $< -o $@
$(BUILD)/sfcore/frontier$(EXE): $(BUILD)/gui/sf.o $(BUILD)/ipc/child_frontier.o $(BUILD)/ipc/sf_ipc_os.o $(LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(ARCH) -o $@ $(BUILD)/ipc/child_frontier.o $(BUILD)/ipc/sf_ipc_os.o $< -Wl,--start-group $(LIBS) -Wl,--end-group $(IPC_LIBS) $(HOSTLIBS)
$(BUILD)/sfcore/server$(EXE): $(BUILD)/gui/server.o $(BUILD)/ipc/child_server.o $(BUILD)/ipc/sf_ipc_os.o $(LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(ARCH) -o $@ $(BUILD)/ipc/child_server.o $(BUILD)/ipc/sf_ipc_os.o $< -Wl,--start-group $(LIBS) -Wl,--end-group $(IPC_LIBS) $(HOSTLIBS)
$(BUILD)/sfcore/nexus$(EXE): $(BUILD)/gui/nexus.o $(BUILD)/ipc/child_nexus.o $(BUILD)/ipc/sf_ipc_os.o $(LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(ARCH) -o $@ $(BUILD)/ipc/child_nexus.o $(BUILD)/ipc/sf_ipc_os.o $< -Wl,--start-group $(LIBS) -Wl,--end-group $(IPC_LIBS) $(HOSTLIBS)
sfcore: $(BUILD)/sfcore/frontier$(EXE) $(BUILD)/sfcore/server$(EXE) $(BUILD)/sfcore/nexus$(EXE)

# 64-bit IPC test host (Linux)
build/linux64/ipctest: $(SRC)/ipc/ipctest_main.cpp $(SRC)/ipc/sf_ipc_host.cpp $(SRC)/ipc/sf_ipc_os.cpp $(SRC)/ipc/sf_ipc.h
	@mkdir -p $(dir $@)
	g++ -O2 -g -std=gnu++17 -I$(SRC)/ipc -o $@ $(SRC)/ipc/ipctest_main.cpp $(SRC)/ipc/sf_ipc_host.cpp $(SRC)/ipc/sf_ipc_os.cpp -lrt -lpthread

# Godot extension (64-bit): godot/addons/sfport/bin
GODOT_CPP := third_party/godot-cpp
EXT_SRCS  := $(wildcard $(SRC)/godot_ext/*.cpp) $(SRC)/ipc/sf_ipc_host.cpp $(SRC)/ipc/sf_ipc_os.cpp
EXT_HDRS  := $(wildcard $(SRC)/godot_ext/*.h) $(wildcard $(SRC)/ipc/*.h) $(SRC)/host/sf_host.h
EXT_FLAGS := -std=c++17 -O2 -fno-exceptions -DTHREADS_ENABLED -DNDEBUG -DGDEXTENSION \
             -I$(GODOT_CPP)/include -I$(GODOT_CPP)/gen/include -I$(GODOT_CPP)/gdextension \
             -I$(SRC)/ipc -I$(SRC)/host -I$(SRC)/godot_ext
godot/addons/sfport/bin/libsfport.linux.x86_64.so: $(EXT_SRCS) $(EXT_HDRS) $(GODOT_CPP)/bin/libgodot-cpp.linux.template_release.x86_64.a
	@mkdir -p $(dir $@)
	g++ $(EXT_FLAGS) -DLINUX_ENABLED -DUNIX_ENABLED -fPIC -shared -o $@ $(EXT_SRCS) \
	  $(GODOT_CPP)/bin/libgodot-cpp.linux.template_release.x86_64.a -lrt -lpthread
godot/addons/sfport/bin/sfport.windows.x86_64.dll: $(EXT_SRCS) $(EXT_HDRS) $(GODOT_CPP)/bin/libgodot-cpp.windows.template_release.x86_64.a
	@mkdir -p $(dir $@)
	x86_64-w64-mingw32-g++-posix $(EXT_FLAGS) -DWINDOWS_ENABLED -shared -static -o $@ $(EXT_SRCS) \
	  $(GODOT_CPP)/bin/libgodot-cpp.windows.template_release.x86_64.a -lwinmm -Wl,--no-undefined
	x86_64-w64-mingw32-strip $@
extension: godot/addons/sfport/bin/libsfport.linux.x86_64.so godot/addons/sfport/bin/sfport.windows.x86_64.dll

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

$(BUILD)/zap$(EXE): $(SRC)/tools/zap.cpp $(LIBS)
	$(CXX) $(CXXFLAGS) -o $@ $< -Wl,--start-group $(LIBS) -Wl,--end-group $(HOSTLIBS)

clean:
	rm -rf $(BUILD)
