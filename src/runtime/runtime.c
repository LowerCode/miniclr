#include "runtime/runtime.h"
#include "pe/metadata.h"
#include "pe/metadata_int.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

static List *modules;

int Runtime_Initialize(void) {

  modules = List_Create();

  return 0;
}

int Runtime_LoadAssembly(PEInfo *info) {
  AssemblyInformation *a_info = p_malloc(sizeof(AssemblyInformation));

  a_info->reference_cnt = Metadata_GetItemCount(info, MetadataType_AssemblyRef);
  a_info->type_cnt = Metadata_GetItemCount(info, MetadataType_TypeDef);
  a_info->compiled_mthd_cnt =
      Metadata_GetItemCount(info, MetadataType_MethodDef);
  a_info->info = info;

  a_info->references = p_malloc(sizeof(uint32_t) * a_info->reference_cnt);
  a_info->types = p_malloc(sizeof(TypeInformation) * a_info->type_cnt);
  a_info->compiled_mthds =
      p_malloc(sizeof(uint64_t) * a_info->compiled_mthd_cnt);

  memset(a_info->references, 0, sizeof(uint32_t) * a_info->reference_cnt);
  memset(a_info->types, 0, sizeof(TypeInformation) * a_info->type_cnt);
  memset(a_info->compiled_mthds, 0,
         sizeof(uint64_t) * a_info->compiled_mthd_cnt);

  // setup list of references, requesting load for any references that are
  // not present
  // Find the references in the reference list and specify the indices.
  for (int i = 0; i < a_info->reference_cnt; i++) {
    MD_AssemblyRef assem_ref;
    Metadata_GetObject(
        info, Metadata_BuildToken(MetadataType_AssemblyRef, i + 1), &assem_ref);

    PEInfo *ref_info = p_malloc(sizeof(PEInfo));
    if (Platform_LoadAssembly(Metadata_GetString(info, assem_ref.name),
                              ref_info) != 0) {
      printf("ERROR: Can't find referenced assembly.\r\n");
      exit(0);
    }
    a_info->references[i] = Runtime_LoadAssembly(ref_info);
  }
  List_AddEntry(modules, a_info);
  int assembly_idx = List_Length(modules) - 1;

  // Setup static methods
  for (int i = 0; i < a_info->compiled_mthd_cnt; i++) {
    a_info->compiled_mthds[i] = 0;
  }

  // Build type vtables.
  for (int i = 0; i < a_info->type_cnt; i++) {
    Runtime_BuildVTable(assembly_idx,
                        Metadata_BuildToken(MetadataType_TypeDef, i + 1),
                        &a_info->types[i]);
  }

  return assembly_idx;
}

int Runtime_ResolveTypeRef(uint32_t token, int *assembly_idx,
                           uint32_t *assembly_tkn) {
  return 0;
}

int Runtime_BuildVTable(int assembly_idx, uint32_t token,
                        TypeInformation *type) {

  AssemblyInformation *assem = List_EntryAt(modules, assembly_idx);

  // Determine the number and range of fields
  // Determine the number and range of methods
  int field_btm = Metadata_GetItemCount(assem->info, MetadataType_Field);
  int mthd_btm = Metadata_GetItemCount(assem->info, MetadataType_MethodDef);

  MD_TypeDef tdef, ndef;
  Metadata_GetObject(assem->info, token, &tdef);
  if (Metadata_GetObject(assem->info, token + 1, &ndef) != -2) {
    field_btm = ndef.fieldList;
    mthd_btm = ndef.methodList;
  }

  type->flags = tdef.flags;

  // Build the parent type chain.
  if (Metadata_GetType(tdef.extends) == MetadataType_TypeDef &&
      Metadata_GetItemIndex(token) < Metadata_GetItemIndex(tdef.extends)) {
    Runtime_BuildVTable(assembly_idx, tdef.extends,
                        &assem->types[Metadata_GetItemIndex(tdef.extends) - 1]);
  } else if (Metadata_GetType(tdef.extends) == MetadataType_TypeRef) {
    // Resolve the type reference and build it's vtable if not done already.
  }

  // Identify implemented interfaces and setup vtable with interface offsets

  // Determine which methods implement interfaces and insert them into the
  // associated slots

  // Determine which methods override abstract/virtual methods, and put them
  // into their slots

  // Append remaining methods to the end of the vtable

  // Determine generic parameters
  // Determine class nesting
  return 0;
}

int Runtime_GenerateCode(int assembly_idx, uint32_t token,
                         uint64_t *code_addr) {
  AssemblyInformation *assem = List_EntryAt(modules, assembly_idx);
  MD_MethodDef mdef;

  Metadata_GetObject(assem->info, token, &mdef);
  // Build a list of parameters
  // First parameter is the instance reference for non-static methods
  // TODO: Need to build vtables and field tables first.
  // TODO: vtables are static
  // TODO: fields are however, not
  // TODO: track offsets for inherited interfaces

  if (mdef.implFlags & MethodImplAttributes_InternalCall) {
    printf("Internal Call\r\n");
    // TODO: Lookup internal call by class name and function name
    return 0;
  }

  // TODO: Parse exception data

  // TODO: Translate instruction sequences using lookup tables.
  // TODO: Attempt to inline short functions
  // TODO: Add built-in function recognized by the JIT to emit specified
  // instruction sequences

  return 0;
}

int Runtime_CallMethodByTokenAssemblyIndex(int assembly_idx, uint32_t token) {

  // Generate the method's code if it hasn't been generated
  AssemblyInformation *assem = List_EntryAt(modules, assembly_idx);
  uint32_t idx = Metadata_GetItemIndex(token);

  if (assem->compiled_mthds[idx] == NULL) {
    if (Runtime_GenerateCode(assembly_idx, token, NULL) != 0)
      return -1;
  }

  // Call the method
  // TODO: this can only happen once the calling convention is figured out.

  return 0;
}

int Runtime_CallMethodByToken(const char *assembly_name, uint32_t token) {
  // Resolve the assembly and call 'CallMethodByTokenAssemblyIndex'
  return 0;
}
int Runtime_CallMethodByName(const char *assembly_name,
                             const char *method_name) {
  // Resolve the assembly and function and call 'CallMethodByTokenAssemblyIndex'
  uint64_t assem_idx = -1;
  AssemblyInformation *assem = NULL;

  List_Lock(modules);
  uint64_t len = List_Length(modules);
  for (uint64_t i = 0; i < len; i++) {
    assem = List_EntryAt(modules, i);
    MD_Assembly assem_info;
    Metadata_GetObject(assem->info,
                       Metadata_BuildToken(MetadataType_Assembly, 1),
                       &assem_info);

    if (strcmp(Metadata_GetString(assem->info, assem_info.name),
               assembly_name) == 0) {
      assem_idx = i;
      break;
    }
  }
  List_Unlock(modules);

  if (assem_idx == -1)
    return -1;

  if (assem == NULL)
    return -1;

  for (uint64_t i = 0;
       i < Metadata_GetItemCount(assem->info, MetadataType_MethodDef); i++) {
    MD_MethodDef mthd_def;
    uint32_t tkn = Metadata_BuildToken(MetadataType_MethodDef, i + 1);
    Metadata_GetObject(assem->info, tkn, &mthd_def);

    if (strcmp(Metadata_GetString(assem->info, mthd_def.name), method_name) ==
        0) {
      return Runtime_CallMethodByTokenAssemblyIndex(assem_idx, tkn);
    }
  }

  return -1;
}