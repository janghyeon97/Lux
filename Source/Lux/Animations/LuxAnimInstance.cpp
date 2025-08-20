// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/LuxAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimSequence.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "Character/LuxHeroComponent.h"
#include "Character/LuxPawnData.h"
#include "Character/LuxCharacter.h"
#include "LuxAnimationData.h"


FLuxAnimInstanceProxy::FLuxAnimInstanceProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy(InAnimInstance)
{
	if (ULuxAnimInstance* LuxAnim = Cast<ULuxAnimInstance>(InAnimInstance))
	{
		StanceTag = LuxAnim->StanceTag;

		TURN_TRIGGER_ANGLE_90 = LuxAnim->TurnTriggerAngle90;
		TURN_ANIM_LENGTH_90 = LuxAnim->TurnAnimLength90;
	}
}

// 게임 스레드에서 애니메이션 스레드로 데이터 복사
void FLuxAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	Super::PreUpdate(InAnimInstance, DeltaSeconds);

	ULuxAnimInstance* LuxAnim = Cast<ULuxAnimInstance>(InAnimInstance);
	if (!LuxAnim) return;

	ALuxCharacter* OwningCharacter = Cast<ALuxCharacter>(LuxAnim->TryGetPawnOwner());
	if (!OwningCharacter) return;

	UCharacterMovementComponent* MovementComponent = OwningCharacter->GetCharacterMovement();
	if (!MovementComponent) return;

	bIsFalling = MovementComponent->IsFalling();

	CharacterVelocity = MovementComponent->Velocity;
	CharacterAcceleration = MovementComponent->GetCurrentAcceleration();
	CharacterActorRotation = OwningCharacter->GetActorRotation();
	CharacterActorLocation = OwningCharacter->GetActorLocation();

	CharacterControlRotation = OwningCharacter->GetBaseAimRotation();
	AimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CharacterControlRotation, CharacterActorRotation);

	bUseSeparateBrakingFriction = MovementComponent->bUseSeparateBrakingFriction;
	BrakingFriction = MovementComponent->BrakingFriction;
	GroundFriction = MovementComponent->GroundFriction;
	BrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;
	BrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;

	bIsActionControlledMovement = OwningCharacter->bIsActionControlledMovement;
}


void FLuxAnimInstanceProxy::Update(float DeltaSeconds)
{
	Super::Update(DeltaSeconds);

	ULuxAnimInstance* LuxAnim = Cast<ULuxAnimInstance>(GetAnimInstanceObject());
	if (!LuxAnim) return;

	// 이동 변수들을 업데이트합니다.
	UpdateMovementData();
	CalculateMovementDirection();

	// 조준 및 워핑에 필요한 각도 변수들을 업데이트합니다.
	UpdateAimingData(DeltaSeconds);
	UpdateWarpingAngle();

	if (bIsActionControlledMovement == false)
	{
		// 디스턴스 매칭 관련 변수들을 업데이트합니다.
		UpdateDistanceMatching(DeltaSeconds);

		// 제자리 회전(Turn In Place) 로직을 처리합니다.
		HandleTurnInPlace(DeltaSeconds);
	}
}

void FLuxAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{
	ULuxAnimInstance* LuxAnim = Cast<ULuxAnimInstance>(InAnimInstance);
	if (!LuxAnim) return;

	// Proxy에서 계산된 최종 값들을 원본 AnimInstance의 변수들로 복사합니다.
	LuxAnim->Velocity = Velocity;
	LuxAnim->Velocity2D = Velocity2D;
	LuxAnim->Acceleration = Acceleration;
	LuxAnim->Acceleration2D = Acceleration2D;

	LuxAnim->bIsFalling = bIsFalling;
	LuxAnim->bShouldMove = bShouldMove;

	LuxAnim->SpeedAccelDotProduct = SpeedAccelDotProduct;

	LuxAnim->AimRotation = AimRotation;
	LuxAnim->YawOffset = YawOffset;
	LuxAnim->RootYawOffset = RootYawOffset;
	LuxAnim->MovementDirection = MovementDirection;
	LuxAnim->LastMovementDirection = LastMovementDirection;

	LuxAnim->PredictedStopLocation = PredictedStopLocation;
	LuxAnim->PredictedStopDistance = PredictedStopDistance;
	LuxAnim->AccumulatedStartDistance = AccumulatedStartDistance;

	LuxAnim->LocomotionAngle = LocomotionAngle;
	LuxAnim->WarpingAngle = WarpingAngle;
	LuxAnim->TurnState = TurnState;

	LuxAnim->bIsActionControlledMovement = bIsActionControlledMovement;
}

