// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PythonConsoleModule.h"
#include "SPythonConsole.h"
#include "SPythonLog.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "LevelEditor.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/FileManager.h"
#include "EditorStyleSet.h"
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
#include "Styling/AppStyle.h"
#endif

#define LOCTEXT_NAMESPACE "PythonConsole"
IMPLEMENT_MODULE( FPythonConsoleModule, PythonConsole );

namespace PythonConsoleModule
{
	static const FName PythonLogTabName = FName(TEXT("PythonLog"));
}

/** This class is to capture all log output even if the log window is closed */
class FPythonLogHistory : public FOutputDevice
{
public:

	FPythonLogHistory()
	{
		GLog->AddOutputDevice(this);
		GLog->SerializeBacklog(this);
	}

	~FPythonLogHistory()
	{
		// At shutdown, GLog may already be null
		if( GLog != NULL )
		{
			GLog->RemoveOutputDevice(this);
		}
	}

	/** Gets all captured messages */
	const TArray< TSharedPtr<FLogMessage> >& GetMessages() const
	{
		return Messages;
	}

protected:

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) override
	{
		// Capture all incoming messages and store them in history
		SPythonLog::CreateLogMessages(V, Verbosity, Category, Messages);
	}

private:

	/** All log messsges since this module has been started */
	TArray< TSharedPtr<FLogMessage> > Messages;
};

/** Our global output log app spawner */
static TSharedPtr<FPythonLogHistory> PythonLogHistory;

TSharedRef<SDockTab> SpawnPythonLog( const FSpawnTabArgs& Args )
{
	return SNew(SDockTab)
	.Icon(FEditorStyle::GetBrush("Log.TabIcon"))
	.TabRole( ETabRole::PanelTab )
	.Label( NSLOCTEXT("PythonConsole", "TabTitle", "Python Console") )
	[
		SNew(SPythonLog).Messages( PythonLogHistory->GetMessages() )
	];
}

void FPythonConsoleModule::StartupModule()
{
	PythonLogHistory = MakeShareable(new FPythonLogHistory);
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FPythonConsoleModule::RegisterMenus);
}

void FPythonConsoleModule::ShutdownModule()
{
	PythonLogHistory.Reset();
	UnRegisterMenus();
}

TSharedRef< SWidget > FPythonConsoleModule::MakeConsoleInputBox( TSharedPtr< SEditableTextBox >& OutExposedEditableTextBox ) const
{
	TSharedRef< SPythonConsoleInputBox > NewConsoleInputBox = SNew( SPythonConsoleInputBox );
	OutExposedEditableTextBox = NewConsoleInputBox->GetEditableTextBox();
	return NewConsoleInputBox;
}