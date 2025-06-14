#include "UEPyWorld.h"

#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/CoreUObject/Public/UObject/UObjectIterator.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"
#include "EditorWorldUtils.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Public/EditorActorFolders.h"
#endif

PyObject *py_ue_world_exec(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *command;
	if (!PyArg_ParseTuple(args, "s:world_exec", &command))
	{
		return NULL;
	}

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	if (world->Exec(world, UTF8_TO_TCHAR(command)))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;

}

PyObject *py_ue_quit_game(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	// no need to support multiple controllers
	APlayerController *controller = world->GetFirstPlayerController();
	if (!controller)
		return PyErr_Format(PyExc_Exception, "unable to retrieve the first controller");

#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 20)
	UKismetSystemLibrary::QuitGame(world, controller, EQuitPreference::Quit, false);
#else
	UKismetSystemLibrary::QuitGame(world, controller, EQuitPreference::Quit);
#endif

	Py_RETURN_NONE;
}

PyObject *py_ue_get_world_type(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	return PyLong_FromUnsignedLong(world->WorldType);
}

PyObject *py_ue_play(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	world->BeginPlay();

	Py_RETURN_NONE;
}

// mainly used for testing
PyObject *py_ue_world_tick(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	float delta_time;
	PyObject *py_increase_fc = nullptr;
	if (!PyArg_ParseTuple(args, "f|O:world_tick", &delta_time, &py_increase_fc))
	{
		return nullptr;
	}

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	world->Tick(LEVELTICK_All, delta_time);

	if (py_increase_fc && PyObject_IsTrue(py_increase_fc))
		GFrameCounter++;

	Py_RETURN_NONE;
}

PyObject *py_ue_all_objects(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	PyObject *ret = PyList_New(0);

	for (TObjectIterator<UObject> Itr; Itr; ++Itr)
	{
		UObject *u_obj = *Itr;
		if (u_obj->GetWorld() != world)
			continue;
		ue_PyUObject *py_obj = ue_get_python_uobject(u_obj);
		if (!py_obj)
			continue;
		PyList_Append(ret, (PyObject *)py_obj);
	}
	return ret;
}

