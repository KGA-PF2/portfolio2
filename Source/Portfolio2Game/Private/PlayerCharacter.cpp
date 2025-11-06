#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h" // (신규) GAS 입력 바인딩을 위해 포함

APlayerCharacter::APlayerCharacter()
{
	bCanAct = false;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	// ❌ BattleManagerRef 찾기 로직 제거 (부모 클래스(CharacterBase)가 수행)
}

/**
 * (신규) 컨트롤러에 빙의될 때 ASC(AbilitySystemComponent)에 입력을 바인딩합니다.
 */
void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버에서 ASC 초기화
	if (AbilitySystem)
	{
		AbilitySystem->InitAbilityActorInfo(this, this);
	}
}

/**
 * (수정됨) WASD/스킬 입력을 이 클래스의 래퍼 함수에 연결합니다.
 */
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (AbilitySystem && PlayerInputComponent)
	{
		// 1. (오류 수정!) FAbilityInputBinding을 사용하지 않고,
		//    BindAction을 'this' (PlayerCharacter)의 멤버 함수에 바인딩합니다.

		static const FName InputName_MoveUp("MoveUp");
		static const FName InputName_MoveDown("MoveDown");
		static const FName InputName_MoveLeft("MoveLeft");
		static const FName InputName_MoveRight("MoveRight");

		PlayerInputComponent->BindAction(InputName_MoveUp, IE_Pressed, this, &APlayerCharacter::Input_MoveUp);
		PlayerInputComponent->BindAction(InputName_MoveDown, IE_Pressed, this, &APlayerCharacter::Input_MoveDown);
		PlayerInputComponent->BindAction(InputName_MoveLeft, IE_Pressed, this, &APlayerCharacter::Input_MoveLeft);
		PlayerInputComponent->BindAction(InputName_MoveRight, IE_Pressed, this, &APlayerCharacter::Input_MoveRight);

		// (참고) BindAbilityActivationToInputComponent는 이제 필요하지 않습니다.
	}
}

// ──────────────────────────────
// (신규) 입력 래퍼 함수 구현
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

// ──────────────────────────────
// 턴 관리
// ──────────────────────────────
void APlayerCharacter::EnableAction(bool bEnable)
{
	bCanAct = bEnable;
}

void APlayerCharacter::EndAction()
{
	if (!bCanAct) return;
	bCanAct = false;

	// (수정) BattleManagerRef는 부모 클래스(CharacterBase)의 변수를 사용
	if (BattleManagerRef)
	{
		BattleManagerRef->EndPlayerTurn();
	}
}

// ──────────────────────────────
// ❌ 기존 Handle 함수들 모두 제거
// ──────────────────────────────