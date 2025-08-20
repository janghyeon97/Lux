#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuxDynamicValue.generated.h"



UENUM(BlueprintType)
enum class EPhaseParameterSource : uint8
{
    Static,
    FromLevelData,
    FromActionPayload
};


/**
 * 오프셋을 어느 공간(Space) 기준으로 적용할지 결정합니다.
 */
UENUM(BlueprintType)
enum class EPhaseParameterSpace : uint8
{
	// 시전자의 로컬 공간을 기준으로 오프셋을 적용합니다. (전방 100cm 등)
	Local,

	// 월드 공간을 기준으로 오프셋을 적용합니다.
	World
};



USTRUCT(BlueprintType)
struct FDynamicVector
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPhaseParameterSource Source = EPhaseParameterSource::Static;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPhaseParameterSpace Space = EPhaseParameterSpace::Local;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Source == EPhaseParameterSource::Static", EditConditionHides))
	FVector StaticValue = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Source != EPhaseParameterSource::Static", EditConditionHides))
	FName DataKey;

	FVector GetValue(class ULuxAction* InAction) const;
};




USTRUCT(BlueprintType)
struct FDynamicRotator
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPhaseParameterSource Source = EPhaseParameterSource::Static;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPhaseParameterSpace Space = EPhaseParameterSpace::Local;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Source == EPhaseParameterSource::Static", EditConditionHides))
	FRotator StaticValue = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Source != EPhaseParameterSource::Static", EditConditionHides))
	FName DataKey;

	FRotator GetValue(class ULuxAction* InAction) const;
};



/**
 * 다양한 타입의 동적 값을 지원할 수 있도록 설계된 베이스 구조체입니다.
 */
 USTRUCT(BlueprintType)
 struct FDynamicFloat
 {
     GENERATED_BODY()
 
 public:
     // 이 파라미터의 값을 어디서 가져올지 결정합니다.
     UPROPERTY(EditAnywhere, BlueprintReadOnly)
     EPhaseParameterSource Source = EPhaseParameterSource::Static;
 
     // Source가 'Static'일 때 사용할 고정 값입니다.
     UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Source == EPhaseParameterSource::Static", EditConditionHides))
     float StaticValue = 0.f;
 
     /**
      * Source가 'FromLevelData' 또는 'FromActionPayload'일 때,
      * 값을 찾기 위한 이름(Key)입니다.
      * * 예: LevelData의 'Range' 변수 값을 사용하려면 여기에 "Range"라고 입력합니다.
      */
     UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "Source != EPhaseParameterSource::Static", EditConditionHides))
     FName DataKey;
 
     /**
      * 정의된 Source와 DataKey를 바탕으로 최종 float 값을 찾아 반환하는 헬퍼 함수입니다.
      * 이 함수의 구체적인 내용은 .cpp 파일에 구현합니다.
      */
     float GetValue(class ULuxAction* InAction) const;
 };