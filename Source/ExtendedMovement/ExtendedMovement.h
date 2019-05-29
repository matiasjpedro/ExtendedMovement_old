// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

//TODO EXM: Delta compress the dual movement.
//TODO EXM: Comment system.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FExtendedMovementModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
