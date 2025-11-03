#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"
#include "GA_Move.h"

ACharacterBase::ACharacterBase()
{
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    Attributes = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("Attributes"));
}

void ACharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystem)
    {
        AbilitySystem->InitAbilityActorInfo(this, this);
    }

    if (HasAuthority())
    {
        InitAttributes();
        GiveAllSkills(); // 공통 스킬 목록 자동 등록
    }
}

void ACharacterBase::InitAttributes()
{
    if (Attributes)
    {
        Attributes->InitHealth(10.f);
    }
}

void ACharacterBase::GiveAllSkills()
{
    if (!AbilitySystem || !HasAuthority()) return;

    for (auto& SkillClass : SkillList)
    {
        if (!SkillClass) continue;

        FGameplayAbilitySpecHandle Handle =
            AbilitySystem->GiveAbility(FGameplayAbilitySpec(SkillClass, 1, INDEX_NONE, this));

        GrantedSkillHandles.Add(Handle);
    }

    // 이동 스킬은 모든 캐릭터 공통
    AbilitySystem->GiveAbility(FGameplayAbilitySpec(UGA_Move::StaticClass(), 1, 0, this));
}

void ACharacterBase::ApplyDamage(float Amount)
{
    if (!Attributes) return;

    const float NewHP = Attributes->GetHP() - Amount;
    Attributes->SetHP(NewHP);

    if (NewHP <= 0.f)
    {
        Die();
    }
}

void ACharacterBase::Die()
{
    bDead = true;
    Destroy();
}
