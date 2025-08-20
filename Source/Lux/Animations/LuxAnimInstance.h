// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstanceProxy.h"
#include "LuxAnimInstance.generated.h"


class UActionSystemComponent;
class ULuxPawnExtensionComponent;
class ULuxHeroComponent;
class UCharacterMovementComponent;

UENUM(BlueprintType)
enum class ETurnState : uint8
{
	None,               // 회전 없음

	StartingTurn_L,     // 왼쪽으로 회전 '시작'
	Turning_L,          // 왼쪽으로 회전 '진행 중'
	RestartTurnL,

	StartingTurn_R,     // 오른쪽으로 회전 '시작'
	Turning_R,          // 오른쪽으로 회전 '진행 중'
	RestartTurnR,
};

UENUM(BlueprintType)
enum class EMovementDirection : uint8
{
	None,
	Forward,
	Backward,
	Left,
	Right
};

/**
 * @struct FLuxAnimInstanceProxy
 * @brief 애니메이션 스레드에서 안전하게 계산을 수행하기 위한 데이터 컨테이너 및 실행기입니다.
 */
USTRUCT(BlueprintType)
struct FLuxAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FLuxAnimInstanceProxy() = default;
	FLuxAnimInstanceProxy(UAnimInstance* InAnimInstance);

	/** 게임 스레드에서 애니메이션 스레드로 데이터를 복사합니다. (스레드-세이프하지 않은 계산은 여기서 수행) */
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;

	/** 애니메이션 스레드에서 실제 업데이트 로직을 수행합니다. (스레드-세이프한 계산만 수행) */
	virtual void Update(float DeltaSeconds) override;

	/** 모든 계산이 끝난 후, 결과를 원본 UAnimInstance 객체로 다시 복사합니다. */
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;



	/** 기본적인 이동 관련 변수들(속도, 가속도, bShouldMove 등)을 업데이트합니다.*/
	void UpdateMovementData();

	/** 조준 및 방향 전환에 사용되는 각도 변수들(YawOffset, LocomotionAngle 등)을 업데이트합니다.*/
	void UpdateAimingData(float DeltaSeconds);
	void UpdateWarpingAngle();

	/** @brief Start 애니메이션을 위한 디스턴스 매칭 변수를 업데이트합니다.  */
	void UpdateDistanceMatching(float DeltaSeconds);

	/** 제자리 선회(Turn In Place) 로직을 처리합니다. */
	void HandleTurnInPlace(float DeltaSeconds);

	/** 움직일 때 Turn In Place 상태를 초기화합니다. */
	void ResetTurnState(float DeltaSeconds);

	/** 진행 중인 회전 애니메이션의 보간을 처리합니다. */
	void ProgressTurnInterpolation(float DeltaSeconds);

	/** YawOffset 값을 기반으로 MovementDirection을 계산합니다. */
	void CalculateMovementDirection();

	FVector PredictGroundMovementStopLocation(
		const FVector& _CurrentLocation,
		const FVector& Velocity,
		bool bUseSeparateBrakingFriction,
		float BrakingFriction,
		float GroundFriction,
		float BrakingFrictionFactor,
		float BrakingDecelerationWalking);

public:
	/** 캐릭터의 현재 속도. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	FVector Velocity = FVector::ZeroVector;

	/** 캐릭터의 지상에서의 현재 속도. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	FVector Velocity2D = FVector::ZeroVector;

	/** 캐릭터의 현재 가속도. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	FVector Acceleration = FVector::ZeroVector;

	/** 캐릭터의 지상에서의 현재 가속도. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	FVector Acceleration2D = FVector::ZeroVector;

	/** 캐릭터가 공중에 떠 있는지 여부 */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	bool bIsFalling = false;

	/** 캐릭터가 움직여야 하는지 여부 */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	bool bShouldMove = false;

	/** 현재 이동 방향과 가속 방향의 일치도 (-1: 감속, 0: 방향전환, 1: 가속)  */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Locomotion")
	float SpeedAccelDotProduct = 0.f;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Aiming")
	FRotator AimRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Aiming")
	float YawOffset = 0.f;

	/** 제자리 선회 애니메이션을 위해 루트 본(Root Bone)에 적용할 Yaw 오프셋입니다. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Aiming")
	float RootYawOffset = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Distance Matching")
	FVector PredictedStopLocation = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Distance Matching")
	float PredictedStopDistance = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Distance Matching")
	float AccumulatedStartDistance = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Warping")
	float LocomotionAngle = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Warping")
	float WarpingAngle = 0.f;

	/** 캐릭터의 현재 이동 방향입니다. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	EMovementDirection MovementDirection = EMovementDirection::None;

	/** 캐릭터가 멈추기 직전의 마지막 이동 방향입니다. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	EMovementDirection LastMovementDirection = EMovementDirection::None;

protected:
	/** 액션(스킬)에 의해 캐릭터의 움직임이 강제되고 있는지 여부 */
	bool bIsActionControlledMovement = false;

	float TurnTimer = 0.f;
	float TurnTargetYaw = 0.f;
	float LastInterpYaw = 0.f;
	float TurnAnimLength = 0.f;

	float TURN_TRIGGER_ANGLE_90 = 90.f;
	float TURN_ANIM_LENGTH_90 = 1.3f;

	FRotator CharacterControlRotation = FRotator::ZeroRotator;
	FRotator PreviousActorRotation = FRotator::ZeroRotator;
	FVector CharacterActorLocation = FVector::ZeroVector;
	FRotator CharacterActorRotation = FRotator::ZeroRotator;
	FVector CharacterVelocity = FVector::ZeroVector;
	FVector CharacterAcceleration = FVector::ZeroVector;

	bool bUseSeparateBrakingFriction = 0.f;
	float BrakingFriction = 0.f;
	float GroundFriction = 0.f;
	float BrakingFrictionFactor = 0.f;
	float BrakingDecelerationWalking = 0.f;

	float CharacterMaxSpeed = 0.f;
	FGameplayTag StanceTag;
	ETurnState TurnState = ETurnState::None;
};



