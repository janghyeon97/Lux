#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Actors/ActorInitData.h"
#include "Actors/LuxActionSpawnedActorInterface.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "LuxBaseActionActor.generated.h"



class ULuxAction;
class UActionSystemComponent;



/**
 * Lux 액션 시스템에서 공통적으로 사용하는 기본 액터 클래스
 * - ILuxActionSpawnedActorInterface 상속
 * - bIsActive 동기화 및 OnRep 함수
 * - 기본 초기화 로직
 * - 액션 정보 자동 저장 및 추적
 */
UCLASS(Abstract)
class LUX_API ALuxBaseActionActor : public AActor, public ILuxActionSpawnedActorInterface
{
    GENERATED_BODY()

public:
    ALuxBaseActionActor();

    /** 액터 활성화(시작) 함수 (자식에서 오버라이드 가능, Blueprint에서도 호출 가능) */
    UFUNCTION(BlueprintCallable, Category = "Lux|Action")
    virtual void Start();

    /** 액터 비활성화(정지) 함수 (자식에서 오버라이드 가능, Blueprint에서도 호출 가능) */
    UFUNCTION(BlueprintCallable, Category = "Lux|Action")
    virtual void Stop();

	/** 액션 객체에서 필요한 데이터만 추출해 초기화합니다. */
	virtual void Initialize(ULuxAction* Action, bool bAutoStart = true) override;

    /** 액터 활성화 여부 (서버에서 변경 시 클라이언트로 복제) */
    UPROPERTY(ReplicatedUsing = OnRep_IsActive, BlueprintReadOnly, Category = "Lux|Action")
    bool bIsActive = false;

    /** 액터의 수명 (초기화 시 설정되며, 자식에서 오버라이드 가능) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action")
    float LifeTime = 0.0f;

    /** bIsActive가 복제될 때 클라이언트에서 호출됨 (자식에서 오버라이드 가능) */
    UFUNCTION()
    virtual void OnRep_IsActive();

protected:
    //~ AActor interface
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End of AActor interface

    // 액션 정보를 저장하여 CC 효과 추적
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<UActionSystemComponent> SourceASC = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action|Tracking")
    TWeakObjectPtr<ULuxAction> SourceAction = nullptr;

    UPROPERTY()
    FLuxActionSpecHandle ActionSpecHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action|Tracking")
    FGameplayTag ActionIdentifierTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action|Tracking")
    int32 ActionLevel = 0;
}; 