void FLuxAnimInstanceProxy::UpdateMovementData()
{
	Velocity = CharacterVelocity;
	Acceleration = CharacterAcceleration;

	Velocity2D = FVector(CharacterVelocity.X, CharacterVelocity.Y, 0.f);
	Acceleration2D = FVector(CharacterAcceleration.X, CharacterAcceleration.Y, 0.f);

	bShouldMove = Velocity2D.Size() > 3.0f && Acceleration2D.Size() > 0.0f;

	const FVector VelNormal = CharacterVelocity.GetSafeNormal(0.0001);
	const FVector AccelNormal = CharacterAcceleration.GetSafeNormal(0.0001);
	SpeedAccelDotProduct = FVector::DotProduct(VelNormal, AccelNormal);
}

void FLuxAnimInstanceProxy::UpdateAimingData(float DeltaSeconds)
{
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(
		UKismetMathLibrary::MakeRotFromX(Velocity2D),
		CharacterControlRotation
	).Yaw;

	if (bShouldMove)
	{
		const FVector CharacterForward = CharacterActorRotation.Vector().GetSafeNormal();
		const FVector CharacterVelocityNormal = Velocity2D.GetSafeNormal();

		// 내적을 이용해 두 벡터 사이의 각도(라디안)를 구합니다.
		const float AngleRadians = FMath::Acos(FVector::DotProduct(CharacterForward, CharacterVelocityNormal));
		float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

		// 외적을 이용해 이동 방향이 왼쪽인지 오른쪽인지 판단합니다.
		const FVector CrossProduct = FVector::CrossProduct(CharacterForward, CharacterVelocityNormal);

		// 외적 결과의 Z값이 음수이면 왼쪽이므로 각도에 마이너스 부호를 붙여줍니다.
		if (CrossProduct.Z < 0.f)
		{
			AngleDegrees *= -1.f;
		}

		LocomotionAngle = AngleDegrees;
	}
	else
	{
		LocomotionAngle = FMath::FInterpTo(LocomotionAngle, 0.f, DeltaSeconds, 10.f);
	}
}

void FLuxAnimInstanceProxy::UpdateWarpingAngle()
{
	if (bShouldMove == false)
	{
		WarpingAngle = 0.f;
		return;
	}

	// 현재 이동 방향(Enum)에 따라 기준 각도를 설정합니다.
	float BaseAngle = 0.f;
	switch (MovementDirection)
	{
	case EMovementDirection::Forward:
		BaseAngle = 0.f;
		break;
	case EMovementDirection::Backward:
		BaseAngle = 180.f;
		break;
	case EMovementDirection::Left:
		BaseAngle = -90.f;
		break;
	case EMovementDirection::Right:
		BaseAngle = 90.f;
		break;
	}

	// 실제 이동 각도(LocomotionAngle)와 기준 각도(BaseAngle)의 차이를 계산합니다.
	const FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(
		FRotator(0, LocomotionAngle, 0),
		FRotator(0, BaseAngle, 0)
	);

	WarpingAngle = DeltaRotator.Yaw;
}

void FLuxAnimInstanceProxy::UpdateDistanceMatching(float DeltaSeconds)
{
	const bool bIsStartingUp = Acceleration2D.IsNearlyZero() == false && Velocity2D.IsNearlyZero(1.f);

	if (bShouldMove == false)
	{
		AccumulatedStartDistance = 0.f;
	}
	else if (bIsStartingUp)
	{
		AccumulatedStartDistance = 0.f;
	}
	else
	{
		AccumulatedStartDistance += Velocity2D.Size() * DeltaSeconds;
	}

	if(Acceleration2D.Size() >= 100.f)
	{
		PredictedStopLocation = PredictGroundMovementStopLocation(
			CharacterActorLocation,
			Velocity2D,
			bUseSeparateBrakingFriction,
			BrakingFriction,
			GroundFriction,
			BrakingFrictionFactor,
			BrakingDecelerationWalking
		);

		PredictedStopDistance = FVector::Dist(CharacterActorLocation, PredictedStopLocation);
	}
}

