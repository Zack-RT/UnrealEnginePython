#include "UEPyObject.h"

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
#include "UEPyProperty.h"
#endif

#include "PythonDelegate.h"
#include "PythonFunction.h"
#include "Components/ActorComponent.h"
#include "Engine/UserDefinedEnum.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"
#include "UnrealEd.h"
#include "Runtime/Core/Public/HAL/FeedbackContextAnsi.h"

#include "Wrappers/UEPyFObjectThumbnail.h"
#endif

#include "Misc/OutputDeviceNull.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"
#include "UObject/SavePackage.h"

PyObject *py_ue_get_class(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	Py_RETURN_UOBJECT(self->ue_object->GetClass());
}

#if WITH_EDITOR
PyObject *py_ue_class_generated_by(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UClass *u_class = ue_py_check_type<UClass>(self);
	if (!u_class)
		return PyErr_Format(PyExc_Exception, "uobject is a not a UClass");

	UObject *u_object = u_class->ClassGeneratedBy;
	if (!u_object)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(u_object);
}
#endif

PyObject *py_ue_class_get_flags(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UClass *u_class = ue_py_check_type<UClass>(self);
	if (!u_class)
		return PyErr_Format(PyExc_Exception, "uobject is a not a UClass");

	return PyLong_FromUnsignedLongLong((uint64)u_class->GetClassFlags());
}

PyObject *py_ue_class_set_flags(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	uint64 flags;
	if (!PyArg_ParseTuple(args, "K:class_set_flags", &flags))
	{
		return nullptr;
	}

	UClass *u_class = ue_py_check_type<UClass>(self);
	if (!u_class)
		return PyErr_Format(PyExc_Exception, "uobject is a not a UClass");

	u_class->ClassFlags = (EClassFlags)flags;

	Py_RETURN_NONE;
}

PyObject *py_ue_get_obj_flags(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	return PyLong_FromUnsignedLongLong((uint64)self->ue_object->GetFlags());
}

PyObject *py_ue_set_obj_flags(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	uint64 flags;
	PyObject *py_reset = nullptr;
	if (!PyArg_ParseTuple(args, "K|O:set_obj_flags", &flags, &py_reset))
	{
		return nullptr;
	}

	if (py_reset && PyObject_IsTrue(py_reset))
	{
		self->ue_object->ClearFlags(self->ue_object->GetFlags());
	}

	self->ue_object->SetFlags((EObjectFlags)flags);

	Py_RETURN_NONE;
}

PyObject *py_ue_clear_obj_flags(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	uint64 flags;
	if (!PyArg_ParseTuple(args, "K:clear_obj_flags", &flags))
	{
		return nullptr;
	}

	self->ue_object->ClearFlags((EObjectFlags)flags);

	Py_RETURN_NONE;
}

PyObject *py_ue_reset_obj_flags(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	self->ue_object->ClearFlags(self->ue_object->GetFlags());

	Py_RETURN_NONE;
}

#if WITH_EDITOR
PyObject *py_ue_class_set_config_name(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *config_name;
	if (!PyArg_ParseTuple(args, "s:class_set_config_name", &config_name))
	{
		return nullptr;
	}

	UClass *u_class = ue_py_check_type<UClass>(self);
	if (!u_class)
		return PyErr_Format(PyExc_Exception, "uobject is a not a UClass");

	u_class->ClassConfigName = UTF8_TO_TCHAR(config_name);

	Py_RETURN_NONE;
}

PyObject *py_ue_class_get_config_name(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);


	UClass *u_class = ue_py_check_type<UClass>(self);
	if (!u_class)
		return PyErr_Format(PyExc_Exception, "uobject is a not a UClass");

	return PyUnicode_FromString(TCHAR_TO_UTF8(*u_class->ClassConfigName.ToString()));
}
#endif

PyObject *py_ue_get_property_struct(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:get_property_struct", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	FStructProperty *prop = CastField<FStructProperty>(f_property);
	if (!prop)
		return PyErr_Format(PyExc_Exception, "object is not a StructProperty");
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	UStructProperty *prop = Cast<UStructProperty>(u_property);
	if (!prop)
		return PyErr_Format(PyExc_Exception, "object is not a StructProperty");
#endif
	return py_ue_new_uscriptstruct(prop->Struct, prop->ContainerPtrToValuePtr<uint8>(self->ue_object));
}

PyObject *py_ue_get_super_class(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UClass *u_class = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_class = (UClass *)self->ue_object;
	}
	else
	{
		u_class = self->ue_object->GetClass();
	}

	Py_RETURN_UOBJECT(u_class->GetSuperClass());
}

PyObject *py_ue_is_native(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UClass *u_class = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_class = (UClass *)self->ue_object;
	}
	else
	{
		u_class = self->ue_object->GetClass();
	}

	if (u_class->IsNative())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject *py_ue_get_outer(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UObject *outer = self->ue_object->GetOuter();
	if (!outer)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(outer);
}

PyObject *py_ue_get_outermost(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UObject *outermost = self->ue_object->GetOutermost();
	if (!outermost)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(outermost);
}

PyObject *py_ue_conditional_begin_destroy(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	self->ue_object->ConditionalBeginDestroy();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *py_ue_is_valid(ue_PyUObject * self, PyObject * args)
{
#if ENGINE_MAJOR_VERSION == 5
	if (!self->ue_object || !self->ue_object->IsValidLowLevel() || self->ue_object->IsUnreachable())
#else
	if (!self->ue_object || !self->ue_object->IsValidLowLevel() || self->ue_object->IsPendingKillOrUnreachable())
#endif
	{
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

PyObject *py_ue_is_a(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	PyObject *obj;
	if (!PyArg_ParseTuple(args, "O:is_a", &obj))
	{
		return NULL;
	}

	if (!ue_is_pyuobject(obj))
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UObject");
	}

	ue_PyUObject *py_obj = (ue_PyUObject *)obj;

	if (self->ue_object->IsA((UClass *)py_obj->ue_object))
	{
		Py_RETURN_TRUE;
	}


	Py_RETURN_FALSE;
}

PyObject *py_ue_is_child_of(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	PyObject *obj;
	if (!PyArg_ParseTuple(args, "O:is_child_of", &obj))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UClass>())
		return PyErr_Format(PyExc_Exception, "object is not a UClass");

	if (!ue_is_pyuobject(obj))
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UObject");
	}

	ue_PyUObject *py_obj = (ue_PyUObject *)obj;

	if (!py_obj->ue_object->IsA<UClass>())
		return PyErr_Format(PyExc_Exception, "argument is not a UClass");

	UClass *parent = (UClass *)py_obj->ue_object;
	UClass *child = (UClass *)self->ue_object;

	if (child->IsChildOf(parent))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject *py_ue_post_edit_change(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

#if WITH_EDITOR
	Py_BEGIN_ALLOW_THREADS;
	self->ue_object->PostEditChange();
	Py_END_ALLOW_THREADS;
#endif
	Py_RETURN_NONE;
}

PyObject *py_ue_post_edit_change_property(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	char *prop_name = nullptr;
	int change_type = (int)EPropertyChangeType::Unspecified;

	if (!PyArg_ParseTuple(args, "s|i:post_edit_change_property", &prop_name, &change_type))
	{
		return nullptr;
	}


	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *prop = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(prop_name)));
#else
	UProperty *prop = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(prop_name)));
#endif
	if (!prop)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", prop_name);

#if WITH_EDITOR
	Py_BEGIN_ALLOW_THREADS;
	FPropertyChangedEvent changed(prop, change_type);
	self->ue_object->PostEditChangeProperty(changed);
	Py_END_ALLOW_THREADS;
#endif
	Py_RETURN_NONE;
}

PyObject *py_ue_modify(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

#if WITH_EDITOR
	Py_BEGIN_ALLOW_THREADS;
	self->ue_object->Modify();
	Py_END_ALLOW_THREADS;
#endif
	Py_RETURN_NONE;
}

PyObject *py_ue_pre_edit_change(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *prop = nullptr;
#else
	UProperty *prop = nullptr;
#endif
	char *prop_name = nullptr;

	if (!PyArg_ParseTuple(args, "|s:pre_edit_change", &prop_name))
	{
		return nullptr;
	}

	if (prop_name)
	{
		UStruct *u_struct = nullptr;

		if (self->ue_object->IsA<UStruct>())
		{
			u_struct = (UStruct *)self->ue_object;
		}
		else
		{
			u_struct = (UStruct *)self->ue_object->GetClass();
		}

		prop = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(prop_name)));
		if (!prop)
			return PyErr_Format(PyExc_Exception, "unable to find property %s", prop_name);
	}

#if WITH_EDITOR
	Py_BEGIN_ALLOW_THREADS;
	self->ue_object->PreEditChange(prop);
	Py_END_ALLOW_THREADS;
#endif
	Py_RETURN_NONE;
}


#if WITH_EDITOR
PyObject *py_ue_set_metadata(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_key;
	char *metadata_value;
	if (!PyArg_ParseTuple(args, "ss:set_metadata", &metadata_key, &metadata_value))
	{
		return NULL;
	}

	if (self->ue_object->IsA<UClass>())
	{
		UClass *u_class = (UClass *)self->ue_object;
		u_class->SetMetaData(FName(UTF8_TO_TCHAR(metadata_key)), UTF8_TO_TCHAR(metadata_value));
	}
	else if (self->ue_object->IsA<UField>())
	{
		UField *u_field = (UField *)self->ue_object;
		u_field->SetMetaData(FName(UTF8_TO_TCHAR(metadata_key)), UTF8_TO_TCHAR(metadata_value));
	}
	else
	{
		return PyErr_Format(PyExc_TypeError, "the object does not support MetaData");
	}

	Py_RETURN_NONE;
}

