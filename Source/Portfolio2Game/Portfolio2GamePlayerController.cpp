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
	// Call the base class  
	Super::BeginPlay();
}

void APortfolio2GamePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// (유지) Enhanced Input Component가 있는지 확인
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

	// 1. 마우스 아래에 있는 물체 감지 (Raycast)
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

			// ★ 함수 호출! (여기서 GridISM의 VisibleGrid가 실행됨)
			// GridSize는 딱히 필요 없다면 ZeroVector나 임의값 전달
			IMouseOverGridInterface::Execute_VisibleGrid(HitActor, HitResult.Location, FVector::ZeroVector);

			// 기억해둠 (나중에 나갔을 때 끄려고)
			LastHoveredGrid = CurrentGrid;
			return;
		}
	}

	// 3. 마우스가 허공에 있거나, 그리드가 아닌 다른 곳으로 갔다면?
	// 아까 기억해둔 그리드가 있으면 'HiddenGrid' 호출해서 끄기
	if (LastHoveredGrid)
	{
		IMouseOverGridInterface::Execute_HiddenGrid(LastHoveredGrid.GetObject());
		LastHoveredGrid = nullptr;
	}
}