PyObject *py_ue_all_actors(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	PyObject *ret = PyList_New(0);

	TUniquePtr<FScopedEditorWorld> ScopedWorld;
	if (!world->bIsWorldInitialized)
	{
		UWorld::InitializationValues IVS;
		IVS.RequiresHitProxies(false);
		IVS.ShouldSimulatePhysics(false);
		IVS.EnableTraceCollision(false);
		IVS.CreateNavigation(false);
		IVS.CreateAISystem(false);
		IVS.AllowAudioPlayback(false);
		IVS.CreatePhysicsScene(true);
		ScopedWorld = MakeUnique<FScopedEditorWorld>(world, IVS);
	}
	
	if(world->IsPartitionedWorld()) // world partition
	{
		UWorldPartition* WorldPartition = world->GetWorldPartition();
		if(!WorldPartition->IsInitialized())
		{
			WorldPartition->Initialize(world, FTransform::Identity);
		}
		UDataLayerManager* DataLayerMgr = WorldPartition->GetDataLayerManager();
		DataLayerMgr->ForEachDataLayerInstance([&](UDataLayerInstance *DataLayer)
		{
			DataLayer->SetIsLoadedInEditor(true, false);
			DataLayer->SetIsInitiallyVisible(true);
			UE_LOG(LogPython, Log, TEXT("Load DataLayer '%s' in World '%s'"), *DataLayer->GetDataLayerFullName(), *world->GetDebugDisplayName());
			return true;
		});
		FDataLayersEditorBroadcast::StaticOnActorDataLayersEditorLoadingStateChanged(false);
		FLoaderAdapterShape LoaderAdapterShape(world,
			FBox(FVector(-HALF_WORLD_MAX, -HALF_WORLD_MAX, -HALF_WORLD_MAX), FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX)),
			TEXT("LoadedRegion"));
		LoaderAdapterShape.Load();

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
		for (FActorDescContainerInstanceCollection::TConstIterator<> ActorDescIterator(WorldPartition); ActorDescIterator; ++ActorDescIterator)
		{
			const FWorldPartitionActorDescInstance* ActorDesc = *ActorDescIterator;
#else
		for (FActorDescContainerCollection::TConstIterator<> ActorDescIterator(WorldPartition); ActorDescIterator; ++ActorDescIterator)
		{
			const FWorldPartitionActorDesc* ActorDesc = *ActorDescIterator;
#endif
			if (AActor* Actor = ActorDesc->GetActor())
			{
				ue_PyUObject* py_obj = ue_get_python_uobject(Actor);
				if (py_obj)
					PyList_Append(ret, (PyObject*)py_obj);
			}
		}
	}
	else // world composition
	{
		for (ULevelStreaming* LevelStreaming : world->GetStreamingLevels())
		{
			if (LevelStreaming != nullptr)
			{
				LevelStreaming->SetShouldBeLoaded(true);
				LevelStreaming->SetShouldBeVisible(true);
				LevelStreaming->SetShouldBeVisibleInEditor(true);
			}
		}
		if (ensure(GEngine) && GEngine->GetWorldContextFromWorld(world) == nullptr)
		{
			FWorldContext* newWorldContext = &GEngine->CreateNewWorldContext(EWorldType::Type::Editor);
			if (ensure(newWorldContext))
				newWorldContext->SetCurrentWorld(world);
		}
		world->FlushLevelStreaming();
		
		for (TActorIterator<AActor> Itr(world, AActor::StaticClass(), EActorIteratorFlags::AllActors); Itr; ++Itr)
		{
			UObject *u_obj = *Itr;
			ue_PyUObject *py_obj = ue_get_python_uobject(u_obj);
			if (!py_obj)
				continue;
			PyList_Append(ret, (PyObject *)py_obj);
		}
	}

	return ret;
}

PyObject *py_ue_find_object(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *name;
	if (!PyArg_ParseTuple(args, "s:find_object", &name))
	{
		return NULL;
	}

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	UObject *u_object = FindObject<UObject>(world->GetOutermost(), UTF8_TO_TCHAR(name));

	if (!u_object)
		return PyErr_Format(PyExc_Exception, "unable to find object %s", name);

	Py_RETURN_UOBJECT(u_object);
}

PyObject *py_ue_get_world(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	Py_RETURN_UOBJECT(world);
}

PyObject *py_ue_get_game_viewport(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	UGameViewportClient *viewport_client = world->GetGameViewport();
	if (!viewport_client)
		return PyErr_Format(PyExc_Exception, "world has no GameViewportClient");

	Py_RETURN_UOBJECT((UObject *)viewport_client);
}




PyObject *py_ue_has_world(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	if (ue_get_uworld(self))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

PyObject *py_ue_set_view_target(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	PyObject *py_obj;
	int controller_id = 0;
	if (!PyArg_ParseTuple(args, "O|i:set_view_target", &py_obj, &controller_id))
	{
		return NULL;
	}

	AActor *actor = ue_py_check_type<AActor>(py_obj);
	if (!actor)
	{
		return PyErr_Format(PyExc_Exception, "argument is not an actor");
	}

	APlayerController *controller = UGameplayStatics::GetPlayerController(world, controller_id);
	if (!controller)
		return PyErr_Format(PyExc_Exception, "unable to retrieve controller %d", controller_id);

	controller->SetViewTarget(actor);

	Py_RETURN_NONE;

}

PyObject *py_ue_get_world_delta_seconds(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	return Py_BuildValue("f", UGameplayStatics::GetWorldDeltaSeconds(world));
}

PyObject *py_ue_get_levels(ue_PyUObject * self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	PyObject *ret = PyList_New(0);

	for (ULevel *level : world->GetLevels())
	{
		ue_PyUObject *py_obj = ue_get_python_uobject(level);
		if (!py_obj)
			continue;
		PyList_Append(ret, (PyObject *)py_obj);

	}
	return ret;
}

PyObject *py_ue_get_current_level(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	ULevel *level = world->GetCurrentLevel();
	if (!level)
		Py_RETURN_NONE;

	Py_RETURN_UOBJECT(level);
}

PyObject *py_ue_set_current_level(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	PyObject *py_level;

	if (!PyArg_ParseTuple(args, "O", &py_level))
		return nullptr;

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	ULevel *level = ue_py_check_type<ULevel>(py_level);
	if (!level)
		return PyErr_Format(PyExc_Exception, "argument is not a ULevel");

#if WITH_EDITOR || !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 22))

	if (world->SetCurrentLevel(level))
		Py_RETURN_TRUE;
#endif

	Py_RETURN_FALSE;
}

#if WITH_EDITOR
PyObject *py_ue_get_level_script_blueprint(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	ULevel *level = ue_py_check_type<ULevel>(self);
	if (!level)
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a ULevel");
	}

	Py_RETURN_UOBJECT((UObject*)level->GetLevelScriptBlueprint());
}

