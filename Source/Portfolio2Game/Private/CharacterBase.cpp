#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"
#include "BattleManager.h"      // (신규) BattleManager 포함
#include "Kismet/GameplayStatics.h" // (신규) GetActorOfClass를 위해 포함

ACharacterBase::ACharacterBase()
{
	// (수정) Tick을 사용하지 않으므로 비활성화합니다.
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("Attributes"));
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// (신규) BeginPlay 시 BattleManager를 찾아 저장합니다.
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
	}

	// ❌ Tick 관련 코드 모두 제거
}

// ❌ Tick 함수 제거

void ACharacterBase::InitAttributes()
{
	if (Attributes)
		Attributes->InitHealth(10.f);
}

// ───────── 스킬 보유 목록 → ASC 부여 (기존과 동일) ─────────
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

/**
 * (수정됨) 이 함수는 이제 단순히 데이터(GridCoord)만 업데이트합니다.
 * (HandleMove 같은 클릭 기반 이동에서 사용)
 */
void ACharacterBase::MoveToCell(FIntPoint Target)
{
	GridCoord = Target;
	// 월드 이동 매핑은 그리드 매니저에서 처리
}

void ACharacterBase::Turn(bool bRight)
{
	bFacingRight = bRight;
}

// ❌ OnMovementFinished 함수 제거

/**
 * (신규) BattleManager에게 물어봐서 현재 1D 인덱스를 가져옵니다. (요구사항 3)
 */
int32 ACharacterBase::GetGridIndex() const
{
	if (!BattleManagerRef)
	{
		return -1; // BattleManager 없음
	}

	// BattleManager의 유틸리티 함수를 호출하여 현재 2D 좌표(GridCoord)를 1D 인덱스로 변환
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