#pragma once



#include "UEPyModule.h"

PyObject *py_ue_static_mesh_get_bounds(ue_PyUObject *self, PyObject * args);
PyObject *py_ue_static_mesh_get_bounding_box(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_get_num_lod(ue_PyUObject *, PyObject *);
#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
PyObject *py_ue_static_mesh_is_inside_occ(ue_PyUObject *, PyObject *); 
#endif

#if WITH_EDITOR
PyObject *py_ue_static_mesh_build(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_create_body_setup(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_get_raw_mesh(ue_PyUObject *, PyObject *);

PyObject *py_ue_static_mesh_generate_kdop10x(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_generate_kdop10y(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_generate_kdop10z(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_generate_kdop18(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_generate_kdop26(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_import_lod(ue_PyUObject *, PyObject *);

PyObject *py_ue_static_mesh_get_num_vertices(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_get_num_triangles(ue_PyUObject *, PyObject *);
PyObject *py_ue_static_mesh_get_num_uv_channels(ue_PyUObject *, PyObject *);
PyObject* py_ue_static_mesh_get_num_sections(ue_PyUObject*, PyObject*);
PyObject* py_ue_static_mesh_get_section_num_triangles(ue_PyUObject*, PyObject*);
PyObject* py_ue_static_mesh_get_lod_screen_size(ue_PyUObject*, PyObject*);
#endif
