// PlayerCharacter.cpp

#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h" // GAS 입력 바인딩을 위해 포함

// (신규) Enhanced Input 관련 헤더
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h" // GetLocalPlayer()를 위해 필요

APlayerCharacter::APlayerCharacter()
{
	bCanAct = false;
}

// ❌ (제거) BeginPlay() (부모 클래스(CharacterBase)가 BattleManager를 찾음)

/**
 * (수정됨) 컨트롤러에 빙의될 때 ASC 초기화 및 Enhanced Input 컨텍스트를 추가합니다.
 */
void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 1. 서버에서 ASC 초기화
	if (AbilitySystem)
	{
		AbilitySystem->InitAbilityActorInfo(this, this);
	}

	// 2. (신규) 로컬 플레이어 컨트롤러에만 WASD용 매핑 컨텍스트 추가
	APlayerController* PC = Cast<APlayerController>(NewController);
	if (PC)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// (중요) PlayerMoveContext를 추가합니다. (BP의 컨텍스트(0)보다 높은 Priority(1)로 설정)
			if (PlayerMoveContext)
			{
				Subsystem->AddMappingContext(PlayerMoveContext, 1);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: 'PlayerMoveContext'가 할당되지 않았습니다. BP_PlayerCharacter에서 설정하세요."), *GetName());
			}
		}
	}
}

/**
 * (수정됨) Enhanced Input Action을 래퍼 함수에 연결합니다.
 */
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// (신규) Enhanced Input Component로 캐스팅
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// (신규) IA_Move... 액션을 Input_Move... 함수에 바인딩
		if (IA_MoveUp)
		{
			EnhancedInputComponent->BindAction(IA_MoveUp, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveUp);
		}
		if (IA_MoveDown)
		{
			EnhancedInputComponent->BindAction(IA_MoveDown, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveDown);
		}
		if (IA_MoveLeft)
		{
			EnhancedInputComponent->BindAction(IA_MoveLeft, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveLeft);
		}
		if (IA_MoveRight)
		{
			EnhancedInputComponent->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveRight);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Enhanced Input Component를 찾지 못했습니다! PlayerController 또는 Project 세팅을 확인하세요."), *GetNameSafe(this));
	}
}

// ──────────────────────────────
// (기존) 입력 래퍼 함수 구현 (수정 없음)
// ──────────────────────────────
void APlayerCharacter::Input_MoveUp()
{
	if (AbilitySystem)
	{
		// ASC에 InputID(1)를 눌렀다고 알립니다.
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveUp);
	}
}

void APlayerCharacter::Input_MoveDown()
{
	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveDown);
	}
}

void APlayerCharacter::Input_MoveLeft()
{
	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveLeft);
	}
}

void APlayerCharacter::Input_MoveRight()
{
	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveRight);
	}
}