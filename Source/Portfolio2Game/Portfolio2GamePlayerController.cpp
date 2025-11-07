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

	// ❌ (제거) CachedDestination, FollowTime 초기화
}

void APortfolio2GamePlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// (유지) BP_TestPlayerController(자식)가 사용할 기본 컨텍스트(카메라, 하이라이트용)를 추가합니다.
	// (Priority 0으로 설정)
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APortfolio2GamePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// (유지) Enhanced Input Component가 있는지 확인
	if (Cast<UEnhancedInputComponent>(InputComponent))
	{
		// ❌ (제거) Setup mouse/touch input events 관련 BindAction 모두 삭제
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

// ❌ (제거) OnInputStarted, OnSetDestinationTriggered, OnSetDestinationReleased, OnTouchTriggered, OnTouchReleased 함수 5개 모두 삭제