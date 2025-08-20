// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "LuxCueNotify.generated.h"

DECLARE_DELEGATE_OneParam(FCueSimpleDelegate, ALuxCueNotify*);

class UActionSystemComponent;

/**
 * 이펙트를 재생할 때 필요한 모든 정보를 담는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FLuxCueContext
{
    GENERATED_BODY()

public:
    FLuxCueContext();
    FLuxCueContext(const FLuxCueContext& Other);
    FLuxCueContext& operator=(const FLuxCueContext& Other);

public:
    /** 이 큐를 최초로 유발시킨 액터 (예: 플레이어 캐릭터) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    TWeakObjectPtr<AActor> Instigator = nullptr;

    /** 이 큐를 직접적으로 발생시킨 객체 (예: 캐릭터가 쏜 발사체) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    TWeakObjectPtr<AActor> EffectCauser = nullptr;

    /** 이 큐를 발생시킨 소스의 액션 시스템 컴포넌트 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    TWeakObjectPtr<UActionSystemComponent> SourceASC = nullptr;

    /** 큐가 발생한 월드 위치 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    FVector Location = FVector::ZeroVector;

    /** 큐가 발생한 지점의 법선 벡터 (이펙트 방향 제어에 사용) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    FVector Normal = FVector::ZeroVector;

    /** 큐가 발생한 지점의 회전값 (예: 발사체 방향) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    FRotator Rotation = FRotator::ZeroRotator;

    /** 액션의 크기, 반지름 등 주된 수치를 전달하는 변수 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Magnitude = 0.0f;

    /** 액션의 전체 지속시간을 전달하는 변수 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Duration = 0.0f;

    /** 액션의 생성 간격을 전달하는 변수 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    float Rate = 0.0f;

    /** 발사체 개수, 콤보 카운트 등 변수 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    int32 Count = 0;

    /** 파티클 수명 등 시간 관련 값을 전달하는 변수. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    float Lifetime = 0.0f;

    /** 이 큐를 발생시킨 액션의 레벨 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    int32 Level = 1;

    /** 시각 효과 분기를 위한 추가적인 태그 컨테이너 (예: 'CriticalHit', 'FireDamage' 등) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LuxCue")
    FGameplayTagContainer CueTags;
};



UCLASS()
class LUX_API ALuxCueNotify : public AActor
{
	GENERATED_BODY()
    friend class ULuxCueManager;
	
public:	
    ALuxCueNotify();

    // CueManager가 Cue를 시작시킬 때 호출할 단일 진입점입니다.
    virtual void Execute(AActor* Target, const FLuxCueContext& Context);

    // CueManager가 Cue를 명시적으로 중지시킬 때 호출할 단일 진입점입니다.
    virtual void Stop();

    FCueSimpleDelegate OnCueActivated;
    FCueSimpleDelegate OnCueStopped;

protected:
    /** 큐가 활성화될 때 호출됩니다. */
    UFUNCTION(BlueprintNativeEvent, Category = "LuxCue", meta = (DisplayName = "On Active"))
    void OnActive(AActor* Target, const FLuxCueContext& Context);
    virtual void OnActive_Implementation(AActor* Target, const FLuxCueContext& Context);

    /**  큐가 중지될 때 호출됩니다. */
    UFUNCTION(BlueprintNativeEvent, Category = "LuxCue", meta = (DisplayName = "On Stopped"))
    void OnStopped(AActor* Target, const FLuxCueContext& Context);
    virtual void OnStopped_Implementation(AActor* Target, const FLuxCueContext& Context);

public:
    /** 어떤 태그에 의해 활성화되는지를 나타냅니다. */
    UPROPERTY(EditDefaultsOnly, Category = "LuxCue", meta = (Categories = "Cue"))
    FGameplayTag CueTag;

    /** 미리 생성해 둘 인스턴스의 개수입니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue")
    int32 PoolSize = 5;

    /** 자동으로 파괴되어야 하는 경우 true로 설정합니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue")
    bool bAutoDestroyOnRemove = false;

    /** true일 경우, ActivateCue 호출 시 Target 액터에 자동으로 부착됩니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue")
    bool bAutoAttachToTarget = false;

    /** 사용할 소켓 이름입니다. (None일 경우 루트에 부착) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue", meta = (EditCondition = "bAutoAttachToTarget"))
    FName AttachSocketName = NAME_None;

public:
    /** 큐의 현재 타겟 액터를 반환합니다. */
    AActor* GetCueTarget() const { return CueTarget.IsValid() ? CueTarget.Get() : nullptr; }

protected:
    UPROPERTY(BlueprintReadOnly, Category = "LuxCue")
    TWeakObjectPtr<AActor> CueTarget;

    UPROPERTY(BlueprintReadOnly, Category = "LuxCue")
    FLuxCueContext InitialContext;
};
