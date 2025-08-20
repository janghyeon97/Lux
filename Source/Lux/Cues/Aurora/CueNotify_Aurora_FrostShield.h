// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Cues/LuxCueNotify.h"
#include "CueNotify_Aurora_FrostShield.generated.h"

class UStaticMeshComponent;
class UParticleSystemComponent;
class UCapsuleComponent;

/**
 * 
 */
UCLASS()
class LUX_API ACueNotify_Aurora_FrostShield : public ALuxCueNotify
{
	GENERATED_BODY()
	
public:
    ACueNotify_Aurora_FrostShield();

protected:
    //~ ALuxCueNotify Overrides
    virtual void OnActive_Implementation(AActor* Target, const FLuxCueContext& Context) override;
    virtual void OnStopped_Implementation(AActor* Target, const FLuxCueContext& Context) override;
    //~ End ALuxCueNotify Overrides

    /** 추후 투사체 방어 로직을 위한 함수 (지금은 비워둠) */
    UFUNCTION()
    void OnShieldOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
    /** 투사체 충돌을 감지할 캡슐 컴포넌트 */
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCapsuleComponent> CollisionComponent;

    /** 방패를 구성하는 스태틱 메시 컴포넌트들 */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BottomMeshComponent;

    UPROPERTY( BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MiddleMeshComponent;

    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> TopMeshComponent;

    /** 대쉬 파티클 효과 컴포넌트 */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TObjectPtr<UParticleSystemComponent> FlareParticleComponent;

    /** 블루프린트에서 설정할 애셋들 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
    TObjectPtr<UStaticMesh> BottomMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
    TObjectPtr<UStaticMesh> MiddleMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
    TObjectPtr<UStaticMesh> TopMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
    TObjectPtr<UParticleSystem> FlareParticle;

    /** 추후 체력 시스템을 위한 변수 (지금은 사용 안 함) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Shield")
    float CurrentHealth = 100.0f;
};
