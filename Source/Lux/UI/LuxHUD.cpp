// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LuxHUD.h"
#include "Engine/Canvas.h"
#include "Game/LuxPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseTags.h"
#include "ActionSystem/Effects/LuxEffect.h"

#include "Character/LuxHeroCharacter.h" 
#include "Camera/LuxCameraComponent.h"  
#include "Camera/LuxCameraMode.h"       
#include "Camera/LuxCameraMode_Focus.h"

namespace LuxHUDConstants
{
	constexpr float PosX = 50.0f;
	constexpr float LineHeight = 16.0f;
}


ALuxHUD::ALuxHUD()
{
}

void ALuxHUD::DrawHUD()
{
    Super::DrawHUD();

    ALuxPlayerController* PC = Cast<ALuxPlayerController>(GetOwningPlayerController());
    if (PC && PC->bShowActionSystemDebug) 
    {
        DrawActionSystemDebugInfo();
		DrawCameraDebugInfo();
    }
}

void ALuxHUD::DrawActionSystemDebugInfo()
{
	if (!Canvas || !GEngine) return;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	APawn* ControlledPawn = PC->GetPawn();
	if (!ControlledPawn)
	{
		FCanvasTextItem TextItem(FVector2D(LuxHUDConstants::PosX, 50.f), FText::FromString("No Pawn Controlled"), GEngine->GetSmallFont(), FLinearColor::Red);
		Canvas->DrawItem(TextItem);
		return;
	}

	IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(ControlledPawn);
	if (!ASCInterface) return;

	UActionSystemComponent* ASC = ASCInterface->GetActionSystemComponent();
	if (!ASC)
	{
		FCanvasTextItem TextItem(FVector2D(LuxHUDConstants::PosX, 50.f), FText::FromString("ActionSystemComponent Not Found"), GEngine->GetSmallFont(), FLinearColor::Red);
		Canvas->DrawItem(TextItem);
		return;
	}

	float CurrentPosY = 50.0f;

	DrawDebugHeader(ASC, CurrentPosY);	
	DrawAttributesInfo(ASC, CurrentPosY);
	DrawGrantedActionsInfo(ASC, CurrentPosY);
	DrawActiveActionsInfo(ASC, CurrentPosY);
	DrawActiveEffectsInfo(ASC, CurrentPosY);
	DrawGrantedTagsInfo(ASC, CurrentPosY);
}

void ALuxHUD::DrawDebugHeader(const UActionSystemComponent* ASC, float& PosY) const
{
	APlayerController* PC = GetOwningPlayerController();
	FString RoleString = PC->HasAuthority() ? TEXT("[SERVER]") : TEXT("[CLIENT]");
	FLinearColor HeaderColor = PC->HasAuthority() ? FLinearColor::Green : FLinearColor::Blue;

	FText TitleText = FText::FromString(FString::Printf(TEXT("%s Action System Debug - %s"), *RoleString, *GetNameSafe(ASC->GetOwnerActor())));
	FCanvasTextItem TextItem(FVector2D(LuxHUDConstants::PosX, PosY), TitleText, GEngine->GetLargeFont(), HeaderColor);
	TextItem.Scale = FVector2D(1.5f, 1.5f);

	Canvas->DrawItem(TextItem);
	PosY += LuxHUDConstants::LineHeight * 2.5f;
}