void FLuxAnimInstanceProxy::CalculateMovementDirection()
{
	// 움직이지 않을 때는 'None' 상태로 설정합니다.
	if (!bShouldMove)
	{
		MovementDirection = EMovementDirection::None;
		return;
	}

	// 정면으로 인식할 범위의 절반 각도
	const float ForwardConeHalfAngle = 60.f;

	// 정면과 측면을 나누는 경계 각도
	const float SideBoundary = ForwardConeHalfAngle;

	// 측면과 후면을 나누는 경계 각도
	const float BackwardBoundary = 180.f - ForwardConeHalfAngle;


	if (FMath::Abs(YawOffset) <= SideBoundary)
		MovementDirection = EMovementDirection::Forward;

	else if (FMath::Abs(YawOffset) >= BackwardBoundary)
		MovementDirection = EMovementDirection::Backward;

	else if (YawOffset > SideBoundary)
		MovementDirection = EMovementDirection::Right;

	else
		MovementDirection = EMovementDirection::Left;


	if (MovementDirection != EMovementDirection::None)
	{
		LastMovementDirection = MovementDirection;
	}
}


void FLuxAnimInstanceProxy::HandleTurnInPlace(float DeltaSeconds)
{
	// 움직이거나 공중에 있으면 회전 상태를 초기화하고 로직을 종료합니다.
	if (bShouldMove || bIsFalling || bIsActionControlledMovement)
	{
		ResetTurnState(DeltaSeconds);
		return;
	}

	// 현재 회전 상태라면 보간을 진행합니다.
	if (TurnState != ETurnState::None)
	{
		ProgressTurnInterpolation(DeltaSeconds);
	}

	const FRotator RotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterActorRotation, PreviousActorRotation);
	RootYawOffset -= RotationDelta.Yaw;
	PreviousActorRotation = CharacterActorRotation;

	// 현재 회전 중인 경우, 보간을 계속 진행하고 새로운 로직 진입을 막습니다.
	if (TurnState != ETurnState::None)
	{
		ProgressTurnInterpolation(DeltaSeconds);
		return;
	}

	// 현재 회전 상태가 아닌 경우에만 회전 시작을 검사합니다.
	if (FMath::Abs(RootYawOffset) >= TURN_TRIGGER_ANGLE_90)
	{
		// 회전 방향을 결정합니다.
		const bool bIsTurningLeft = RootYawOffset > 0.f;

		// 새로운 회전을 시작합니다.
		TurnState = bIsTurningLeft ? ETurnState::Turning_L : ETurnState::Turning_R;
		TurnTargetYaw = bIsTurningLeft ? 90.f : -90.f;

		TurnAnimLength = TURN_ANIM_LENGTH_90;
		TurnTimer = 0.f;
		LastInterpYaw = 0.f;
	}
}

void FLuxAnimInstanceProxy::ResetTurnState(float DeltaSeconds)
{
	RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, 20.f);

	PreviousActorRotation = CharacterActorRotation;
	TurnState = ETurnState::None;
	TurnTargetYaw = 0.f;
	TurnTimer = 0.f;
	LastInterpYaw = 0.f;
}

void FLuxAnimInstanceProxy::ProgressTurnInterpolation(float DeltaSeconds)
{
	TurnTimer += DeltaSeconds;
	TurnState = RootYawOffset > 0.f ? ETurnState::Turning_L : ETurnState::Turning_R;

	// SinInOut 곡선을 사용하여 부드러운 가감속 보간 값을 계산합니다.
	const float Alpha = FMath::Clamp(TurnTimer / TurnAnimLength, 0.f, 1.f);
	const float CurrentInterpYaw = FMath::InterpSinInOut(0.f, FMath::Abs(TurnTargetYaw), Alpha);
	const float DeltaInterp = CurrentInterpYaw - LastInterpYaw;

	// 이번 프레임에 보간된 만큼 RootYawOffset에서 빼주어, 회전이 진행됨에 따라 Offset을 점차 줄여나갑니다.
	RootYawOffset += (TurnTargetYaw > 0.f ? -DeltaInterp : DeltaInterp);
	LastInterpYaw = CurrentInterpYaw;

	if (Alpha >= 1.f)
	{
		TurnTargetYaw = 0.f;
		TurnState = ETurnState::None;
	}
}

ULuxAnimInstance::ULuxAnimInstance(const FObjectInitializer& ObjectInitializer)
{

}

FAnimInstanceProxy* ULuxAnimInstance::CreateAnimInstanceProxy()
{
	return new FLuxAnimInstanceProxy(this);
}

void ULuxAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

void ULuxAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* OwningPawn = TryGetPawnOwner();
	if (!OwningPawn) return;

	OwningCharacter = Cast<ACharacter>(OwningPawn);
	if (!OwningCharacter) return;

	MovementComponent = OwningCharacter->GetCharacterMovement();
}

void ULuxAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}

void ULuxAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);


}


FVector FLuxAnimInstanceProxy::PredictGroundMovementStopLocation(
	const FVector& _CurrentLocation,
	const FVector& _Velocity,
	bool _bUseSeparateBrakingFriction,
	float _BrakingFriction,
	float _GroundFriction,
	float _BrakingFrictionFactor,
	float _BrakingDecelerationWalking)
{
	const float Speed2D = FVector(_Velocity.X, _Velocity.Y, 0).Size();
	if (Speed2D < 1.f)
	{
		return _CurrentLocation;
	}

	const float Friction = _bUseSeparateBrakingFriction ? _BrakingFriction : _GroundFriction;
	const float MaxBrakingDeceleration = FMath::Max(0.f, Friction * _BrakingFrictionFactor + _BrakingDecelerationWalking);

	if (MaxBrakingDeceleration <= 0.f)
	{
		return _CurrentLocation;
	}

	const float BrakingDistance = FMath::Square(Speed2D) / (2.f * MaxBrakingDeceleration);

	return _CurrentLocation + _Velocity.GetSafeNormal2D() * BrakingDistance;
}

float ULuxAnimInstance::GetTimeAtRemainingDistance(const UAnimSequence* Animation, float RemainingDistance) const
{
	if (!Animation || Animation->GetCurveData().FloatCurves.Num() == 0)
	{
		return 0.f;
	}

	const FFloatCurve* DistanceCurve = nullptr;
	const FName CurveToFindName = FName("DistanceCurve");

	for (const FFloatCurve& Curve : Animation->GetCurveData().FloatCurves)
	{
		if (Curve.GetName() == CurveToFindName)
		{
			DistanceCurve = &Curve;
			break;
		}
	}

	if (!DistanceCurve)
	{
		return 0.f;
	}

	// 최종적으로 반환될 시간을 저장할 변수
	float CalculatedTime = 0.f;

	const FRichCurve& RichCurve = DistanceCurve->FloatCurve;
	const TArray<FRichCurveKey>& Keys = RichCurve.GetConstRefOfKeys();

	// 키가 없는 경우를 대비
	if (Keys.Num() == 0)
	{
		return 0.f;
	}

	const float DistanceToFind = -RemainingDistance;
	const float MinDistance = Keys[0].Value;
	const float MaxDistance = Keys.Last().Value;

	if (DistanceToFind <= MinDistance)
	{
		CalculatedTime = Keys[0].Time;
	}
	else if (DistanceToFind >= MaxDistance)
	{
		CalculatedTime = Keys.Last().Time;
	}
	else
	{
		for (int32 i = 0; i < Keys.Num() - 1; ++i)
		{
			const FRichCurveKey& KeyA = Keys[i];
			const FRichCurveKey& KeyB = Keys[i + 1];

			if (DistanceToFind >= KeyA.Value && DistanceToFind <= KeyB.Value)
			{
				if (FMath::IsNearlyZero(KeyB.Value - KeyA.Value))
				{
					CalculatedTime = KeyA.Time;
				}
				else
				{
					const float Alpha = (DistanceToFind - KeyA.Value) / (KeyB.Value - KeyA.Value);
					CalculatedTime = FMath::Lerp(KeyA.Time, KeyB.Time, Alpha);
				}
				break; // 적절한 시간을 찾았으므로 루프 종료
			}
		}
	}

	// 함수 마지막에서 단 한번만 Clamp를 적용하여 안정성 보장
	return FMath::Clamp(CalculatedTime, 0.f, Animation->GetPlayLength());
}

UCharacterMovementComponent* ULuxAnimInstance::GetMovementComponent() const
{
	if (::IsValid(MovementComponent))
	{
		return MovementComponent;
	}

	ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (!Character) return nullptr;

	UCharacterMovementComponent* MC = Character->GetCharacterMovement();
	if (!MC) return nullptr;

	return MC;
}

void ULuxAnimInstance::ResetRootYawOffset()
{
	RootYawOffset = 0.f;
}
