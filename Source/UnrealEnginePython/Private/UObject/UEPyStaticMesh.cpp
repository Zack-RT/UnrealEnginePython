#include "UEPyStaticMesh.h"
#include "Engine/StaticMesh.h"

PyObject *py_ue_static_mesh_get_bounds(ue_PyUObject *self, PyObject * args)
{
    ue_py_check(self);
    UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
    if (!mesh)
        return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

    FBoxSphereBounds bounds = mesh->GetBounds();
    UScriptStruct *u_struct = FindFirstObjectSafe<UScriptStruct>(UTF8_TO_TCHAR("BoxSphereBounds"));
    if (!u_struct)
    {
        return PyErr_Format(PyExc_Exception, "unable to get BoxSphereBounds struct");
    }
    return py_ue_new_owned_uscriptstruct(u_struct, (uint8 *)&bounds);
}

PyObject *py_ue_static_mesh_get_bounding_box(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	FBox bbox = mesh->GetBoundingBox();
	UScriptStruct *u_struct = FindFirstObjectSafe<UScriptStruct>(UTF8_TO_TCHAR("Box"));
	if (!u_struct)
	{
		return PyErr_Format(PyExc_Exception, "unable to get Box struct");
	}
	return py_ue_new_owned_uscriptstruct(u_struct, (uint8 *)&bbox);
}

PyObject *py_ue_static_mesh_get_num_lod(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");
	return PyLong_FromLong(mesh->GetNumLODs());
}


#if WITH_EDITOR

#include "Wrappers/UEPyFRawMesh.h"
#include "Editor/UnrealEd/Private/GeomFitUtils.h"
#include "FbxMeshUtils.h"

static PyObject *generate_kdop(ue_PyUObject *self, const FVector *directions, uint32 num_directions)
{
	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	TArray<FVector> DirArray;
	for (uint32 i = 0; i < num_directions; i++)
	{
		DirArray.Add(directions[i]);
	}

	if (GenerateKDopAsSimpleCollision(mesh, DirArray) == INDEX_NONE)
	{
		return PyErr_Format(PyExc_Exception, "unable to generate KDop vectors");
	}

	PyObject *py_list = PyList_New(0);
	for (FVector v : DirArray)
	{
		PyList_Append(py_list, py_ue_new_fvector(v));
	}
	return py_list;
}

PyObject *py_ue_static_mesh_generate_kdop10x(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	return generate_kdop(self, KDopDir10X, 10);
}

PyObject *py_ue_static_mesh_generate_kdop10y(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	return generate_kdop(self, KDopDir10Y, 10);
}

PyObject *py_ue_static_mesh_generate_kdop10z(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	return generate_kdop(self, KDopDir10Z, 10);
}

PyObject *py_ue_static_mesh_generate_kdop18(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	return generate_kdop(self, KDopDir18, 18);
}

PyObject *py_ue_static_mesh_generate_kdop26(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	return generate_kdop(self, KDopDir26, 26);
}

PyObject *py_ue_static_mesh_build(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 13)
	mesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
#endif
	mesh->Build();

	Py_RETURN_NONE;
}

PyObject *py_ue_static_mesh_create_body_setup(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	mesh->CreateBodySetup();

	Py_RETURN_NONE;
}

PyObject *py_ue_static_mesh_get_raw_mesh(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	int lod_index = 0;
	if (!PyArg_ParseTuple(args, "|i:get_raw_mesh", &lod_index))
		return nullptr;

	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	FRawMesh raw_mesh;

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 24)
	if (lod_index < 0 || lod_index >= mesh->GetSourceModels().Num())
		return PyErr_Format(PyExc_Exception, "invalid LOD index");

	mesh->GetSourceModel(lod_index).RawMeshBulkData->LoadRawMesh(raw_mesh);
#else
	if (lod_index < 0 || lod_index >= mesh->SourceModels.Num())
		return PyErr_Format(PyExc_Exception, "invalid LOD index");

	mesh->SourceModels[lod_index].RawMeshBulkData->LoadRawMesh(raw_mesh);
#endif

	return py_ue_new_fraw_mesh(raw_mesh);
}