PyObject *py_ue_get_metadata(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_key;
	if (!PyArg_ParseTuple(args, "s:get_metadata", &metadata_key))
	{
		return NULL;
	}

	char *metadata_value = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		UClass *u_class = (UClass *)self->ue_object;
		FString value = u_class->GetMetaData(FName(UTF8_TO_TCHAR(metadata_key)));
		return PyUnicode_FromString(TCHAR_TO_UTF8(*value));
	}

	if (self->ue_object->IsA<UField>())
	{
		UField *u_field = (UField *)self->ue_object;
		FString value = u_field->GetMetaData(FName(UTF8_TO_TCHAR(metadata_key)));
		return PyUnicode_FromString(TCHAR_TO_UTF8(*value));
	}

	return PyErr_Format(PyExc_TypeError, "the object does not support MetaData");
}

// as far as I can see metadata tags are only defined for UEditorAssetLibrary UObjects
// - other UObjects/FProperties just have metadata

PyObject *py_ue_get_metadata_tag(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_tag_key;
	if (!PyArg_ParseTuple(args, "s:get_metadata_tag", &metadata_tag_key))
	{
		return nullptr;
	}

	const FString& Value = self->ue_object->GetOutermost()->GetMetaData()->GetValue(self->ue_object, UTF8_TO_TCHAR(metadata_tag_key));
	return PyUnicode_FromString(TCHAR_TO_UTF8(*Value));
}

PyObject *py_ue_has_metadata_tag(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_tag_key;
	if (!PyArg_ParseTuple(args, "s:has_metadata_tag", &metadata_tag_key))
	{
		return nullptr;
	}

	if (self->ue_object->GetOutermost()->GetMetaData()->HasValue(self->ue_object, UTF8_TO_TCHAR(metadata_tag_key)))
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject *py_ue_remove_metadata_tag(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_tag_key;
	if (!PyArg_ParseTuple(args, "s:remove_metadata_tag", &metadata_tag_key))
	{
		return nullptr;
	}

	self->ue_object->GetOutermost()->GetMetaData()->RemoveValue(self->ue_object, UTF8_TO_TCHAR(metadata_tag_key));
	Py_RETURN_NONE;
}

PyObject *py_ue_set_metadata_tag(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_tag_key;
	char *metadata_tag_value;
	if (!PyArg_ParseTuple(args, "ss:set_metadata_tag", &metadata_tag_key, &metadata_tag_value))
	{
		return nullptr;
	}

	self->ue_object->GetOutermost()->GetMetaData()->SetValue(self->ue_object, UTF8_TO_TCHAR(metadata_tag_key), UTF8_TO_TCHAR(metadata_tag_value));
	Py_RETURN_NONE;
}


PyObject *py_ue_metadata_tags(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	TMap<FName, FString> *TagsMap = self->ue_object->GetOutermost()->GetMetaData()->GetMapForObject(self->ue_object);
	if (!TagsMap)
		Py_RETURN_NONE;

	PyObject* py_list = PyList_New(0);
	for (TPair< FName, FString>& Pair : *TagsMap)
	{
		PyList_Append(py_list, PyUnicode_FromString(TCHAR_TO_UTF8(*Pair.Key.ToString())));
	}
	return py_list;
}

PyObject *py_ue_has_metadata(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *metadata_key;
	if (!PyArg_ParseTuple(args, "s:has_metadata", &metadata_key))
	{
		return NULL;
	}

	if (self->ue_object->IsA<UClass>())
	{
		UClass *u_class = (UClass *)self->ue_object;
		if (u_class->HasMetaData(FName(UTF8_TO_TCHAR(metadata_key))))
		{
			Py_RETURN_TRUE;
		}
		Py_RETURN_FALSE;
	}

	if (self->ue_object->IsA<UField>())
	{
		UField *u_field = (UField *)self->ue_object;
		if (u_field->HasMetaData(FName(UTF8_TO_TCHAR(metadata_key))))
		{
			Py_INCREF(Py_True);
			return Py_True;
		}
		Py_INCREF(Py_False);
		return Py_False;
	}

	return PyErr_Format(PyExc_TypeError, "the object does not support MetaData");
}
#endif

PyObject *py_ue_call_function(ue_PyUObject * self, PyObject * args, PyObject *kwargs)
{

	ue_py_check(self);

	UFunction *function = nullptr;

	if (PyTuple_Size(args) < 1)
	{
		return PyErr_Format(PyExc_TypeError, "this function requires at least an argument");
	}

	PyObject *func_id = PyTuple_GetItem(args, 0);

	if (PyUnicodeOrString_Check(func_id))
	{
		function = self->ue_object->FindFunction(FName(UTF8_TO_TCHAR(UEPyUnicode_AsUTF8(func_id))));
	}

	if (!function)
		return PyErr_Format(PyExc_Exception, "unable to find function");

	return py_ue_ufunction_call(function, self->ue_object, args, 1, kwargs);

}

PyObject *py_ue_find_function(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *name;
	if (!PyArg_ParseTuple(args, "s:find_function", &name))
	{
		return NULL;
	}

	UFunction *function = self->ue_object->FindFunction(FName(UTF8_TO_TCHAR(name)));
	if (!function)
	{
		Py_RETURN_NONE;
	}

	Py_RETURN_UOBJECT((UObject *)function);
}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 15)
#if WITH_EDITOR
PyObject *py_ue_can_modify(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	if (self->ue_object->CanModify())
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}
#endif
#endif

PyObject *py_ue_set_name(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *name;
	if (!PyArg_ParseTuple(args, "s:set_name", &name))
	{
		return NULL;
	}

	if (!self->ue_object->Rename(UTF8_TO_TCHAR(name), self->ue_object->GetOutermost(), REN_Test))
	{
		return PyErr_Format(PyExc_Exception, "cannot set name %s", name);
	}

	if (self->ue_object->Rename(UTF8_TO_TCHAR(name)))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject *py_ue_set_outer(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	PyObject *py_outer;
	if (!PyArg_ParseTuple(args, "O:set_outer", &py_outer))
	{
		return nullptr;
	}

	UPackage *package = ue_py_check_type<UPackage>(py_outer);
	if (!package)
		return PyErr_Format(PyExc_Exception, "argument is not a UPackage");

	if (!self->ue_object->Rename(nullptr, package, REN_Test))
	{
		return PyErr_Format(PyExc_Exception, "cannot move to package %s", TCHAR_TO_UTF8(*package->GetPathName()));
	}

	if (self->ue_object->Rename(nullptr, package))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject *py_ue_get_name(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);


	return PyUnicode_FromString(TCHAR_TO_UTF8(*(self->ue_object->GetName())));
}

PyObject *py_ue_get_display_name(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

#if WITH_EDITOR
	if (UClass *uclass = ue_py_check_type<UClass>(self))
	{
		return PyUnicode_FromString(TCHAR_TO_UTF8(*uclass->GetDisplayNameText().ToString()));
	}

	if (AActor *actor = ue_py_check_type<AActor>(self))
	{
		return PyUnicode_FromString(TCHAR_TO_UTF8(*actor->GetActorLabel()));
	}
#endif

	if (UActorComponent *component = ue_py_check_type<UActorComponent>(self))
	{
		return PyUnicode_FromString(TCHAR_TO_UTF8(*component->GetReadableName()));
	}

	return PyUnicode_FromString(TCHAR_TO_UTF8(*(self->ue_object->GetName())));
}


PyObject *py_ue_get_full_name(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);


	return PyUnicode_FromString(TCHAR_TO_UTF8(*(self->ue_object->GetFullName())));
}

PyObject *py_ue_get_path_name(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	return PyUnicode_FromString(TCHAR_TO_UTF8(*(self->ue_object->GetPathName())));
}

PyObject *py_ue_save_config(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	Py_BEGIN_ALLOW_THREADS;
	self->ue_object->SaveConfig();
	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}

PyObject *py_ue_set_property(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	PyObject *property_value;
	int index = 0;
	if (!PyArg_ParseTuple(args, "sO|i:set_property", &property_name, &property_value, &index))
	{
		return nullptr;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);


	if (!ue_py_convert_pyobject(property_value, f_property, (uint8 *)self->ue_object, index))
	{
		return PyErr_Format(PyExc_Exception, "unable to set property %s", property_name);
	}
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);


	if (!ue_py_convert_pyobject(property_value, u_property, (uint8 *)self->ue_object, index))
	{
		return PyErr_Format(PyExc_Exception, "unable to set property %s", property_name);
	}
#endif

	Py_RETURN_NONE;

}

PyObject *py_ue_set_property_flags(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	uint64 flags;
	if (!PyArg_ParseTuple(args, "sK:set_property_flags", &property_name, &flags))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);
#endif

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	f_property->SetPropertyFlags((EPropertyFlags)flags);
#else
#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 20))
	u_property->SetPropertyFlags(flags);
#else
	u_property->SetPropertyFlags((EPropertyFlags)flags);
#endif
#endif
	Py_RETURN_NONE;
}

PyObject *py_ue_add_property_flags(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	uint64 flags;
	if (!PyArg_ParseTuple(args, "sK:add_property_flags", &property_name, &flags))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);
#endif


#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	f_property->SetPropertyFlags(f_property->GetPropertyFlags() | (EPropertyFlags)flags);
#else
#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 20))
	u_property->SetPropertyFlags(u_property->GetPropertyFlags() | flags);
#else
	u_property->SetPropertyFlags(u_property->GetPropertyFlags() | (EPropertyFlags)flags);
#endif
#endif
	Py_RETURN_NONE;
}

PyObject *py_ue_get_property_flags(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:get_property_flags", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	return PyLong_FromUnsignedLong(f_property->GetPropertyFlags());
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	return PyLong_FromUnsignedLong(u_property->GetPropertyFlags());
#endif
}

