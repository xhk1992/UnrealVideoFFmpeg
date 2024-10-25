// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FVideoStreamModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static bool Initialized;
};

DECLARE_LOG_CATEGORY_EXTERN(LogVideoStream, Log, Log);