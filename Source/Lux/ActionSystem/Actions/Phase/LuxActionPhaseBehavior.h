#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseTypes.h"
#include "Cues/LuxCueNotify.h"
#include "LuxActionPhaseBehavior.generated.h"

class ULuxAction;
class ULuxEffect;

USTRUCT(BlueprintType)
struct FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const {};

    virtual ~FPhaseBehaviorBase() {}

    UPROPERTY(EditAnywhere, Category = "Behavior")
    EPhaseBehaviorNetExecutionPolicy NetExecutionPolicy = EPhaseBehaviorNetExecutionPolicy::ServerOnly;
};

USTRUCT(BlueprintType)
struct FPhaseBehavior_AddTags : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    FGameplayTagContainer TagsToAdd;
};

USTRUCT(BlueprintType)
struct FPhaseBehavior_RemoveTags : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    FGameplayTagContainer TagsToRemove;
};

USTRUCT(BlueprintType)
struct FPhaseBehavior_RunTask : public FPhaseBehaviorBase
{
    GENERATED_BODY()

    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    FPhaseTaskDefinition TaskToRun;
};

USTRUCT(BlueprintType)
struct FPhaseBehavior_ApplyEffectToSelf : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    TSubclassOf<class ULuxEffect> EffectToApply;
};

USTRUCT(BlueprintType)
struct FPhaseBehavior_PlaySound : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    TObjectPtr<class USoundBase> SoundToPlay;
};

USTRUCT(BlueprintType)
struct FPhaseBehavior_ExecuteCue : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    FGameplayTag CueTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDynamicFloat Magnitude;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDynamicFloat Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDynamicFloat Rate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDynamicFloat Lifetime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
    FDynamicVector LocationOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
    FDynamicRotator RotationOffset;
};


USTRUCT(BlueprintType)
struct FPhaseBehavior_EndAction : public FPhaseBehaviorBase
{
    GENERATED_BODY()
   
public:
    virtual void Execute(ULuxAction* Action) const override;
}; 


/**
 * 태스크가 PostTaskEvent로 보낸 결과를 ActionPayload에 저장하는 Behavior입니다.
 */
USTRUCT(BlueprintType)
struct FPhaseBehavior_StoreTaskResult : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    /** 수신 대기할 태스크 이벤트의 태그입니다. (예: Task.Event.Path.Ready) */
    UPROPERTY(EditAnywhere, Category = "Behavior")
    FGameplayTag TriggeringEventTag;

    /**
     * 이벤트 페이로드에서 데이터를 가져올 때 사용할 키(Key)입니다.
     * 예: TraceAndBuildPath 태스크는 "PathData"라는 키로 결과를 보냅니다.
     */
    UPROPERTY(EditAnywhere, Category = "Behavior")
    FName SourcePayloadKey;

    /**
     * 가져온 데이터를 ActionPayload에 저장할 때 사용할 키(Key)입니다.
     * 비워두면 SourcePayloadKey와 동일한 키를 사용합니다.
     */
    UPROPERTY(EditAnywhere, Category = "Behavior")
    FName DestinationPayloadKey;
};


USTRUCT(BlueprintType)
struct FPhaseBehavior_SpawnActionActor : public FPhaseBehaviorBase
{
    GENERATED_BODY()
public:
    virtual void Execute(ULuxAction* Action) const override;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    TSubclassOf<AActor> ActorClass;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    bool bDestroyWithAction = true;
}; 


/**
 * 지정된 카메라 모드를 LuxCameraComponent 스택에 Push합니다.
 */
USTRUCT(BlueprintType)
struct FPhaseBehavior_PushCameraMode : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    /** 이 페이즈에 진입할 때 활성화할 카메라 모드입니다. */
    UPROPERTY(EditAnywhere, Category = "Behavior")
    TSubclassOf<class ULuxCameraMode> CameraModeToPush;
};


/**
 * LuxCameraComponent 스택에서 현재 카메라 모드를 Pop합니다.
 */
USTRUCT(BlueprintType)
struct FPhaseBehavior_PopCameraMode : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;
};



/**
 * 지정된 카메라 쉐이크를 재생합니다.
 */
USTRUCT(BlueprintType)
struct FPhaseBehavior_PlayCameraShake : public FPhaseBehaviorBase
{
    GENERATED_BODY()

public:
    virtual void Execute(ULuxAction* Action) const override;

    /** 재생할 카메라 쉐이크 클래스입니다. */
    UPROPERTY(EditAnywhere, Category = "Behavior")
    TSubclassOf<UCameraShakeBase> ShakeClass;

    /** 쉐이크의 강도 배율입니다. */
    UPROPERTY(EditAnywhere, Category = "Behavior")
    float Scale = 1.0f;
};


/**
 * 페이즈에 진입할 때 액션의 쿨다운을 시작시킵니다.
 */
USTRUCT(BlueprintType)
struct FPhaseBehavior_StartCooldown : public FPhaseBehaviorBase
{
    GENERATED_BODY()
public:
    virtual void Execute(ULuxAction* Action) const override;
};