PyObject *py_ue_enum_values(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	if (!self->ue_object->IsA<UEnum>())
		return PyErr_Format(PyExc_TypeError, "uobject is not a UEnum");

	UEnum *u_enum = (UEnum *)self->ue_object;
	uint8 max_enum_value = u_enum->GetMaxEnumValue();
	PyObject *ret = PyList_New(0);
	for (uint8 i = 0; i < max_enum_value; i++)
	{
		PyObject *py_long = PyLong_FromLong(i);
		PyList_Append(ret, py_long);
		Py_DECREF(py_long);
	}
	return ret;
}

PyObject *py_ue_enum_names(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	if (!self->ue_object->IsA<UEnum>())
		return PyErr_Format(PyExc_TypeError, "uobject is not a UEnum");

	UEnum *u_enum = (UEnum *)self->ue_object;
	uint8 max_enum_value = u_enum->GetMaxEnumValue();
	PyObject *ret = PyList_New(0);
	for (uint8 i = 0; i < max_enum_value; i++)
	{
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 15)
		PyObject *py_long = PyUnicode_FromString(TCHAR_TO_UTF8(*u_enum->GetNameStringByIndex(i)));
#else
		PyObject *py_long = PyUnicode_FromString(TCHAR_TO_UTF8(*u_enum->GetEnumName(i)));
#endif
		PyList_Append(ret, py_long);
		Py_DECREF(py_long);
	}
	return ret;
}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 15)
PyObject *py_ue_enum_user_defined_names(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	if (!self->ue_object->IsA<UUserDefinedEnum>())
		return PyErr_Format(PyExc_TypeError, "uobject is not a UEnum");

	UUserDefinedEnum *u_enum = (UUserDefinedEnum *)self->ue_object;
	TArray<FText> user_defined_names;
	u_enum->DisplayNameMap.GenerateValueArray(user_defined_names);
	PyObject *ret = PyList_New(0);
	for (FText text : user_defined_names)
	{
		PyObject *py_long = PyUnicode_FromString(TCHAR_TO_UTF8(*text.ToString()));
		PyList_Append(ret, py_long);
		Py_DECREF(py_long);
	}
	return ret;
}
#endif

PyObject *py_ue_properties(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

	PyObject *ret = PyList_New(0);

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	for (TFieldIterator<FProperty> PropIt(u_struct); PropIt; ++PropIt)
	{
		FProperty* property = *PropIt;
		PyObject *property_name = PyUnicode_FromString(TCHAR_TO_UTF8(*property->GetName()));
		PyList_Append(ret, property_name);
		Py_DECREF(property_name);
	}
#else
	for (TFieldIterator<UProperty> PropIt(u_struct); PropIt; ++PropIt)
	{
		UProperty* property = *PropIt;
		PyObject *property_name = PyUnicode_FromString(TCHAR_TO_UTF8(*property->GetName()));
		PyList_Append(ret, property_name);
		Py_DECREF(property_name);
	}
#endif

	return ret;
}

PyObject *py_ue_functions(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

	PyObject *ret = PyList_New(0);

	for (TFieldIterator<UFunction> FuncIt(u_struct); FuncIt; ++FuncIt)
	{
		UFunction* func = *FuncIt;
		PyObject *func_name = PyUnicode_FromString(TCHAR_TO_UTF8(*func->GetFName().ToString()));
		PyList_Append(ret, func_name);
		Py_DECREF(func_name);
	}

	return ret;

}

PyObject *py_ue_call(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *call_args;
	if (!PyArg_ParseTuple(args, "s:call", &call_args))
	{
		return nullptr;
	}

	FOutputDeviceNull od_null;
	bool success = false;
	Py_BEGIN_ALLOW_THREADS;
	success = self->ue_object->CallFunctionByNameWithArguments(UTF8_TO_TCHAR(call_args), od_null, NULL, true);
	Py_END_ALLOW_THREADS;
	if (!success)
	{
		return PyErr_Format(PyExc_Exception, "error while calling \"%s\"", call_args);
	}

	Py_RETURN_NONE;
}

PyObject *py_ue_broadcast(ue_PyUObject *self, PyObject *args)
{

	ue_py_check(self);

	Py_ssize_t args_len = PyTuple_Size(args);
	if (args_len < 1)
	{
		return PyErr_Format(PyExc_Exception, "you need to specify the event to trigger");
	}

	PyObject *py_property_name = PyTuple_GetItem(args, 0);
	if (!PyUnicodeOrString_Check(py_property_name))
	{
		return PyErr_Format(PyExc_Exception, "event name must be a unicode string");
	}

	const char *property_name = UEPyUnicode_AsUTF8(py_property_name);

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = self->ue_object->GetClass()->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find event property %s", property_name);

	if (auto casted_prop = CastField<FMulticastDelegateProperty>(f_property))
#else
	UProperty *u_property = self->ue_object->GetClass()->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find event property %s", property_name);

	if (auto casted_prop = Cast<UMulticastDelegateProperty>(u_property))
#endif
	{
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 23)
		FMulticastScriptDelegate multiscript_delegate = *casted_prop->GetMulticastDelegate(self->ue_object);
#else
		FMulticastScriptDelegate multiscript_delegate = casted_prop->GetPropertyValue_InContainer(self->ue_object);
#endif
		uint8 *parms = (uint8 *)FMemory_Alloca(casted_prop->SignatureFunction->PropertiesSize);
		FMemory::Memzero(parms, casted_prop->SignatureFunction->PropertiesSize);

		uint32 argn = 1;

		// initialize args
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
		for (TFieldIterator<FProperty> IArgs(casted_prop->SignatureFunction); IArgs && IArgs->HasAnyPropertyFlags(CPF_Parm); ++IArgs)
		{
			FProperty *prop = *IArgs;
#else
		for (TFieldIterator<UProperty> IArgs(casted_prop->SignatureFunction); IArgs && IArgs->HasAnyPropertyFlags(CPF_Parm); ++IArgs)
		{
			UProperty *prop = *IArgs;
#endif
			if (!prop->HasAnyPropertyFlags(CPF_ZeroConstructor))
			{
				prop->InitializeValue_InContainer(parms);
			}

			if ((IArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm)
			{
				if (!prop->IsInContainer(casted_prop->SignatureFunction->ParmsSize))
				{
					return PyErr_Format(PyExc_Exception, "Attempting to import func param property that's out of bounds. %s", TCHAR_TO_UTF8(*casted_prop->SignatureFunction->GetName()));
				}

				PyObject *py_arg = PyTuple_GetItem(args, argn);
				if (!py_arg)
				{
					PyErr_Clear();
#if WITH_EDITOR
					FString default_key = FString("CPP_Default_") + prop->GetName();
					FString default_key_value = casted_prop->SignatureFunction->GetMetaData(FName(*default_key));
					if (!default_key_value.IsEmpty())
					{
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 17)
						prop->ImportText_Direct(*default_key_value, prop->ContainerPtrToValuePtr<uint8>(parms), NULL, PPF_None);
#else
						prop->ImportText(*default_key_value, prop->ContainerPtrToValuePtr<uint8>(parms), PPF_Localized, NULL);
#endif
					}
#endif
				}
				else if (!ue_py_convert_pyobject(py_arg, prop, parms, 0))
				{
					return PyErr_Format(PyExc_TypeError, "unable to convert pyobject to property %s (%s)", TCHAR_TO_UTF8(*prop->GetName()), TCHAR_TO_UTF8(*prop->GetClass()->GetName()));
				}


			}

			argn++;

		}

		Py_BEGIN_ALLOW_THREADS;
		multiscript_delegate.ProcessMulticastDelegate<UObject>(parms);
		Py_END_ALLOW_THREADS;
	}
	else
	{
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
		return PyErr_Format(PyExc_Exception, "property is not a FMulticastDelegateProperty");
#else
		return PyErr_Format(PyExc_Exception, "property is not a UMulticastDelegateProperty");
#endif
	}

	Py_RETURN_NONE;
}