/**
 * 
 */
UCLASS()
class LUX_API ULuxAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FLuxAnimInstanceProxy;

public:
	ULuxAnimInstance(const FObjectInitializer& ObjectInitializer);

	/** 게임플레이 코드에 의해 강제로 회전이 변경될 때 호출되어 RootYawOffset을 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ResetRootYawOffset();

protected:
	/** 이 AnimInstance에 대한 프록시를 생성합니다. */
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	/** AnimInstance가 소멸될 때 프록시를 파괴합니다. */
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UFUNCTION(BlueprintPure, Category = "Animation", meta = (BlueprintThreadSafe))
	float GetTimeAtRemainingDistance(const UAnimSequence* Animation, float Distance) const;

	UFUNCTION(BlueprintPure, Category = "ThreadSafe", meta = (BlueprintThreadSafe))
	UCharacterMovementComponent* GetMovementComponent() const;

protected:
	/** 캐릭터의 현재 속도. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	FVector Velocity = FVector::ZeroVector;

	/** 캐릭터의 현재 이동 방향입니다. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess))
	EMovementDirection MovementDirection = EMovementDirection::None;

	/** 캐릭터가 멈추기 직전의 마지막 이동 방향입니다. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Movement")
	EMovementDirection LastMovementDirection = EMovementDirection::None;

	/** 캐릭터의 지상에서의 현재 속도. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	FVector Velocity2D = FVector::ZeroVector;

	/** 캐릭터의 현재 가속도. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	FVector Acceleration = FVector::ZeroVector;

	/** 캐릭터의 지상에서의 현재 가속도. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	FVector Acceleration2D = FVector::ZeroVector;

	/** 캐릭터가 공중에 떠 있는지 여부 */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	bool bIsFalling = false;

	/** 캐릭터가 움직여야 하는지 여부 */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	bool bShouldMove = false;

	/** 액션(스킬)에 의해 캐릭터의 움직임이 강제되고 있는지 여부 */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Aiming", meta = (AllowPrivateAccess))
	bool bIsActionControlledMovement = false;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	uint8 bTurnInPlace : 1 = 1;

	/** 현재 이동 방향과 가속 방향의 일치도 (-1: 감속, 0: 방향전환, 1: 가속)  */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Locomotion", meta = (AllowPrivateAccess))
	float SpeedAccelDotProduct = 0.f;

	UPROPERTY(Transient, Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Aiming", meta = (AllowPrivateAccess))
	FRotator AimRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Aiming", meta = (AllowPrivateAccess))
	float YawOffset = 0.f;

	/** 제자리 선회 애니메이션을 위해 루트 본(Root Bone)에 적용할 Yaw 오프셋입니다. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Aiming", meta = (AllowPrivateAccess))
	float RootYawOffset = 0.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Turn In Place", meta = (AllowPrivateAccess))
	ETurnState TurnState = ETurnState::None;

	/** 제자리 회전을 시작할 Yaw 각도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn In Place", meta = (AllowPrivateAccess))
	float TurnTriggerAngle90 = 90.f;

	/** 90도 제자리 회전 애니메이션의 재생 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn In Place", meta = (AllowPrivateAccess))
	float TurnAnimLength90 = 1.3f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Distance Matching", meta = (AllowPrivateAccess))
	FVector PredictedStopLocation;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Distance Matching", meta = (AllowPrivateAccess))
	float PredictedStopDistance = 0.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Distance Matching", meta = (AllowPrivateAccess))
	float AccumulatedStartDistance = 0.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Warping", meta = (AllowPrivateAccess))
	float LocomotionAngle = 0.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Warping", meta = (AllowPrivateAccess))
	float WarpingAngle = 0.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "Gameplay State", meta = (AllowPrivateAccess))
	FGameplayTag StanceTag;

private:
	UPROPERTY()
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY()
	TObjectPtr<ULuxPawnExtensionComponent> PawnExtComponet;

	UPROPERTY()
	TObjectPtr<ULuxHeroComponent> HeroComponent;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY()
	TObjectPtr<UActionSystemComponent> ActionSystemComponent;

	/** 우선순위 순서로 정렬된 태그 배열입니다. */
	TArray<FGameplayTag> PrioritySortedStanceTags;
};
