// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/LuxTeamAgentInterface.h"
#include "UObject/ScriptInterface.h"

// Add default functionality here for any ILuxTeamAgentInterface functions that are not pure virtual.

void ILuxTeamAgentInterface::ConditionalBroadcastTeamChanged(TScriptInterface<ILuxTeamAgentInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	if (OldTeamID != NewTeamID)
	{
		const int32 OldTeamIndex = GenericTeamIdToInteger(OldTeamID);
		const int32 NewTeamIndex = GenericTeamIdToInteger(NewTeamID);

		UObject* ThisObj = This.GetObject();
	
		This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeamIndex, NewTeamIndex);
	}
}
