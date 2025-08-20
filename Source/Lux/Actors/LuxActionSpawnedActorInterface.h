#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LuxActionSpawnedActorInterface.generated.h"

class ULuxAction;

UINTERFACE(MinimalAPI, Blueprintable)
class ULuxActionSpawnedActorInterface : public UInterface
{
    GENERATED_BODY()
};

class ILuxActionSpawnedActorInterface
{
    GENERATED_BODY()
public:
    /**
     * 액션에서 생성된 액터가 액션 인스턴스를 받아 초기화할 때 호출됩니다.
     */
    virtual void Initialize(ULuxAction* Action, bool bAutoStart) = 0;
}; 