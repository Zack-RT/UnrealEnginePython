#pragma once

#include "UnrealEnginePython.h"

#include "UEPySTreeView.h"

extern PyTypeObject ue_PySPythonTreeViewType;

class SPythonTreeView : public STreeView<TSharedPtr<struct FPythonItem>>
{
public:
	~SPythonTreeView()
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
		SetItemsSource(nullptr);
#else
	delete(ItemsSource);
#endif
	}

	void SetPythonItemExpansion(PyObject *item, bool InShouldExpandItem);
};

typedef struct
{
	ue_PySTreeView s_tree_view;
	/* Type-specific fields go here. */
} ue_PySPythonTreeView;

void ue_python_init_spython_tree_view(PyObject *);
