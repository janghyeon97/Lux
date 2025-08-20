// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

LUX_API DECLARE_LOG_CATEGORY_EXTERN(LogLux, Log, All);
LUX_API DECLARE_LOG_CATEGORY_EXTERN(LogLuxActionSystem, Log, All);
LUX_API DECLARE_LOG_CATEGORY_EXTERN(LogLuxTeams, Log, All);
LUX_API DECLARE_LOG_CATEGORY_EXTERN(LogLuxCooldown, Log, All);
LUX_API DECLARE_LOG_CATEGORY_EXTERN(LogLuxCombat, Log, All);

LUX_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
