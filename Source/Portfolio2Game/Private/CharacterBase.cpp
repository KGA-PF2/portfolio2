#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"
#include "BattleManager.h"      // (신규) BattleManager 포함
#include "PlayerCharacter.h"    // (신규) GiveMoveAbilities에서 InputID를 사용하기 위해 포함
#include "Kismet/GameplayStatics.h" // (신규) GetActorOfClass를 위해 포함

ACharacterBase::ACharacterBase()
{
	// (수정) Tick을 사용하지 않으므로 비활성화
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("Attributes"));
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// (신규) BeginPlay 시 BattleManager를 찾아 저장
	BattleManagerRef = Cast<ABattleManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));

	if (!BattleManagerRef)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: 맵에서 BattleManager를 찾을 수 없습니다!"), *GetName());
	}

	if (AbilitySystem)
		AbilitySystem->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		InitAttributes();
		GiveAllSkills();
		GiveMoveAbilities(); // (신규) 이동 어빌리티 부여
	}

	// ❌ Tick 관련 코드 모두 제거
}

// ❌ Tick 함수 제거

void ACharacterBase::InitAttributes()
{
	if (Attributes)
		Attributes->InitHealth(10.f);
}

// ───────── 스킬 보유 목록 → ASC 부여 ─────────
void ACharacterBase::GiveAllSkills()
{
	if (!AbilitySystem || !HasAuthority()) return;

	GrantedSkillHandles.Reset();

	for (auto& SkillClass : SkillList)
	{
		if (!SkillClass) continue;
		FGameplayAbilitySpecHandle Handle =
			AbilitySystem->GiveAbility(FGameplayAbilitySpec(SkillClass, 1, INDEX_NONE, this));
		GrantedSkillHandles.Add(Handle);
	}
}

/**
 * (신규) PlayerCharacter에 정의된 InputID와 연결하여 이동 어빌리티를 부여합니다.
 */
void ACharacterBase::GiveMoveAbilities()
{
	if (!AbilitySystem || !HasAuthority()) return;

	// PlayerAbilityInputID Enum 값은 PlayerCharacter.h에 정의될 예정입니다.
	// (1: Fwd, 2: Bwd, 3: Left, 4: Right)
	if (MoveAbility_Up)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(MoveAbility_Up, 1, 1, this));
	}
	if (MoveAbility_Down)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(MoveAbility_Down, 1, 2, this));
	}
	if (MoveAbility_Left)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(MoveAbility_Left, 1, 3, this));
	}
	if (MoveAbility_Right)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(MoveAbility_Right, 1, 4, this));
	}
}

// ───────── 예약/실행/취소 (기존과 동일) ─────────
void ACharacterBase::ReserveSkill(TSubclassOf<UGameplayAbility> SkillClass)
{
	if (!SkillClass) return;
	if (!SkillList.Contains(SkillClass)) return;
	if (QueuedSkills.Num() >= MaxQueuedSkills) return;
	QueuedSkills.Add(SkillClass);
}

void ACharacterBase::ExecuteSkillQueue()
{
	if (!AbilitySystem || QueuedSkills.Num() == 0) return;
	for (TSubclassOf<UGameplayAbility> SkillClass : QueuedSkills)
	{
		if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(SkillClass))
		{
			AbilitySystem->TryActivateAbility(Spec->Handle);
		}
	}
}

void ACharacterBase::CancelSkillQueue()
{
	QueuedSkills.Empty();
}

// ───────── 이동/회전 (수정됨) ─────────
void ACharacterBase::MoveToCell(FIntPoint Target)
{
	// (수정) 이 함수는 이제 GA_Move가 아닌 다른 방식(예: 스킬)으로
	// 캐릭터의 좌표만 강제 이동시킬 때 사용합니다.
	GridCoord = Target;

	// (신규) BattleManager에게도 좌표가 변경되었음을 알려야 합니다.
	if (BattleManagerRef)
	{
		// (참고) BattleManager에 UpdateCharacterOccupancy 함수를 추가하는 것이 좋습니다.
		// (현재는 GetCharacterAt이 실시간으로 검색하므로 필수 아님)
	}
}

void ACharacterBase::Turn(bool bRight)
{
	bFacingRight = bRight;
}

// ❌ OnMovementFinished 함수 제거

/**
 * (신규) BattleManager에게 물어봐서 현재 1D 인덱스를 가져옵니다.
 */
int32 ACharacterBase::GetGridIndex() const
{
	if (!BattleManagerRef) return -1;

	// BattleManager의 유틸리티 함수(GetGridIndexFromCoord)를 호출합니다.
	return BattleManagerRef->GetGridIndexFromCoord(GridCoord);
}

// ───────── 대미지/사망 (기존과 동일) ─────────
void ACharacterBase::ApplyDamage(float Amount)
{
	if (!Attributes) return;
	const float NewHP = Attributes->GetHP() - Amount;
	Attributes->SetHP(NewHP);
	if (NewHP <= 0.f)
		Die();
}

void ACharacterBase::Die()
{
	bDead = true;
	Destroy();
}