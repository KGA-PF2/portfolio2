// Portfolio2GamePlayerController.cpp

#include "Portfolio2GamePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Portfolio2GameCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

APortfolio2GamePlayerController::APortfolio2GamePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void APortfolio2GamePlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void APortfolio2GamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (Cast<UEnhancedInputComponent>(InputComponent))
	{
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void APortfolio2GamePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		// 2. 그 물체가 '마우스 오버 인터페이스'를 가지고 있는지 확인
		if (HitActor->Implements<UMouseOverGridInterface>())
		{
			// 인터페이스 형태로 변환
			TScriptInterface<IMouseOverGridInterface> CurrentGrid;
			CurrentGrid.SetObject(HitActor);
			CurrentGrid.SetInterface(Cast<IMouseOverGridInterface>(HitActor));
			IMouseOverGridInterface::Execute_VisibleGrid(HitActor, HitResult.Location, FVector::ZeroVector);

			LastHoveredGrid = CurrentGrid;
			return;
		}
	}

	if (LastHoveredGrid)
	{
		IMouseOverGridInterface::Execute_HiddenGrid(LastHoveredGrid.GetObject());
		LastHoveredGrid = nullptr;
	}
}