void ALuxHUD::DrawAttributesInfo(const UActionSystemComponent* ASC, float& PosY) const
{
	const UFont* Font = GEngine->GetSmallFont();
	const FLinearColor HeaderColor = ASC->GetOwner()->HasAuthority() ? FLinearColor::Green : FLinearColor::Blue;

	FCanvasTextItem HeaderItem(FVector2D(LuxHUDConstants::PosX, PosY), FText::FromString(TEXT("--- ATTRIBUTES ---")), Font, HeaderColor);
	Canvas->DrawItem(HeaderItem);
	PosY += LuxHUDConstants::LineHeight;

	TSet<FName> DrawnAttributeNames;
	for (const ULuxAttributeSet* Set : ASC->GetSpawnedAttributes())
	{
		if (!Set) continue;

		for (TFieldIterator<FProperty> PropIt(Set->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			if (DrawnAttributeNames.Contains(Property->GetFName()))
				continue;

			if (Property->HasMetaData(TEXT("bHiddenInGame")))
				continue;

			FStructProperty* StructProp = CastField<FStructProperty>(*PropIt);
			if (!StructProp) continue;

			if (StructProp->Struct != FLuxAttributeData::StaticStruct())
				continue;

			const FLuxAttributeData* Data = StructProp->ContainerPtrToValuePtr<FLuxAttributeData>(Set);
			if (!Data) continue;

			FString AttrText = FString::Printf(TEXT("  - %s: %.1f / %.1f"), *(*PropIt)->GetName(), Data->GetCurrentValue(), Data->GetBaseValue());
			FCanvasTextItem AttrItem(FVector2D(LuxHUDConstants::PosX, PosY), FText::FromString(AttrText), Font, FLinearColor::White);
			Canvas->DrawItem(AttrItem);
			PosY += LuxHUDConstants::LineHeight;
		}
	}

	PosY += LuxHUDConstants::LineHeight;
}

void ALuxHUD::DrawGrantedActionsInfo(const UActionSystemComponent* ASC, float& PosY) const
{
	const UFont* Font = GEngine->GetSmallFont();
	const FLinearColor HeaderColor = ASC->GetOwner()->HasAuthority() ? FLinearColor::Green : FLinearColor::Blue;
	const FLinearColor CooldownColor = FLinearColor(211.f/255.f, 131.f/255.f, 15.f/255.f);

	FCanvasTextItem HeaderItem(FVector2D(LuxHUDConstants::PosX, PosY), FText::FromString(TEXT("--- GRANTED ACTIONS ---")), Font, HeaderColor);
	Canvas->DrawItem(HeaderItem);
	PosY += LuxHUDConstants::LineHeight;

	for (const FLuxActionSpec& Spec : ASC->GetActionSpecs())
	{
		FString CooldownStatus;
		FLinearColor TextColor = FLinearColor::White;

		if (Spec.Action && Spec.Action->Cooldown)
		{
			if (ASC->HasTag(Spec.GetCooldownTag()))
			{
				CooldownStatus = TEXT(" (ON COOLDOWN)");
				TextColor = CooldownColor;
			}
		}

		FText SpecText = FText::FromString(FString::Printf(TEXT("  - %s (Lvl: %d, Input: %s)%s"), *GetNameSafe(Spec.Action), Spec.Level, *Spec.InputTag.ToString(), *CooldownStatus));
		FCanvasTextItem SpecItem(FVector2D(LuxHUDConstants::PosX, PosY), SpecText, Font, TextColor);
		Canvas->DrawItem(SpecItem);
		PosY += LuxHUDConstants::LineHeight;
	}

	PosY += LuxHUDConstants::LineHeight;
}

void ALuxHUD::DrawActiveActionsInfo(const UActionSystemComponent* ASC, float& PosY) const
{
	const UFont* Font = GEngine->GetSmallFont();
	const FLinearColor HeaderColor = ASC->GetOwner()->HasAuthority() ? FLinearColor::Green : FLinearColor::Blue;
	const FLinearColor ActionColor = FLinearColor(1.0f, 1.0f, 0.4f); // 밝은 노란색
	const FLinearColor TaskColor = FLinearColor(0.4f, 1.0f, 1.0f);   // 밝은 청록색 (Cyan)

	FCanvasTextItem HeaderItem(FVector2D(LuxHUDConstants::PosX, PosY), FText::FromString(TEXT("--- ACTIVE ACTIONS ---")), Font, HeaderColor);
	Canvas->DrawItem(HeaderItem);
	PosY += LuxHUDConstants::LineHeight;

	for (const FActiveLuxAction& ActiveAction : ASC->GetActiveActions())
	{
		const float TimeActive = GetWorld() ? GetWorld()->GetTimeSeconds() - ActiveAction.StartTime : 0.f;
		FText ActionText = FText::FromString(FString::Printf(TEXT("  - %s (Time: %.1fs, Phase: %s)"),
			*GetNameSafe(ActiveAction.Action),
			TimeActive,
			*ActiveAction.Action->CurrentPhaseTag.ToString()));

		// 액션 정보 텍스트 색상 변경
		FCanvasTextItem ActionItem(FVector2D(LuxHUDConstants::PosX, PosY), ActionText, Font, ActionColor);
		Canvas->DrawItem(ActionItem);
		PosY += LuxHUDConstants::LineHeight;

		if (ActiveAction.Action)
		{
			for (const ULuxActionTask* Task : ActiveAction.Action->ActiveTasks)
			{
				if (Task)
				{
					FText TaskText = FText::FromString(FString::Printf(TEXT("    └ Task: %s"), *Task->TaskName.ToString()));

					// 태스크 정보 텍스트 색상 변경
					FCanvasTextItem TaskItem(FVector2D(LuxHUDConstants::PosX, PosY), TaskText, Font, TaskColor);
					Canvas->DrawItem(TaskItem);
					PosY += LuxHUDConstants::LineHeight;
				}
			}
		}
	}

	PosY += LuxHUDConstants::LineHeight;
}

void ALuxHUD::DrawActiveEffectsInfo(const UActionSystemComponent* ASC, float& PosY) const
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	const UFont* Font = GEngine->GetSmallFont();
	const FLinearColor HeaderColor = ASC->GetOwner()->HasAuthority() ? FLinearColor::Green : FLinearColor::Blue;

	FCanvasTextItem HeaderItem(FVector2D(LuxHUDConstants::PosX, PosY), FText::FromString(TEXT("--- ACTIVE EFFECTS ---")), Font, HeaderColor);
	Canvas->DrawItem(HeaderItem);
	PosY += LuxHUDConstants::LineHeight;

	for (const FActiveLuxEffect& ActiveEffect : ASC->GetActiveEffects())
	{
		FString DurationText;

		// 이펙트가 지속시간을 가지는 경우에만 남은 시간을 계산하고 표시합니다.
		if (ActiveEffect.Spec.EffectTemplate->DurationPolicy == ELuxEffectDurationPolicy::HasDuration)
		{
			const float CurrentServerTime = GameState->GetServerWorldTimeSeconds();
			const float TimeRemaining = FMath::Max(0.f, ActiveEffect.EndTime - CurrentServerTime);
			const float FullDuration = ActiveEffect.Spec.CalculatedDuration;

			DurationText = FString::Printf(TEXT(" (Duration: %.1fs, Remain: %.1fs)"), FullDuration, TimeRemaining);
		}

		FText EffectText = FText::FromString(FString::Printf(TEXT("  - %s (Stacks: %d%s)"), *GetNameSafe(ActiveEffect.Spec.EffectTemplate.Get()), ActiveEffect.CurrentStacks, *DurationText));
		FCanvasTextItem EffectItem(FVector2D(LuxHUDConstants::PosX, PosY), EffectText, Font, FLinearColor::White);
		Canvas->DrawItem(EffectItem);
		PosY += LuxHUDConstants::LineHeight;
	}

	PosY += LuxHUDConstants::LineHeight;
}