PyObject *py_ue_static_mesh_import_lod(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	char *filename;
	int lod_level;
	if (!PyArg_ParseTuple(args, "si:static_mesh_import_lod", &filename, &lod_level))
		return nullptr;

	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
	if (FbxMeshUtils::ImportStaticMeshLOD(mesh, FString(UTF8_TO_TCHAR(filename)), lod_level, true))
#else
	if (FbxMeshUtils::ImportStaticMeshLOD(mesh, FString(UTF8_TO_TCHAR(filename)), lod_level))
#endif
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* py_ue_static_mesh_get_num_vertices(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	int32 lod_level;
	if (!PyArg_ParseTuple(args, "i:get_num_vertices", &lod_level))
		return nullptr;

	UStaticMesh* mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	int32 num_vertices = mesh->GetNumVertices(lod_level);

	return PyLong_FromLong(num_vertices);
}

PyObject *py_ue_static_mesh_get_num_triangles(ue_PyUObject *self, PyObject *args)
{
	ue_py_check(self);

	int32 lod_level;
	if (!PyArg_ParseTuple(args, "i:get_num_triangles", &lod_level))
		return nullptr;

	UStaticMesh *mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");
#if ENGINE_MAJOR_VERSION >= 5
	int32 num_triangles = mesh->GetRenderData()->LODResources[lod_level].GetNumTriangles();
#else
	int32 num_triangles = mesh->RenderData.Get()->LODResources[lod_level].GetNumTriangles();
#endif

	return PyLong_FromLong(num_triangles);
}

PyObject* py_ue_static_mesh_get_num_uv_channels(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	int32 lod_level;
	if (!PyArg_ParseTuple(args, "i:get_num_uv_channels", &lod_level))
		return nullptr;

	UStaticMesh* mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	int32 num_uv_channels = mesh->GetNumUVChannels(lod_level);

	return PyLong_FromLong(num_uv_channels);
}

PyObject* py_ue_static_mesh_get_num_sections(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	int32 lod_idx = 0;
	if (!PyArg_ParseTuple(args, "i:get_num_uv_channels", &lod_idx))
		return nullptr;

	UStaticMesh* mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	if (lod_idx < 0 || lod_idx >= mesh->GetSourceModels().Num())
		return PyErr_Format(PyExc_Exception, "invalid LOD index");

	const FStaticMeshLODResources& LOD = mesh->GetRenderData()->LODResources[lod_idx];
	int32 num_sections = LOD.Sections.Num();

	return PyLong_FromLong(num_sections);
}

PyObject* py_ue_static_mesh_get_section_num_triangles(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	int32 lod_idx = 0;
	int32 section_idx = 0;
	if (!PyArg_ParseTuple(args, "ii:get_section_num_triangles", &lod_idx, &section_idx))
		return nullptr;

	UStaticMesh* mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	if (lod_idx < 0 || lod_idx >= mesh->GetSourceModels().Num())
		return PyErr_Format(PyExc_Exception, "invalid LOD index");

	const FStaticMeshLODResources& LOD = mesh->GetRenderData()->LODResources[lod_idx];

	if (section_idx < 0 || section_idx >= LOD.Sections.Num())
		return PyErr_Format(PyExc_Exception, "invalid Section index");

	int32 num_triangles = LOD.Sections[section_idx].NumTriangles;

	return PyLong_FromLong(num_triangles);
}

// 参考FLevelOfDetailSettingsLayout::GetLODScreenSize
PyObject* py_ue_static_mesh_get_lod_screen_size(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	int32 lod_idx = 0;
	if (!PyArg_ParseTuple(args, "i:get_lod_screen_size", &lod_idx))
		return nullptr;
	
	UStaticMesh* mesh = ue_py_check_type<UStaticMesh>(self);
	if (!mesh)
		return PyErr_Format(PyExc_Exception, "uobject is not a UStaticMesh");

	if (lod_idx < 0 || lod_idx >= mesh->GetSourceModels().Num())
		return PyErr_Format(PyExc_Exception, "invalid LOD index");

	double ScreenSize = 0.f;
	if(mesh->bAutoComputeLODScreenSize && mesh->GetRenderData())
	{
		ScreenSize = mesh->GetRenderData()->ScreenSize[lod_idx].Default;
	}
	else if(mesh->IsSourceModelValid(lod_idx))
	{
		ScreenSize = mesh->GetSourceModel(lod_idx).ScreenSize.Default;
	}

	return PyFloat_FromDouble(ScreenSize);
}

#endif
