#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h" // (신규) GAS 입력 바인딩을 위해 포함
#include "InputMappingContext.h"    // (신규) Enhanced Input을 사용한다고 가정

// (신규) GAS 입력 바인딩을 위한 Enum 정의
// 프로젝트의 'GameplayTags' 설정과 일치해야 합니다.
// 예: "Input.Move.Forward", "Input.Move.Backward", ...
namespace PlayerAbilityInputID
{
	const int32 None = 0;
	const int32 MoveForward = 1;
	const int32 MoveBackward = 2;
	const int32 MoveLeft = 3;
	const int32 MoveRight = 4;
	const int32 ExecuteSkills = 5;
	const int32 CancelSkills = 6;
	// ... (기타 스킬 예약 등)
}


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
 * (수정됨) WASD/스킬 입력을 ASC의 어빌리티 태그에 연결합니다.
 */
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// (참고) Enhanced Input을 사용한다고 가정합니다.
	// 만약 Legacy Input을 사용한다면 'BindAction'을 사용해야 합니다.

	if (AbilitySystem)
	{
		// 1. AbilitySystemComponent에 입력 ID를 바인딩합니다.
		AbilitySystem->BindAbilityActivationToInputComponent(PlayerInputComponent,
			FGameplayAbilityInputBinds(
				"Confirm", // GameplayAbility "Confirm" 입력 (예: 스킬 실행)
				"Cancel",  // GameplayAbility "Cancel" 입력 (예: 예약 취소)
				"PlayerAbilityInputID", // 위에서 정의한 Enum의 이름
				static_cast<int32>(PlayerAbilityInputID::None), // Enum의 첫 번째/기본값
				static_cast<int32>(PlayerAbilityInputID::None)  // Enum의 첫 번째/기본값
			)
		);

		// 2. 실제 입력 액션과 Enum ID를 연결합니다.
		// (이 부분은 Enhanced Input 또는 Legacy Input 설정에 따라 달라집니다.)
		// 예시 (Legacy Input):
		// PlayerInputComponent->BindAction("MoveForward", IE_Pressed, this, &APlayerCharacter::Input_MoveForward);
		// PlayerInputComponent->BindAction("MoveBackward", IE_Pressed, this, &APlayerCharacter::Input_MoveBackward);
		// ...

		// 3. (필수!) AbilitySystemComponent에 어떤 어빌리티가 어떤 입력 ID에 반응할지 알려줘야 합니다.
		// 이 작업은 ACharacterBase::GiveAllSkills() 함수 또는 BeginPlay에서
		// FGameplayAbilitySpec에 InputID를 할당하여 수행합니다.
		//
		// 예: ACharacterBase::GiveAllSkills() 내부 수정
		// FGameplayAbilitySpec Spec(GA_Move_Forward_Class, 1, PlayerAbilityInputID::MoveForward, this);
		// AbilitySystem->GiveAbility(Spec);
	}
}

void APlayerCharacter::EnableAction(bool bEnable)
{
	bCanAct = bEnable;
}

/**
 * (수정됨) GA_Move 같은 어빌리티가 이동/모션을 완료한 후 "직접" 이 함수를 호출해야 합니다.
 */
void APlayerCharacter::EndAction()
{
	// 턴이 활성화되어 있을 때만 턴을 종료할 수 있습니다.
	if (!bCanAct) return;

	bCanAct = false;

	// BattleManagerRef는 부모 클래스(CharacterBase)의 변수를 사용합니다.
	if (BattleManagerRef)
	{
		BattleManagerRef->EndPlayerTurn();
	}
}


