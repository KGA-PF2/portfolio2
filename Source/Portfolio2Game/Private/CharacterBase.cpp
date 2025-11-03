#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"

ACharacterBase::ACharacterBase()
{
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    Attributes = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("Attributes"));
}

void ACharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystem)
        AbilitySystem->InitAbilityActorInfo(this, this);

    if (HasAuthority())
    {
        InitAttributes();
        GiveAllSkills(); // 보유 스킬 목록을 실제로 부여
    }
}

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

// ───────── 예약/실행/취소 ─────────
void ACharacterBase::ReserveSkill(TSubclassOf<UGameplayAbility> SkillClass)
{
    if (!SkillClass) return;

    // 보유 목록에 없는 스킬은 예약 불가(보안/일관성)
    if (!SkillList.Contains(SkillClass)) return;

    if (QueuedSkills.Num() >= MaxQueuedSkills) return;
    QueuedSkills.Add(SkillClass);
}

void ACharacterBase::ExecuteSkillQueue()
{
    if (!AbilitySystem || QueuedSkills.Num() == 0) return;

    // 예약된 스킬을 선입선출로 “전부” 발동. 큐는 유지(룰에 따라 유지).
    for (TSubclassOf<UGameplayAbility> SkillClass : QueuedSkills)
    {
        if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(SkillClass))
        {
            AbilitySystem->TryActivateAbility(Spec->Handle);
        }
    }
    // ※ 큐는 유지. 취소 전까지 그대로.
}

void ACharacterBase::CancelSkillQueue()
{
    QueuedSkills.Empty();
}

// ───────── 이동/회전 ─────────
void ACharacterBase::MoveToCell(FIntPoint Target)
{
    GridCoord = Target;
    // 월드 이동 매핑은 그리드 매니저에서 처리
}

void ACharacterBase::Turn(bool bRight)
{
    bFacingRight = bRight;
}

// ───────── 대미지/사망 ─────────
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