void ALuxHUD::DrawGrantedTagsInfo(const UActionSystemComponent* ASC, float& PosY) const
{
	const UFont* Font = GEngine->GetSmallFont();
	const FLinearColor HeaderColor = ASC->GetOwner()->HasAuthority() ? FLinearColor::Green : FLinearColor::Blue;

	FCanvasTextItem HeaderItem(FVector2D(LuxHUDConstants::PosX, PosY), FText::FromString(TEXT("--- GRANTED TAGS ---")), Font, HeaderColor);
	Canvas->DrawItem(HeaderItem);
	PosY += LuxHUDConstants::LineHeight;

	for (const FGameplayTag& Tag : ASC->GetGameplayTags())
	{
		const int32 StackCount = ASC->GetTagStackCount(Tag);
		FText TagText = FText::FromString(FString::Printf(TEXT("  - %s (Count: %d)"), *Tag.ToString(), StackCount));
		
		FCanvasTextItem TagItem(FVector2D(LuxHUDConstants::PosX, PosY), TagText, Font, FLinearColor::White);
		Canvas->DrawItem(TagItem);
		PosY += LuxHUDConstants::LineHeight;
	}
}


void ALuxHUD::DrawCameraDebugInfo()
{
	if (!Canvas || !GEngine) return;

	ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(GetOwningPawn());
	if (!Hero) return;

	ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent();
	if (!CameraComponent) return;

	// ActionSystem 디버그와 동일한 폰트 및 기본 색상 사용
	const UFont* Font = GEngine->GetSmallFont();
	const FLinearColor HeaderColor = FLinearColor::Yellow;
	const FLinearColor TextColor = FLinearColor::White;
	const FLinearColor ActiveColor = FLinearColor::Green;
	const FLinearColor EnabledColor = FLinearColor(0.0f, 1.0f, 1.0f);	//Cyan
	const FLinearColor DisabledColor = FLinearColor(0.5f, 0.5f, 0.5f); // 회색

	// 우측 상단 기준 위치 및 간격 재조정
	float CurrentPosY = 100.0f;
	const float StartX = Canvas->SizeX - 350.f;
	const float LineHeight = LuxHUDConstants::LineHeight;
	const float SectionSpacing = LineHeight * 1.5f;

	// --- 헤더 ---
	FCanvasTextItem HeaderItem(
		FVector2D(StartX, CurrentPosY),
		FText::FromString(TEXT("--- CAMERA DEBUG ---")),
		Font,
		HeaderColor
	);
	Canvas->DrawItem(HeaderItem);
	CurrentPosY += SectionSpacing;

	// --- 현재 활성화된 모드 ---
	const TSubclassOf<ULuxCameraMode> CurrentModeClass = CameraComponent->GetCurrentCameraMode();
	const ULuxCameraMode* CurrentModeCDO = CurrentModeClass ? CurrentModeClass->GetDefaultObject<ULuxCameraMode>() : nullptr;

	FString ActiveModeStr = FString::Printf(TEXT("Active Mode: %s"), *GetNameSafe(CurrentModeClass));
	FCanvasTextItem ActiveModeItem(FVector2D(StartX, CurrentPosY), FText::FromString(ActiveModeStr), Font, ActiveColor);
	Canvas->DrawItem(ActiveModeItem);
	CurrentPosY += LineHeight;

	// --- 포커스 모드 ---
	const ULuxCameraMode_Focus* FocusMode = CameraComponent->GetFocusModeInstance();
	FString FocusModeStr = FString::Printf(TEXT("Focus Mode: %s"), FocusMode ? *GetNameSafe(FocusMode->GetClass()) : TEXT("None"));
	FCanvasTextItem FocusModeItem(FVector2D(StartX, CurrentPosY), FText::FromString(FocusModeStr), Font, FocusMode ? EnabledColor : DisabledColor);
	Canvas->DrawItem(FocusModeItem);
	CurrentPosY += LineHeight;

	// --- Lag 서브시스템 정보 ---
	if (CurrentModeCDO)
	{
		const bool bIsLagEnabled = CurrentModeCDO->bUseLagSubsystem;
		FString LagHeaderStr = FString::Printf(TEXT("Lag Subsystem: %s"), bIsLagEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
		FCanvasTextItem LagHeaderItem(
			FVector2D(StartX, CurrentPosY),
			FText::FromString(LagHeaderStr),
			Font,
			bIsLagEnabled ? EnabledColor : DisabledColor
		);
		Canvas->DrawItem(LagHeaderItem);
		CurrentPosY += LineHeight;

		if (bIsLagEnabled)
		{
			FString LagSpeedStr = FString::Printf(TEXT(" > Lag Speed: %.1f"), CurrentModeCDO->LagSpeed);
			FCanvasTextItem LagSpeedItem(FVector2D(StartX, CurrentPosY), FText::FromString(LagSpeedStr), Font, TextColor);
			Canvas->DrawItem(LagSpeedItem);
			CurrentPosY += LineHeight;
		}
	}
	CurrentPosY += SectionSpacing;

	// --- 카메라 스택 목록 ---
	const TArray<TSubclassOf<ULuxCameraMode>>& CameraStack = CameraComponent->GetCameraModeStack();
	FString StackHeaderStr = FString::Printf(TEXT("Camera Mode Stack (Size: %d)"), CameraStack.Num());
	FCanvasTextItem StackHeaderItem(FVector2D(StartX, CurrentPosY), FText::FromString(StackHeaderStr), Font, HeaderColor);
	Canvas->DrawItem(StackHeaderItem);
	CurrentPosY += LineHeight;

	for (int32 i = CameraStack.Num() - 1; i >= 0; --i)
	{
		const TSubclassOf<ULuxCameraMode> ModeClass = CameraStack[i];
		const bool bIsActive = (ModeClass == CurrentModeClass);
		const FString Prefix = bIsActive ? TEXT("-> ") : TEXT("   ");
		const FString StackEntryStr = FString::Printf(TEXT("%s[%d] %s"), *Prefix, i, *GetNameSafe(ModeClass));

		FCanvasTextItem StackEntryItem(FVector2D(StartX, CurrentPosY), FText::FromString(StackEntryStr), Font, bIsActive ? ActiveColor : TextColor);
		Canvas->DrawItem(StackEntryItem);
		CurrentPosY += LineHeight;
	}
}