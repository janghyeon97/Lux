// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Action/Aurora/FrozenSimulacrum.h"
#include "Components/StaticMeshComponent.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Attributes/ResourceSet.h" 
#include "ActionSystem/Actions/Aurora/AuroraAction_FrozenSimulacrum.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "LuxLogChannels.h"

AFrozenSimulacrum::AFrozenSimulacrum()
{
    PrimaryActorTick.bCanEverTick = false;

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel10);
        Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel10, ECollisionResponse::ECR_Overlap);
        Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
    }

    ResourceSet = CreateDefaultSubobject<UResourceSet>(TEXT("ResourceSet"));
}

void AFrozenSimulacrum::Initialize(ULuxAction* Action, bool bAutoStart)
{
    if (!HasAuthority())
    {
        return;
    }

    SourceAction = Action;
    if (!SourceAction.IsValid()) 
    {
        UE_LOG(LogLux, Error, TEXT("'%s' Initialize: 유효한 액션 정보를 찾을 수 없습니다."), *GetName());
        return;
    }

    SourceASC = Action->GetActionSystemComponent();
    if (!SourceASC.IsValid())
    {
        UE_LOG(LogLux, Error, TEXT("'%s' Initialize: 유효한 액션 시스템 컴포넌트를 찾을 수 없습니다."), *GetName());
        return;
    }

    FLuxActionSpec* Spec = Action->GetLuxActionSpec();
    if (!Spec)
    {
        UE_LOG(LogLux, Error, TEXT("'%s' Initialize: 유효한 액션 스펙을 찾을 수 없습니다."), *GetName());
        return;
    }

    ActionLevel = Spec->Level;
    ActionSpecHandle = Spec->Handle;
    ActionIdentifierTag = Action->ActionIdentifierTag;

    const FName RowName = FName(*FString::FromInt(ActionLevel));
    const FLuxActionLevelData* LevelData = Action->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
    if (!LevelData)
    {
        UE_LOG(LogLux, Error, TEXT("'%s' Initialize: 레벨 %d 데이터를 찾을 수 없습니다."), *GetName(), ActionLevel);
        return;
    }

    const FAuroraActionLevelData_FrozenSimulacrum* SimulacrumData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_FrozenSimulacrum>();
    if (!SimulacrumData)
    {
        UE_LOG(LogLux, Error, TEXT("'%s' Initialize: LevelData가 FAuroraActionLevelData_FrozenSimulacrum 타입이 아닙니다."), *GetName());
        return;
    }

    // 2. 체력 설정
    const UResourceSet* CasterResourceSet = SourceASC->GetAttributeSet<UResourceSet>();
    UResourceSet* MyResourceSet = SourceASC->GetMutableAttributeSet<UResourceSet>();

    if (CasterResourceSet && MyResourceSet)
    {
        const float FinalMaxHealth = SimulacrumData->SimulacrumBaseMaxHealth + (CasterResourceSet->GetMaxHealth() * SimulacrumData->SimulacrumHealthScale_CasterMaxHealth);
        MyResourceSet->SetMaxHealth(FinalMaxHealth);
        MyResourceSet->SetHealth(FinalMaxHealth);
    }

    SetLifeSpan(SimulacrumData->SimulacrumDuration);
}

void AFrozenSimulacrum::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);


}