PyObject *py_ue_get_property(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	int index = 0;
	if (!PyArg_ParseTuple(args, "s|i:get_property", &property_name, &index))
	{
		return nullptr;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	return ue_py_convert_property(f_property, (uint8 *)self->ue_object, index);
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	return ue_py_convert_property(u_property, (uint8 *)self->ue_object, index);
#endif
}

PyObject *py_ue_get_property_array_dim(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:get_property_array_dim", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	return PyLong_FromLongLong(f_property->ArrayDim);
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	return PyLong_FromLongLong(u_property->ArrayDim);
#endif
}

#if WITH_EDITOR
PyObject *py_ue_get_thumbnail(ue_PyUObject *self, PyObject * args)
{

	PyObject *py_generate = nullptr;
	if (!PyArg_ParseTuple(args, "|O:get_thumbnail", &py_generate))
	{
		return nullptr;
	}

	ue_py_check(self);

	TArray<FName> names;
	names.Add(FName(*self->ue_object->GetFullName()));

	FThumbnailMap thumb_map;

	FObjectThumbnail *object_thumbnail = nullptr;

	if (!ThumbnailTools::ConditionallyLoadThumbnailsForObjects(names, thumb_map))
	{
		if (py_generate && PyObject_IsTrue(py_generate))
		{
			object_thumbnail = ThumbnailTools::GenerateThumbnailForObjectToSaveToDisk(self->ue_object);
		}
	}
	else
	{
		object_thumbnail = &thumb_map[names[0]];
	}

	if (!object_thumbnail)
	{
		return PyErr_Format(PyExc_Exception, "unable to retrieve thumbnail");
	}

	return py_ue_new_fobject_thumbnail(*object_thumbnail);
}

PyObject *py_ue_render_thumbnail(ue_PyUObject *self, PyObject * args)
{

	int width = ThumbnailTools::DefaultThumbnailSize;
	int height = ThumbnailTools::DefaultThumbnailSize;
	PyObject *no_flush = nullptr;
	if (!PyArg_ParseTuple(args, "|iiO:render_thumbnail", &width, height, &no_flush))
	{
		return nullptr;
	}

	ue_py_check(self);
	FObjectThumbnail object_thumbnail;
	ThumbnailTools::RenderThumbnail(self->ue_object, width, height,
		(no_flush && PyObject_IsTrue(no_flush)) ? ThumbnailTools::EThumbnailTextureFlushMode::NeverFlush : ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush,
		nullptr, &object_thumbnail);

	return py_ue_new_fobject_thumbnail(object_thumbnail);
}
#endif

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
PyObject *py_ue_get_fproperty(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:get_fproperty", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

	// my current idea is that we need to return a python wrapper round FProperty now
	// as FProperties are not UObjects
	// currently going to assume that we dont need to handle garbage collection for FProperties
	// because if I understand the rel notes right we should just use the simple c++ new
	// and delete funcs
	// but the question is it looks as tho for UObjects we have a single ue_PyUObject python
	// wrapper stored in the housekeeper which we lookup by UObject address
	// but for FProperties do we generate a python wrapper each time??
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	//Py_RETURN_UOBJECT(u_property);
	// so the above def is the following
	//ue_PyUObject *ret = ue_get_python_uobject_inc(py_uobj);\
	//if (!ret)\
	//	PyErr_Format(PyExc_Exception, "uobject is in invalid state");\
	//return (PyObject *)ret;

	Py_RETURN_FPROPERTY(f_property);
}
#else
PyObject *py_ue_get_uproperty(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:get_uproperty", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	Py_RETURN_UOBJECT(u_property);

}
#endif

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
PyObject *py_ue_get_inner(ue_PyFProperty *self, PyObject * args)
{

	// not clear if need this - this checks if the UObject is in some invalid state
	// - Im not sure FProperty has such a state - although I have seen some mentions of PendingDelete possible
	//ue_py_check(self);

	FArrayProperty *f_property = ue_py_check_ftype<FArrayProperty>(self);
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "object is not a FArrayProperty");

	FProperty* inner = f_property->Inner;
	if (!inner)
		Py_RETURN_NONE;

	Py_RETURN_FPROPERTY(inner);
}
#else
PyObject *py_ue_get_inner(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UArrayProperty *u_property = ue_py_check_type<UArrayProperty>(self);
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "object is not a UArrayProperty");

	UProperty* inner = u_property->Inner;
	if (!inner)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(inner);
}
#endif

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
PyObject *py_ue_get_key_prop(ue_PyFProperty *self, PyObject * args)
{

	// not clear if need this - this checks if the UObject is in some invalid state
	// - Im not sure FProperty has such a state - although I have seen some mentions of PendingDelete possible
	//ue_py_check(self);

	FMapProperty *f_property = ue_py_check_ftype<FMapProperty>(self);
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "object is not a FMapProperty");

	FProperty* key = f_property->KeyProp;
	if (!key)
		Py_RETURN_NONE;

	Py_RETURN_FPROPERTY(key);
}
#else
PyObject *py_ue_get_key_prop(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UMapProperty *u_property = ue_py_check_type<UMapProperty>(self);
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "object is not a UMapProperty");

	UProperty* key = u_property->KeyProp;
	if (!key)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(key);
}
#endif

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
PyObject *py_ue_get_value_prop(ue_PyFProperty *self, PyObject * args)
{

	// not clear if need this - this checks if the UObject is in some invalid state
	// - Im not sure FProperty has such a state - although I have seen some mentions of PendingDelete possible
	//ue_py_check(self);

	FMapProperty *f_property = ue_py_check_ftype<FMapProperty>(self);
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "object is not a FMapProperty");

	FProperty* value = f_property->ValueProp;
	if (!value)
		Py_RETURN_NONE;

	Py_RETURN_FPROPERTY(value);
}
#else
PyObject *py_ue_get_value_prop(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UMapProperty *u_property = ue_py_check_type<UMapProperty>(self);
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "object is not a UMapProperty");

	UProperty* value = u_property->ValueProp;
	if (!value)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(value);
}
#endif

PyObject *py_ue_has_property(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:has_property", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		Py_RETURN_FALSE;
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		Py_RETURN_FALSE;
#endif
	Py_RETURN_TRUE;
}

