// Fill out your copyright notice in the Description page of Project Settings.


#include "Cues/Aurora/CueNotify_Aurora_FrostShield.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

ACueNotify_Aurora_FrostShield::ACueNotify_Aurora_FrostShield()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    // 2. 비주얼 컴포넌트들을 담을 새로운 부모 SceneComponent를 만듭니다.
    USceneComponent* VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(RootComponent);

    // 3. 이 VisualRoot에 캐릭터와 동일한 90도 회전 오프셋을 적용합니다.
    VisualRoot->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

    // 4. 나머지 모든 비주얼/충돌 컴포넌트들을 VisualRoot에 부착합니다.
    CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(VisualRoot);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BottomMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottomMeshComponent"));
    BottomMeshComponent->SetupAttachment(VisualRoot);

    MiddleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiddleMeshComponent"));
    MiddleMeshComponent->SetupAttachment(VisualRoot);

    TopMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopMeshComponent"));
    TopMeshComponent->SetupAttachment(VisualRoot);

    FlareParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FlareParticleComponent"));
    FlareParticleComponent->SetupAttachment(VisualRoot);
    FlareParticleComponent->bAutoActivate = false;

    bAutoDestroyOnRemove = false;
}

void ACueNotify_Aurora_FrostShield::OnActive_Implementation(AActor* Target, const FLuxCueContext& Context)
{
    Super::OnActive_Implementation(Target, Context);

    // 블루프린트에서 설정된 애셋을 각 컴포넌트에 할당합니다.
    BottomMeshComponent->SetStaticMesh(BottomMesh);
    MiddleMeshComponent->SetStaticMesh(MiddleMesh);
    TopMeshComponent->SetStaticMesh(TopMesh);
    FlareParticleComponent->SetTemplate(FlareParticle);

    BottomMeshComponent->SetVisibility(true);
    MiddleMeshComponent->SetVisibility(true);
    TopMeshComponent->SetVisibility(true);
    FlareParticleComponent->Activate(true);

    /*
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ACueNotify_Aurora_FrostShield::OnShieldOverlap);
    CurrentHealth = MaxHealth;
    */
}

void ACueNotify_Aurora_FrostShield::OnStopped_Implementation(AActor* Target, const FLuxCueContext& Context)
{
    BottomMeshComponent->SetVisibility(false);
    MiddleMeshComponent->SetVisibility(false);
    TopMeshComponent->SetVisibility(false);
    FlareParticleComponent->Deactivate();

    /*
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ACueNotify_Aurora_FrostShield::OnShieldOverlap);
    */

    Super::OnStopped_Implementation(Target, Context);
}

void ACueNotify_Aurora_FrostShield::OnShieldOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
   
}
