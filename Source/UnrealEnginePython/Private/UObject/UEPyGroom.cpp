#include "UEPyGroom.h"
#include "GroomAsset.h"


PyObject *py_ue_groom_get_num_group(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	
	UGroomAsset *groom = ue_py_check_type<UGroomAsset>(self);
	if (!groom)
		return PyErr_Format(PyExc_Exception, "uobject is not a UGroomAsset");

	return PyLong_FromLong(groom->GetHairGroupsLOD().Num());
}

PyObject* py_ue_groom_get_num_lod(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	UGroomAsset *groom = ue_py_check_type<UGroomAsset>(self);
	if (!groom)
		return PyErr_Format(PyExc_Exception, "uobject is not a UGroomAsset");

	int32 grounp_index;
	if (!PyArg_ParseTuple(args, "i:groom_get_num_lod", &grounp_index))
		return nullptr;

	if (grounp_index >= groom->GetHairGroupsLOD().Num())
		return PyErr_Format(PyExc_Exception, "grounp_index out of range");

	return PyLong_FromLong(groom->GetHairGroupsLOD()[grounp_index].LODs.Num());
}

PyObject* py_ue_groom_is_simulation_enable(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	UGroomAsset *groom = ue_py_check_type<UGroomAsset>(self);
	if (!groom)
		return PyErr_Format(PyExc_Exception, "uobject is not a UGroomAsset");

	int32 grounp_index;
	int32 lod_index;
	if (!PyArg_ParseTuple(args, "ii:groom_get_num_lod", &grounp_index, &lod_index))
		return nullptr;

	bool is_enable = groom->IsSimulationEnable(grounp_index, lod_index);
	return PyBool_FromLong(is_enable);
}

PyObject* py_ue_groom_is_visible(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	UGroomAsset *groom = ue_py_check_type<UGroomAsset>(self);
	if (!groom)
		return PyErr_Format(PyExc_Exception, "uobject is not a UGroomAsset");

	int32 grounp_index;
	int32 lod_index;
	if (!PyArg_ParseTuple(args, "ii:groom_get_num_lod", &grounp_index, &lod_index))
		return nullptr;

	bool is_visible = groom->IsVisible(grounp_index, lod_index);
	return PyBool_FromLong(is_visible);
}

PyObject* py_ue_groom_get_geometry_type(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	UGroomAsset *groom = ue_py_check_type<UGroomAsset>(self);
	if (!groom)
		return PyErr_Format(PyExc_Exception, "uobject is not a UGroomAsset");

	int32 grounp_index;
	int32 lod_index;
	if (!PyArg_ParseTuple(args, "ii:groom_get_geometry_type", &grounp_index, &lod_index))
		return nullptr;

	EGroomGeometryType GroomGeometry = groom->GetGeometryType(grounp_index, lod_index);
	return PyLong_FromLong(static_cast<uint8>(GroomGeometry));
}