PyObject *py_ue_get_property_class(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *property_name;
	if (!PyArg_ParseTuple(args, "s:get_property_class", &property_name))
	{
		return NULL;
	}

	UStruct *u_struct = nullptr;

	if (self->ue_object->IsA<UClass>())
	{
		u_struct = (UStruct *)self->ue_object;
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	// this now has to be an FFieldClass - we are returning the class of the property
	// NOT the class of the contained property
	// similar to below which would return eg UIntProperty etc
	// so I think we have a problem - whereas for UObjects the class is UClass which is also
	// a UObject - for FProperties the class is FFieldClass so we need a wrap of FProperty
	// and FFieldClass - which means we need a different function for returning the
	// python wrap of FProperty and the python wrap of an FProperties class
	Py_RETURN_FFIELDCLASS(f_property->GetClass());
#else
	UProperty *u_property = u_struct->FindPropertyByName(FName(UTF8_TO_TCHAR(property_name)));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", property_name);

	Py_RETURN_UOBJECT(u_property->GetClass());
#endif

}

PyObject *py_ue_is_rooted(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	if (self->ue_object->IsRooted())
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}


PyObject *py_ue_add_to_root(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	self->ue_object->AddToRoot();

	Py_RETURN_NONE;
}

PyObject *py_ue_own(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	if (self->owned)
	{
		return PyErr_Format(PyExc_Exception, "uobject already owned");
	}

	Py_DECREF(self);

	self->owned = 1;
	FUnrealEnginePythonHouseKeeper::Get()->TrackUObject(self->ue_object);

	Py_RETURN_NONE;
}

PyObject *py_ue_disown(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	if (!self->owned)
	{
		return PyErr_Format(PyExc_Exception, "uobject not owned");
	}

	Py_INCREF(self);

	self->owned = 0;
	FUnrealEnginePythonHouseKeeper::Get()->UntrackUObject(self->ue_object);

	Py_RETURN_NONE;
}

PyObject *py_ue_is_owned(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	if (!self->owned)
	{
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

PyObject *py_ue_auto_root(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	self->ue_object->AddToRoot();
	self->auto_rooted = 1;

	Py_RETURN_NONE;
}

PyObject *py_ue_remove_from_root(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	self->ue_object->RemoveFromRoot();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *py_ue_bind_event(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *event_name;
	PyObject *py_callable;
	if (!PyArg_ParseTuple(args, "sO:bind_event", &event_name, &py_callable))
	{
		return NULL;
	}

	if (!PyCallable_Check(py_callable))
	{
		return PyErr_Format(PyExc_Exception, "object is not callable");
	}

	return ue_bind_pyevent(self, FString(event_name), py_callable, true);
}

PyObject *py_ue_unbind_event(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	char *event_name;
	PyObject *py_callable;
	if (!PyArg_ParseTuple(args, "sO:bind_event", &event_name, &py_callable))
	{
		return NULL;
	}

	if (!PyCallable_Check(py_callable))
	{
		return PyErr_Format(PyExc_Exception, "object is not callable");
	}

	return ue_unbind_pyevent(self, FString(event_name), py_callable, true);
}

PyObject *py_ue_delegate_bind_ufunction(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	char *delegate_name;
	PyObject *py_obj;
	char *fname;

	if (!PyArg_ParseTuple(args, "sOs:delegate_bind_ufunction", &delegate_name, &py_obj, &fname))
		return nullptr;

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	FProperty *f_property = self->ue_object->GetClass()->FindPropertyByName(FName(delegate_name));
	if (!f_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", delegate_name);

	FDelegateProperty *Prop = CastField<FDelegateProperty>(f_property);
	if (!Prop)
		return PyErr_Format(PyExc_Exception, "property is not a FDelegateProperty");
#else
	UProperty *u_property = self->ue_object->GetClass()->FindPropertyByName(FName(delegate_name));
	if (!u_property)
		return PyErr_Format(PyExc_Exception, "unable to find property %s", delegate_name);

	UDelegateProperty *Prop = Cast<UDelegateProperty>(u_property);
	if (!Prop)
		return PyErr_Format(PyExc_Exception, "property is not a UDelegateProperty");
#endif

	UObject *Object = ue_py_check_type<UObject>(py_obj);
	if (!Object)
		return PyErr_Format(PyExc_Exception, "argument is not a UObject");

	FScriptDelegate script_delegate;
	script_delegate.BindUFunction(Object, FName(fname));

	// re-assign multicast delegate
	Prop->SetPropertyValue_InContainer(self->ue_object, script_delegate);

	Py_RETURN_NONE;
}

#if PY_MAJOR_VERSION >= 3
PyObject *py_ue_add_function(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *name;
	PyObject *py_callable;
	if (!PyArg_ParseTuple(args, "sO:add_function", &name, &py_callable))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UClass>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
	}

	UClass *u_class = (UClass *)self->ue_object;

	if (!PyCallable_Check(py_callable))
	{
		return PyErr_Format(PyExc_Exception, "object is not callable");
	}

	if (!unreal_engine_add_function(u_class, name, py_callable, FUNC_Native | FUNC_BlueprintCallable | FUNC_Public))
	{
		return PyErr_Format(PyExc_Exception, "unable to add function");
	}

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
// so I think we have to completely rewrite this for 4.25
PyObject *py_ue_add_property(ue_PyUObject * self, PyObject * args)
{
	// so this routine appears to be dynamically adding a new property to an existing
	// UObject in Python
	// - nothing to do with accessing or setting properties - thats the ue_PyUObject getttro/setattro functions

	ue_py_check(self);

	PyObject *obj;
	char *name;
	PyObject *property_class = nullptr;
	PyObject *property_class2 = nullptr;
	if (!PyArg_ParseTuple(args, "Os|OO:add_property", &obj, &name, &property_class, &property_class2))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UStruct>())
		return PyErr_Format(PyExc_Exception, "uobject is not a UStruct");

	// so scope defines the property container subclass of container properties eg array or map 
	// for FProperties it apparently is an FFieldVariant ie either a UObject or FField
	// fproperty is the property class of non-container properties or the item property class for
	// array properties or key property class for map containers
	// fproperty2 defines the value property class for map containers

	// u_prop_class defines the UObject class of property values, the item UOBject class for array
	// properties or the key UObject class for map properties
	// u_prop_class2 defines the UObject class of value property values for map properties

	// this is not a UObject anymore but not clear what replacement is
	// this should be an FFieldVariant into which we can store either a UObject or an FField
	//UObject *scope = nullptr;
	FFieldVariant scope;
	FFieldVariant *scopeptr = nullptr;

	FProperty *f_property = nullptr;
	FFieldClass *f_class = nullptr;
	FProperty *f_property2 = nullptr;
	FFieldClass *f_class2 = nullptr;

	UClass *u_prop_class = nullptr;
	UScriptStruct *u_script_struct = nullptr;
	UClass *u_prop_class2 = nullptr;
	UScriptStruct *u_script_struct2 = nullptr;
	bool is_array = false;
	bool is_map = false;

	// so what to do here
	// this is going to need exploration
	// as these are describing the class of the property value I think these are still going
	// to be some version of UObject

	if (property_class)
	{
		if (!ue_is_pyuobject(property_class))
		{
			return PyErr_Format(PyExc_Exception, "property class arg is not a uobject");
		}
		ue_PyUObject *py_prop_class = (ue_PyUObject *)property_class;
		if (py_prop_class->ue_object->IsA<UClass>())
		{
			u_prop_class = (UClass *)py_prop_class->ue_object;
		}
		else if (py_prop_class->ue_object->IsA<UScriptStruct>())
		{
			u_script_struct = (UScriptStruct *)py_prop_class->ue_object;
		}
		else
		{
			return PyErr_Format(PyExc_Exception, "property class arg is not a UClass or a UScriptStruct");
		}
	}

	if (property_class2)
	{
		if (!ue_is_pyuobject(property_class2))
		{
			return PyErr_Format(PyExc_Exception, "secondary property class arg is not a uobject");
		}
		ue_PyUObject *py_prop_class = (ue_PyUObject *)property_class2;
		if (py_prop_class->ue_object->IsA<UClass>())
		{
			u_prop_class2 = (UClass *)py_prop_class->ue_object;
		}
		else if (py_prop_class->ue_object->IsA<UScriptStruct>())
		{
			u_script_struct2 = (UScriptStruct *)py_prop_class->ue_object;
		}
		else
		{
			return PyErr_Format(PyExc_Exception, "secondary property class arg is not a UClass or a UScriptStruct");
		}
	}

	EObjectFlags o_flags = RF_Public | RF_MarkAsNative;// | RF_Transient;

	if (ue_is_pyuobject(obj))
	{
		// complication here - a uobject can NOT be an FProperty any more
		// what I think this may be doing is seeing if the passed UObject was some form of UProperty
		// except that here it MUST be a UProperty subclass
		// so apparently this object was expected to be a UProperty subclass of some description
		// so currently think the thing to do here is to exit
		return PyErr_Format(PyExc_Exception, "object is a uobject - it must be an fproperty subclass");
		//ue_PyUObject *py_obj = (ue_PyUObject *)obj;
		//if (!py_obj->ue_object->IsA<UClass>())
		//{
		//	return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
		//}
		//u_class = (UClass *)py_obj->ue_object;
		//if (!u_class->IsChildOf<UProperty>())
		//	return PyErr_Format(PyExc_Exception, "uobject is not a UProperty");
		//if (u_class == UArrayProperty::StaticClass())
		//	return PyErr_Format(PyExc_Exception, "please use a single-item list of property for arrays");
		//scope = self->ue_object;
	}
	else if (ue_is_pyfproperty(obj))
	{
		// I think all this is doing is allowing a new property to be defined in python
		// by passing an FProperty subclass python object

		// so now think this is wrong also - we need a class here which is now a different c++ class
		// from FProperty ie FFieldClass
		return PyErr_Format(PyExc_Exception, "object is an fproperty - it must be an ffieldclass subclass");

		// so far this seems to be referring to the Property class
		//ue_PyFProperty *py_obj = (ue_PyFProperty *)obj;
		//if (!py_obj->ue_fproperty->IsA(FFieldClass))
		//{
		//	return PyErr_Format(PyExc_Exception, "fproperty is not an FFieldClass ie an FProperty subclass class object");
		//}
		//f_class = (FFieldClass *)py_obj->ue_fproperty;
		// this is sort of by definition true now
		//if (!f_class->IsChildOf(FProperty::StaticClass()))
		//	return PyErr_Format(PyExc_Exception, "fproperty is not an FProperty");
		//if (f_class == FArrayProperty::StaticClass())
		//	return PyErr_Format(PyExc_Exception, "please use a single-item list of property for arrays");
		// so scope should be the owning object we are creating the property for ie a uobject
		//scope = FFieldVariant(self->ue_object);
		//scopeptr = &scope;
	}
	else if (ue_is_pyffieldclass(obj))
	{
		// I think all this is doing is allowing a new property to be defined in python
		// by passing an FProperty subclass python object

		// so far this seems to be referring to the Property class
		ue_PyFFieldClass *py_obj = (ue_PyFFieldClass *)obj;
		// by definition this is a FFieldClass object
		//if (!py_obj->ue_ffieldclass->IsA(FFieldClass))
		//{
		//	return PyErr_Format(PyExc_Exception, "fproperty is not an FFieldClass ie an FProperty subclass class object");
		//}
		//f_class = (FFieldClass *)py_obj->ue_ffieldclass;
		f_class = py_obj->ue_ffieldclass;
		// this is sort of by definition true now
		if (!f_class->IsChildOf(FProperty::StaticClass()))
			return PyErr_Format(PyExc_Exception, "fproperty is not an FProperty");
		if (f_class == FArrayProperty::StaticClass())
			return PyErr_Format(PyExc_Exception, "please use a single-item list of property for arrays");
		// so scope should be the owning object we are creating the property for ie a uobject
		scope = FFieldVariant(self->ue_object);
		scopeptr = &scope;
	}
	else if (PyList_Check(obj))
	{
		if (PyList_Size(obj) == 1)
		{
			PyObject *py_item = PyList_GetItem(obj, 0);
			//if (ue_is_pyfproperty(py_item))
			if (ue_is_pyffieldclass(py_item))
			{
				//ue_PyFProperty *py_obj = (ue_PyFProperty *)py_item;
				//if (!py_obj->ue_fproperty->IsA(FFieldClass))
				//{
				//	return PyErr_Format(PyExc_Exception, "array property item fproperty is not an FFieldClass ie an FProperty subclass class object");
				//}
				//ue_PyFFieldClass *py_obj = (ue_PyFFieldClass *)py_item;
				//f_class = (FFieldClass *)py_obj->ue_fproperty;
				ue_PyFFieldClass *py_obj = (ue_PyFFieldClass *)py_item;
				f_class = py_obj->ue_ffieldclass;
				if (!f_class->IsChildOf(FProperty::StaticClass()))
					return PyErr_Format(PyExc_Exception, "array property item fproperty is not a FProperty");
				if (f_class == FArrayProperty::StaticClass())
					return PyErr_Format(PyExc_Exception, "please use a single-item list of property for arrays");
				FArrayProperty *f_array = new FArrayProperty(self->ue_object, UTF8_TO_TCHAR(name), o_flags);
				if (!f_array)
					return PyErr_Format(PyExc_Exception, "unable to allocate new FProperty");
				scope = FFieldVariant(f_array);
				scopeptr = &scope;
				is_array = true;
			}
			Py_DECREF(py_item);
		}
		else if (PyList_Size(obj) == 2)
		{
			PyObject *py_key = PyList_GetItem(obj, 0);
			PyObject *py_value = PyList_GetItem(obj, 1);
			//if (ue_is_pyfproperty(py_key) && ue_is_pyfproperty(py_value))
			if (ue_is_pyffieldclass(py_key) && ue_is_pyffieldclass(py_value))
			{
				// KEY
				//ue_PyFProperty *py_obj = (ue_PyFProperty *)py_key;
				//if (!py_obj->ue_fproperty->IsA(FFieldClass))
				//{
				//	return PyErr_Format(PyExc_Exception, "map property key fproperty is not an FFieldClass ie an FProperty subclass class object");
				//}
				//f_class = (FFieldClass *)py_obj->ue_fproperty;
				ue_PyFFieldClass *py_obj = (ue_PyFFieldClass *)py_key;
				f_class = py_obj->ue_ffieldclass;
				if (!f_class->IsChildOf(FProperty::StaticClass()))
					return PyErr_Format(PyExc_Exception, "map property key fproperty is not a FProperty");
				if (f_class == FArrayProperty::StaticClass())
					return PyErr_Format(PyExc_Exception, "please use a two-items list of properties for maps");

				// VALUE
				//ue_PyFProperty *py_obj2 = (ue_PyFProperty *)py_value;
				//if (!py_obj2->ue_fproperty->IsA(FFieldClass))
				//{
				//	return PyErr_Format(PyExc_Exception, "map property value fproperty is not an FFieldClass ie an FProperty subclass class object");
				//}
				//f_class2 = (FFieldClass *)py_obj2->ue_fproperty;
				ue_PyFFieldClass *py_obj2 = (ue_PyFFieldClass *)py_key;
				f_class2 = py_obj2->ue_ffieldclass;
				if (!f_class2->IsChildOf(FProperty::StaticClass()))
					return PyErr_Format(PyExc_Exception, "map property value fproperty is not a FProperty");
				if (f_class2 == FArrayProperty::StaticClass())
					return PyErr_Format(PyExc_Exception, "please use a two-items list of properties for maps");


				FMapProperty *f_map = new FMapProperty(self->ue_object, UTF8_TO_TCHAR(name), o_flags);
				if (!f_map)
					return PyErr_Format(PyExc_Exception, "unable to allocate new FProperty");
				scope = FFieldVariant(f_map);
				scopeptr = &scope;
				is_map = true;
			}
			Py_DECREF(py_key);
			Py_DECREF(py_value);
		}
	}


	if (!scope)
	{
		return PyErr_Format(PyExc_Exception, "argument is not an FProperty subclass or a single item list");
	}

	// so for NewObject the 1st argument scope is the outer uobject
	// - this does not exist for FProperties - its now the owner object
	//u_property = NewObject<UProperty>(scope, u_class, UTF8_TO_TCHAR(name), o_flags);

	// this is all wrong - not clear how to do this yet
	// but we do not allocate FProperties with NewObject
	// however we need a new function because the following is not a valid FProperty constructor
	// for the moment looks as tho need to explicitly allocate each FProperty type
	// so scope is the owner object which is apparently an FFieldVarient ie it can hold
	// either a UObject or an FField
	//f_property = new FProperty(scope, u_class, UTF8_TO_TCHAR(name), o_flags);
	f_property = FProperty_New(&scope, f_class, FName(UTF8_TO_TCHAR(name)), o_flags);
	if (!f_property)
	{
		// MarkPendingKill is a UObject feature
		// NEEDS FIXING
		// or does it - maybe taken care of by the destructor of FArrayProperty or FMapProperty
		//if (is_array || is_map)
		//	scope->MarkPendingKill();
		return PyErr_Format(PyExc_Exception, "unable to allocate new FProperty");
	}

	// one day we may want to support transient properties...
	//uint64 flags = CPF_Edit | CPF_BlueprintVisible | CPF_Transient | CPF_ZeroConstructor;
	uint64 flags = CPF_Edit | CPF_BlueprintVisible | CPF_ZeroConstructor;

	// we assumed Actor Components to be non-editable
	if (u_prop_class && u_prop_class->IsChildOf<UActorComponent>())
	{
		flags |= ~CPF_Edit;
	}

	// TODO manage replication
	/*
	if (replicate && PyObject_IsTrue(replicate)) {
		flags |= CPF_Net;
	}*/

	if (is_array)
	{
		FArrayProperty *f_array = (FArrayProperty *)scope.ToField();
		f_array->AddCppProperty(f_property);
		f_property->SetPropertyFlags((EPropertyFlags)flags);
		if (f_property->GetClass() == FObjectProperty::StaticClass())
		{
			FObjectProperty *obj_prop = (FObjectProperty *)f_property;
			if (u_prop_class)
			{
				obj_prop->SetPropertyClass(u_prop_class);
			}
		}
		if (f_property->GetClass() == FStructProperty::StaticClass())
		{
			FStructProperty *obj_prop = (FStructProperty *)f_property;
			if (u_script_struct)
			{
				obj_prop->Struct = u_script_struct;
			}
		}
		f_property = f_array;
	}

	if (is_map)
	{
		//u_property2 = NewObject<FProperty>(scope, u_class2, NAME_None, o_flags);
		f_property2 = FProperty_New(&scope, f_class2, FName(UTF8_TO_TCHAR(name)), o_flags);
		if (!f_property2)
		{
			// MarkPendingKill is a UObject feature
			// NEEDS FIXING
			// or does it - maybe taken care of by the destructor of FArrayProperty or FMapProperty
			//if (is_array || is_map)
			//	scope->MarkPendingKill();
			return PyErr_Format(PyExc_Exception, "unable to allocate new FProperty");
		}
		FMapProperty *f_map = (FMapProperty *)scope.ToField();

		f_property->SetPropertyFlags((EPropertyFlags)flags);
		f_property2->SetPropertyFlags((EPropertyFlags)flags);

		if (f_property->GetClass() == FObjectProperty::StaticClass())
		{
			FObjectProperty *obj_prop = (FObjectProperty *)f_property;
			if (u_prop_class)
			{
				obj_prop->SetPropertyClass(u_prop_class);
			}
		}
		if (f_property->GetClass() == FStructProperty::StaticClass())
		{
			FStructProperty *obj_prop = (FStructProperty *)f_property;
			if (u_script_struct)
			{
				obj_prop->Struct = u_script_struct;
			}
		}

		if (f_property2->GetClass() == FObjectProperty::StaticClass())
		{
			FObjectProperty *obj_prop = (FObjectProperty *)f_property2;
			if (u_prop_class2)
			{
				obj_prop->SetPropertyClass(u_prop_class2);
			}
		}
		if (f_property2->GetClass() == FStructProperty::StaticClass())
		{
			FStructProperty *obj_prop = (FStructProperty *)f_property2;
			if (u_script_struct2)
			{
				obj_prop->Struct = u_script_struct2;
			}
		}

		f_map->KeyProp = f_property;
		f_map->ValueProp = f_property2;

		f_property = f_map;
	}

	if (f_class == FMulticastDelegateProperty::StaticClass())
	{
		FMulticastDelegateProperty *mcp = (FMulticastDelegateProperty *)f_property;
		mcp->SignatureFunction = NewObject<UFunction>(self->ue_object, NAME_None, RF_Public | RF_Transient | RF_MarkAsNative);
		mcp->SignatureFunction->FunctionFlags = FUNC_MulticastDelegate | FUNC_BlueprintCallable | FUNC_Native;
		flags |= CPF_BlueprintAssignable | CPF_BlueprintCallable;
		flags &= ~CPF_Edit;
	}

	else if (f_class == FDelegateProperty::StaticClass())
	{
		FDelegateProperty *udp = (FDelegateProperty *)f_property;
		udp->SignatureFunction = NewObject<UFunction>(self->ue_object, NAME_None, RF_Public | RF_Transient | RF_MarkAsNative);
		udp->SignatureFunction->FunctionFlags = FUNC_MulticastDelegate | FUNC_BlueprintCallable | FUNC_Native;
		flags |= CPF_BlueprintAssignable | CPF_BlueprintCallable;
		flags &= ~CPF_Edit;
	}

	else if (f_class == FObjectProperty::StaticClass())
	{
		// ensure it is not an arry as we have already managed it !
		if (!is_array && !is_map)
		{
			FObjectProperty *obj_prop = (FObjectProperty *)f_property;
			if (u_prop_class)
			{
				obj_prop->SetPropertyClass(u_prop_class);
			}
		}
	}

	else if (f_class == FStructProperty::StaticClass())
	{
		// ensure it is not an arry as we have already managed it !
		if (!is_array && !is_map)
		{
			FStructProperty *obj_prop = (FStructProperty *)f_property;
			if (u_script_struct)
			{
				obj_prop->Struct = u_script_struct;
			}
		}
	}

	f_property->SetPropertyFlags((EPropertyFlags)flags);
	f_property->ArrayDim = 1;

	// so is this how we add properties??
	UStruct *u_struct = (UStruct *)self->ue_object;
	u_struct->AddCppProperty(f_property);
	u_struct->StaticLink(true);


	if (u_struct->IsA<UClass>())
	{
		UClass *owner_class = (UClass *)u_struct;
		owner_class->GetDefaultObject()->RemoveFromRoot();
		owner_class->GetDefaultObject()->ConditionalBeginDestroy();
		owner_class->ClassDefaultObject = nullptr;
		owner_class->GetDefaultObject()->PostInitProperties();
	}

	// TODO add default value

	Py_RETURN_FPROPERTY(f_property);
}
#else
PyObject *py_ue_add_property(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	PyObject *obj;
	char *name;
	PyObject *property_class = nullptr;
	PyObject *property_class2 = nullptr;
	if (!PyArg_ParseTuple(args, "Os|OO:add_property", &obj, &name, &property_class, &property_class2))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UStruct>())
		return PyErr_Format(PyExc_Exception, "uobject is not a UStruct");

	UObject *scope = nullptr;

	UProperty *u_property = nullptr;
	UClass *u_class = nullptr;
	UProperty *u_property2 = nullptr;
	UClass *u_class2 = nullptr;

	UClass *u_prop_class = nullptr;
	UScriptStruct *u_script_struct = nullptr;
	UClass *u_prop_class2 = nullptr;
	UScriptStruct *u_script_struct2 = nullptr;
	bool is_array = false;
	bool is_map = false;

	if (property_class)
	{
		if (!ue_is_pyuobject(property_class))
		{
			return PyErr_Format(PyExc_Exception, "property class arg is not a uobject");
		}
		ue_PyUObject *py_prop_class = (ue_PyUObject *)property_class;
		if (py_prop_class->ue_object->IsA<UClass>())
		{
			u_prop_class = (UClass *)py_prop_class->ue_object;
		}
		else if (py_prop_class->ue_object->IsA<UScriptStruct>())
		{
			u_script_struct = (UScriptStruct *)py_prop_class->ue_object;
		}
		else
		{
			return PyErr_Format(PyExc_Exception, "property class arg is not a UClass or a UScriptStruct");
		}
	}

	if (property_class2)
	{
		if (!ue_is_pyuobject(property_class2))
		{
			return PyErr_Format(PyExc_Exception, "property class arg is not a uobject");
		}
		ue_PyUObject *py_prop_class = (ue_PyUObject *)property_class2;
		if (py_prop_class->ue_object->IsA<UClass>())
		{
			u_prop_class2 = (UClass *)py_prop_class->ue_object;
		}
		else if (py_prop_class->ue_object->IsA<UScriptStruct>())
		{
			u_script_struct2 = (UScriptStruct *)py_prop_class->ue_object;
		}
		else
		{
			return PyErr_Format(PyExc_Exception, "property class arg is not a UClass or a UScriptStruct");
		}
	}

	EObjectFlags o_flags = RF_Public | RF_MarkAsNative;// | RF_Transient;

	if (ue_is_pyuobject(obj))
	{
		ue_PyUObject *py_obj = (ue_PyUObject *)obj;
		if (!py_obj->ue_object->IsA<UClass>())
		{
			return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
		}
		u_class = (UClass *)py_obj->ue_object;
		if (!u_class->IsChildOf<UProperty>())
			return PyErr_Format(PyExc_Exception, "uobject is not a UProperty");
		if (u_class == UArrayProperty::StaticClass())
			return PyErr_Format(PyExc_Exception, "please use a single-item list of property for arrays");
		scope = self->ue_object;
	}
	else if (PyList_Check(obj))
	{
		if (PyList_Size(obj) == 1)
		{
			PyObject *py_item = PyList_GetItem(obj, 0);
			if (ue_is_pyuobject(py_item))
			{
				ue_PyUObject *py_obj = (ue_PyUObject *)py_item;
				if (!py_obj->ue_object->IsA<UClass>())
				{
					return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
				}
				u_class = (UClass *)py_obj->ue_object;
				if (!u_class->IsChildOf<UProperty>())
					return PyErr_Format(PyExc_Exception, "uobject is not a UProperty");
				if (u_class == UArrayProperty::StaticClass())
					return PyErr_Format(PyExc_Exception, "please use a single-item list of property for arrays");
				UArrayProperty *u_array = NewObject<UArrayProperty>(self->ue_object, UTF8_TO_TCHAR(name), o_flags);
				if (!u_array)
					return PyErr_Format(PyExc_Exception, "unable to allocate new UProperty");
				scope = u_array;
				is_array = true;
			}
			Py_DECREF(py_item);
		}
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 15)
		else if (PyList_Size(obj) == 2)
		{
			PyObject *py_key = PyList_GetItem(obj, 0);
			PyObject *py_value = PyList_GetItem(obj, 1);
			if (ue_is_pyuobject(py_key) && ue_is_pyuobject(py_value))
			{
				// KEY
				ue_PyUObject *py_obj = (ue_PyUObject *)py_key;
				if (!py_obj->ue_object->IsA<UClass>())
				{
					return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
				}
				u_class = (UClass *)py_obj->ue_object;
				if (!u_class->IsChildOf<UProperty>())
					return PyErr_Format(PyExc_Exception, "uobject is not a UProperty");
				if (u_class == UArrayProperty::StaticClass())
					return PyErr_Format(PyExc_Exception, "please use a two-items list of properties for maps");

				// VALUE
				ue_PyUObject *py_obj2 = (ue_PyUObject *)py_value;
				if (!py_obj2->ue_object->IsA<UClass>())
				{
					return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
				}
				u_class2 = (UClass *)py_obj2->ue_object;
				if (!u_class2->IsChildOf<UProperty>())
					return PyErr_Format(PyExc_Exception, "uobject is not a UProperty");
				if (u_class2 == UArrayProperty::StaticClass())
					return PyErr_Format(PyExc_Exception, "please use a two-items list of properties for maps");


				UMapProperty *u_map = NewObject<UMapProperty>(self->ue_object, UTF8_TO_TCHAR(name), o_flags);
				if (!u_map)
					return PyErr_Format(PyExc_Exception, "unable to allocate new UProperty");
				scope = u_map;
				is_map = true;
			}
			Py_DECREF(py_key);
			Py_DECREF(py_value);
		}
#endif
	}


	if (!scope)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UObject or a single item list");
	}

	u_property = NewObject<UProperty>(scope, u_class, UTF8_TO_TCHAR(name), o_flags);
	if (!u_property)
	{
		if (is_array || is_map)
			scope->MarkPendingKill();
		return PyErr_Format(PyExc_Exception, "unable to allocate new UProperty");
	}

	// one day we may want to support transient properties...
	//uint64 flags = CPF_Edit | CPF_BlueprintVisible | CPF_Transient | CPF_ZeroConstructor;
	uint64 flags = CPF_Edit | CPF_BlueprintVisible | CPF_ZeroConstructor;

	// we assumed Actor Components to be non-editable
	if (u_prop_class && u_prop_class->IsChildOf<UActorComponent>())
	{
		flags |= ~CPF_Edit;
	}

	// TODO manage replication
	/*
	if (replicate && PyObject_IsTrue(replicate)) {
		flags |= CPF_Net;
	}*/

	if (is_array)
	{
		UArrayProperty *u_array = (UArrayProperty *)scope;
		u_array->AddCppProperty(u_property);
#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 20))
		u_property->SetPropertyFlags(flags);
#else
		u_property->SetPropertyFlags((EPropertyFlags)flags);
#endif
		if (u_property->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty *obj_prop = (UObjectProperty *)u_property;
			if (u_prop_class)
			{
				obj_prop->SetPropertyClass(u_prop_class);
			}
		}
		if (u_property->GetClass() == UStructProperty::StaticClass())
		{
			UStructProperty *obj_prop = (UStructProperty *)u_property;
			if (u_script_struct)
			{
				obj_prop->Struct = u_script_struct;
			}
		}
		u_property = u_array;
	}

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 15)
	if (is_map)
	{
		u_property2 = NewObject<UProperty>(scope, u_class2, NAME_None, o_flags);
		if (!u_property2)
		{
			if (is_array || is_map)
				scope->MarkPendingKill();
			return PyErr_Format(PyExc_Exception, "unable to allocate new UProperty");
		}
		UMapProperty *u_map = (UMapProperty *)scope;

#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 20))
		u_property->SetPropertyFlags(flags);
		u_property2->SetPropertyFlags(flags);
#else
		u_property->SetPropertyFlags((EPropertyFlags)flags);
		u_property2->SetPropertyFlags((EPropertyFlags)flags);
#endif

		if (u_property->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty *obj_prop = (UObjectProperty *)u_property;
			if (u_prop_class)
			{
				obj_prop->SetPropertyClass(u_prop_class);
			}
		}
		if (u_property->GetClass() == UStructProperty::StaticClass())
		{
			UStructProperty *obj_prop = (UStructProperty *)u_property;
			if (u_script_struct)
			{
				obj_prop->Struct = u_script_struct;
			}
		}

		if (u_property2->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty *obj_prop = (UObjectProperty *)u_property2;
			if (u_prop_class2)
			{
				obj_prop->SetPropertyClass(u_prop_class2);
			}
		}
		if (u_property2->GetClass() == UStructProperty::StaticClass())
		{
			UStructProperty *obj_prop = (UStructProperty *)u_property2;
			if (u_script_struct2)
			{
				obj_prop->Struct = u_script_struct2;
			}
		}

		u_map->KeyProp = u_property;
		u_map->ValueProp = u_property2;

		u_property = u_map;
	}
#endif

	if (u_class == UMulticastDelegateProperty::StaticClass())
	{
		UMulticastDelegateProperty *mcp = (UMulticastDelegateProperty *)u_property;
		mcp->SignatureFunction = NewObject<UFunction>(self->ue_object, NAME_None, RF_Public | RF_Transient | RF_MarkAsNative);
		mcp->SignatureFunction->FunctionFlags = FUNC_MulticastDelegate | FUNC_BlueprintCallable | FUNC_Native;
		flags |= CPF_BlueprintAssignable | CPF_BlueprintCallable;
		flags &= ~CPF_Edit;
	}

	else if (u_class == UDelegateProperty::StaticClass())
	{
		UDelegateProperty *udp = (UDelegateProperty *)u_property;
		udp->SignatureFunction = NewObject<UFunction>(self->ue_object, NAME_None, RF_Public | RF_Transient | RF_MarkAsNative);
		udp->SignatureFunction->FunctionFlags = FUNC_MulticastDelegate | FUNC_BlueprintCallable | FUNC_Native;
		flags |= CPF_BlueprintAssignable | CPF_BlueprintCallable;
		flags &= ~CPF_Edit;
	}

	else if (u_class == UObjectProperty::StaticClass())
	{
		// ensure it is not an arry as we have already managed it !
		if (!is_array && !is_map)
		{
			UObjectProperty *obj_prop = (UObjectProperty *)u_property;
			if (u_prop_class)
			{
				obj_prop->SetPropertyClass(u_prop_class);
			}
		}
	}

	else if (u_class == UStructProperty::StaticClass())
	{
		// ensure it is not an arry as we have already managed it !
		if (!is_array && !is_map)
		{
			UStructProperty *obj_prop = (UStructProperty *)u_property;
			if (u_script_struct)
			{
				obj_prop->Struct = u_script_struct;
			}
		}
	}

#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 20))
	u_property->SetPropertyFlags(flags);
#else
	u_property->SetPropertyFlags((EPropertyFlags)flags);
#endif
	u_property->ArrayDim = 1;

	UStruct *u_struct = (UStruct *)self->ue_object;
	u_struct->AddCppProperty(u_property);
	u_struct->StaticLink(true);


	if (u_struct->IsA<UClass>())
	{
		UClass *owner_class = (UClass *)u_struct;
		owner_class->GetDefaultObject()->RemoveFromRoot();
		owner_class->GetDefaultObject()->ConditionalBeginDestroy();
		owner_class->ClassDefaultObject = nullptr;
		owner_class->GetDefaultObject()->PostInitProperties();
	}

	// TODO add default value

	Py_RETURN_UOBJECT(u_property);
	}