PyObject *py_ue_world_create_folder(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *path;
	if (!PyArg_ParseTuple(args, "s:world_create_folder", &path))
		return nullptr;

	FActorFolders::Get();

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	FName FolderPath = FName(UTF8_TO_TCHAR(path));

	FActorFolders::Get().CreateFolder(*world, FFolder(FFolder::GetWorldRootFolder(world).GetRootObject(), FolderPath));

	Py_RETURN_NONE;
}

PyObject *py_ue_world_delete_folder(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *path;
	if (!PyArg_ParseTuple(args, "s:world_delete_folder", &path))
		return nullptr;

	FActorFolders::Get();

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	FName FolderPath = FName(UTF8_TO_TCHAR(path));

	FActorFolders::Get().DeleteFolder(*world, FFolder(FFolder::GetWorldRootFolder(world).GetRootObject(), FolderPath));

	Py_RETURN_NONE;
}

PyObject *py_ue_world_rename_folder(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *path;
	char *new_path;
	if (!PyArg_ParseTuple(args, "ss:world_rename_folder", &path, &new_path))
		return nullptr;

	FActorFolders::Get();

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

	FFolder FolderPath(FFolder::GetWorldRootFolder(world).GetRootObject(), FName(UTF8_TO_TCHAR(path)));
	FFolder NewFolderPath(FFolder::GetWorldRootFolder(world).GetRootObject(), FName(UTF8_TO_TCHAR(new_path)));

	if (FActorFolders::Get().RenameFolderInWorld(*world, FolderPath, NewFolderPath))
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

PyObject *py_ue_world_folders(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	FActorFolders::Get();

	UWorld *world = ue_get_uworld(self);
	if (!world)
		return PyErr_Format(PyExc_Exception, "unable to retrieve UWorld from uobject");

#if ENGINE_MAJOR_VERSION == 4
	const TMap<FName, FActorFolderProps> &Folders = FActorFolders::Get().GetFolderPropertiesForWorld(*world);

#endif
	PyObject *py_list = PyList_New(0);

#if ENGINE_MAJOR_VERSION == 5
	FActorFolders::Get().ForEachFolder(*world, [py_list](const FFolder& Folder)
	{
		PyObject* py_str = PyUnicode_FromString(TCHAR_TO_UTF8(*Folder.ToString()));
		PyList_Append(py_list, py_str);
		Py_DECREF(py_str);
		return true;
	});
#else
	TArray<FName> FolderNames;
	Folders.GenerateKeyArray(FolderNames);
	
	for (FName FolderName : FolderNames)
	{
		PyObject *py_str = PyUnicode_FromString(TCHAR_TO_UTF8(*FolderName.ToString()));
		PyList_Append(py_list, py_str);
		Py_DECREF(py_str);
	}
#endif

	return py_list;
}
#endif
