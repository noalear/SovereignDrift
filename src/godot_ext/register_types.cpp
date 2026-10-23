/* register_types.cpp -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
   GDExtension entry point: registers the SFProgram node. */
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "sf_program_node.h"

using namespace godot;

static void initialize_sfport(ModuleInitializationLevel level)
{
  if (level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
  GDREGISTER_CLASS(SFProgram);
}

static void uninitialize_sfport(ModuleInitializationLevel level) { (void)level; }

extern "C" {
GDExtensionBool GDE_EXPORT sfport_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                               const GDExtensionClassLibraryPtr library,
                                               GDExtensionInitialization *initialization)
{
  GDExtensionBinding::InitObject init_obj(get_proc_address, library, initialization);
  init_obj.register_initializer(initialize_sfport);
  init_obj.register_terminator(uninitialize_sfport);
  init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
  return init_obj.init();
}
}