#endif

PyObject *py_ue_as_dict(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UStruct *u_struct = nullptr;
	UObject *u_object = self->ue_object;

	if (self->ue_object->IsA<UStruct>())
	{
		u_struct = (UStruct *)self->ue_object;
		if (self->ue_object->IsA<UClass>())
		{
			UClass *u_class = (UClass *)self->ue_object;
			u_object = u_class->GetDefaultObject();
		}
	}
	else
	{
		u_struct = (UStruct *)self->ue_object->GetClass();
	}

	PyObject *py_struct_dict = PyDict_New();
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25)
	TFieldIterator<FProperty> SArgs(u_struct);
#else
	TFieldIterator<UProperty> SArgs(u_struct);
#endif
	for (; SArgs; ++SArgs)
	{
		PyObject *struct_value = ue_py_convert_property(*SArgs, (uint8 *)u_object, 0);
		if (!struct_value)
		{
			Py_DECREF(py_struct_dict);
			return NULL;
		}
		PyDict_SetItemString(py_struct_dict, TCHAR_TO_UTF8(*SArgs->GetName()), struct_value);
	}
	return py_struct_dict;
}

PyObject *py_ue_get_cdo(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UClass *u_class = ue_py_check_type<UClass>(self);
	if (!u_class)
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UClass");
	}

	Py_RETURN_UOBJECT(u_class->GetDefaultObject());
}

PyObject *py_ue_get_archetype(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UObject *Archetype = self->ue_object->GetArchetype();

	if (!Archetype)
		return PyErr_Format(PyExc_Exception, "uobject has no archetype");

	Py_RETURN_UOBJECT(Archetype);
}

PyObject *py_ue_get_archetype_instances(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	TArray<UObject *> Instances;

	self->ue_object->GetArchetypeInstances(Instances);

	PyObject *py_list = PyList_New(0);

	for (UObject *Instance : Instances)
	{
		PyList_Append(py_list, (PyObject *)ue_get_python_uobject(Instance));
	}

	return py_list;
}


#if WITH_EDITOR
PyObject *py_ue_save_package(ue_PyUObject * self, PyObject * args)
{

	/*

		Here we have the following cases to manage:

		calling on a UObject without an outer
		calling on a UObject with an outer
		calling on a UObject with an outer and a name arg

	*/

	ue_py_check(self);

	bool has_package = false;

	char *name = nullptr;
	UPackage *package = nullptr;
	if (!PyArg_ParseTuple(args, "|s:save_package", &name))
	{
		return NULL;
	}
	UObject *outer = self->ue_object->GetOutermost();
	UObject *u_object = self->ue_object;

	if (outer && outer->IsA<UPackage>() && outer != GetTransientPackage())
	{
		package = (UPackage *)outer;
		has_package = true;
	}
	else if (u_object && u_object->IsA<UPackage>() && u_object != GetTransientPackage())
	{
		package = (UPackage *)u_object;
		has_package = true;
	}

	bool bIsMap = u_object->IsA<UWorld>();

	if (!package || name)
	{
		if (!name)
		{
			return PyErr_Format(PyExc_Exception, "the object has no associated package, please specify a name");
		}
		if (!has_package)
		{
			// unmark transient object
			if (u_object->HasAnyFlags(RF_Transient))
			{
				u_object->ClearFlags(RF_Transient);
			}
		}
		// create a new package if it does not exist
		package = CreatePackage(UTF8_TO_TCHAR(name));
		if (!package)
			return PyErr_Format(PyExc_Exception, "unable to create package");

		package->SetLoadedPath(FPackagePath::FromPackageNameChecked(UTF8_TO_TCHAR(name)));
		if (has_package)
		{
			FString split_path;
			FString split_filename;
			FString split_extension;
			FString split_base(UTF8_TO_TCHAR(name));
			FPaths::Split(split_base, split_path, split_filename, split_extension);
			u_object = DuplicateObject(self->ue_object, package, FName(*split_filename));
		}
		else
		{
			// move to object into the new package
			if (!self->ue_object->Rename(*(self->ue_object->GetName()), package, REN_Test))
			{
				return PyErr_Format(PyExc_Exception, "unable to set object outer to package");
			}
			if (!self->ue_object->Rename(*(self->ue_object->GetName()), package))
			{
				return PyErr_Format(PyExc_Exception, "unable to set object outer to package");
			}
		}
	}

	// ensure the right flags are applied
	u_object->SetFlags(RF_Public | RF_Standalone);

	package->FullyLoad();
	package->MarkPackageDirty();

	if (package->GetLoadedPath().IsEmpty())
	{
		package->SetLoadedPath(FPackagePath::FromPackageNameChecked(package->GetPathName()));
		UE_LOG(LogPython, Warning, TEXT("no file mapped to UPackage %s, setting its FileName to %s"), *package->GetPathName(), *package->GetLoadedPath().GetPackageName());
	}

	// NOTE: FileName may not be a fully qualified filepath
	//if (FPackageName::IsValidLongPackageName(package->FileName.ToString()))
	//{
	//	package->FileName = *FPackageName::LongPackageNameToFilename(package->GetPathName(), bIsMap ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());
	//}

	FSavePackageArgs SaveArgs = { nullptr, nullptr, EObjectFlags::RF_Standalone, ESaveFlags::SAVE_None, false, true, true, FDateTime::MinValue(), GError };
	if (GEditor->SavePackage(package, u_object, *package->GetLoadedPath().GetLocalFullPath(), SaveArgs))
	{
		FAssetRegistryModule::AssetCreated(u_object);
		Py_RETURN_UOBJECT(u_object);
	}

	return PyErr_Format(PyExc_Exception, "unable to save package");
}

PyObject *py_ue_import_custom_properties(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	char *t3d;

	if (!PyArg_ParseTuple(args, "s:import_custom_properties", &t3d))
	{
		return nullptr;
	}

	FFeedbackContextAnsi context;

	Py_BEGIN_ALLOW_THREADS;
	self->ue_object->ImportCustomProperties(UTF8_TO_TCHAR(t3d), &context);
	Py_END_ALLOW_THREADS;

	TArray<FString> errors;
	context.GetErrors(errors);

	if (errors.Num() > 0)
	{
		return PyErr_Format(PyExc_Exception, "%s", TCHAR_TO_UTF8(*errors[0]));
	}

	Py_RETURN_NONE;
}

PyObject *py_ue_asset_can_reimport(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	if (FReimportManager::Instance()->CanReimport(self->ue_object))
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject *py_ue_asset_reimport(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	PyObject *py_ask_for_new_file = nullptr;
	PyObject *py_show_notification = nullptr;
	char *filename = nullptr;
	if (!PyArg_ParseTuple(args, "|OOs:asset_reimport", &py_ask_for_new_file, &py_show_notification, &filename))
	{
		return NULL;
	}

	bool ask_for_new_file = false;
	bool show_notification = false;
	FString f_filename = FString("");

	if (py_ask_for_new_file && PyObject_IsTrue(py_ask_for_new_file))
		ask_for_new_file = true;

	if (py_show_notification && PyObject_IsTrue(py_show_notification))
		show_notification = true;

	if (filename)
		f_filename = FString(UTF8_TO_TCHAR(filename));

	if (FReimportManager::Instance()->Reimport(self->ue_object, ask_for_new_file, show_notification, f_filename))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject *py_ue_duplicate(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	char *package_name;
	char *object_name;
	PyObject *py_overwrite = nullptr;

	if (!PyArg_ParseTuple(args, "ss|O:duplicate", &package_name, &object_name, &py_overwrite))
		return nullptr;

	ObjectTools::FPackageGroupName pgn;
	pgn.ObjectName = UTF8_TO_TCHAR(object_name);
	pgn.GroupName = FString("");
	pgn.PackageName = UTF8_TO_TCHAR(package_name);

	TSet<UPackage *> refused;

	UObject *new_asset = nullptr;

	Py_BEGIN_ALLOW_THREADS;
#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 14))
	new_asset = ObjectTools::DuplicateSingleObject(self->ue_object, pgn, refused);
#else
	new_asset = ObjectTools::DuplicateSingleObject(self->ue_object, pgn, refused, (py_overwrite && PyObject_IsTrue(py_overwrite)));
#endif
	Py_END_ALLOW_THREADS;

	if (!new_asset)
		return PyErr_Format(PyExc_Exception, "unable to duplicate object");

	Py_RETURN_UOBJECT(new_asset);
}
#endif


PyObject *py_ue_to_bytes(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	TArray<uint8> Bytes;

	FObjectWriter(self->ue_object, Bytes);

	return PyBytes_FromStringAndSize((const char *)Bytes.GetData(), Bytes.Num());
}

PyObject *py_ue_to_bytearray(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	TArray<uint8> Bytes;

	FObjectWriter(self->ue_object, Bytes);

	return PyByteArray_FromStringAndSize((const char *)Bytes.GetData(), Bytes.Num());
}

PyObject *py_ue_from_bytes(ue_PyUObject * self, PyObject * args)
{

	Py_buffer py_buf;

	if (!PyArg_ParseTuple(args, "z*:from_bytes", &py_buf))
		return nullptr;

	ue_py_check(self);

	TArray<uint8> Bytes;
	Bytes.AddUninitialized(py_buf.len);
	FMemory::Memcpy(Bytes.GetData(), py_buf.buf, py_buf.len);

	FObjectReader(self->ue_object, Bytes);

	Py_RETURN_NONE;
}

PyObject* py_ue_get_resource_size_ex(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	FResourceSizeEx resource_size(EResourceSizeMode::Exclusive);
	self->ue_object->GetResourceSizeEx(resource_size);
	SIZE_T size = resource_size.GetTotalMemoryBytes();

	return PyLong_FromSize_t(size);
}

PyObject* py_ue_get_resource_size_total(ue_PyUObject* self, PyObject* args)
{
	ue_py_check(self);

	FResourceSizeEx resource_size(EResourceSizeMode::EstimatedTotal);
	self->ue_object->GetResourceSizeEx(resource_size);
	SIZE_T size = resource_size.GetTotalMemoryBytes();

	return PyLong_FromSize_t(size);
}