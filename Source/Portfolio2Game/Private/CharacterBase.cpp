// CharacterBase.cpp

#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"
#include "BattleManager.h"      
#include "GridDataInterface.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PlayerCharacter.h" 
#include "Kismet/GameplayStatics.h" 
#include "GridISM.h"
#include "GA_Move.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("Attributes"));
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	BattleManagerRef = Cast<ABattleManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));

	if (!BattleManagerRef)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: 맵에서 BattleManager를 찾을 수 없습니다!"), *GetName());
	}

	if (AbilitySystem)
	{
		AbilitySystem->InitAbilityActorInfo(this, this);

		if (Attributes)
		{
			// "Health 속성값이 변하면 OnHealthAttributeChanged 함수를 실행해라" 라고 등록
			AbilitySystem->GetGameplayAttributeValueChangeDelegate(
				Attributes->GetHPAttribute()).AddUObject(this, &ACharacterBase::OnHealthAttributeChanged);
		}
	}

	if (HasAuthority())
	{
		InitAttributes();
		GiveAllSkills();
		GiveMoveAbilities();
	}

	if (BattleManagerRef && BattleManagerRef->GridActorRef)
	{
		AGridISM* Grid = Cast<AGridISM>(BattleManagerRef->GridActorRef);
		if (Grid && Attributes)
		{
			int32 Cur = FMath::RoundToInt(Attributes->GetHealth_BP());
			int32 Max = FMath::RoundToInt(Attributes->GetMaxHealth_BP());

			// 현재 칸(GridIndex)의 HP바 켜기
			Grid->UpdateTileHPBar(GridIndex, true, Cur, Max);

			// 위치 기억
			CachedGridIndex = GridIndex;

			// 델리게이트 연결 (HP 변할 때 갱신용)
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, FName("OnHealthChanged_Wrapper")); // 아래 4번에서 만들 함수
			OnHealthChanged.Add(Delegate);
		}
	}
}


// ───────── GAS ─────────

void ACharacterBase::InitAttributes()
{
	if (AbilitySystem && EffectList.Num() > 0)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystem->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		for (TSubclassOf<UGameplayEffect> EffectClass : EffectList)
		{
			FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
			if (SpecHandle.IsValid())
			{
				AbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void ACharacterBase::GiveAllSkills()
{
	if (AbilitySystem && HasAuthority())
	{
		for (TSubclassOf<UGameplayAbility> SkillClass : SkillList)
		{
			if (SkillClass)
			{
				FGameplayAbilitySpec AbilitySpec(SkillClass, 1, 0, this);
				AbilitySystem->GiveAbility(AbilitySpec);
			}
		}
	}
}

/**
 * (오류 수정) 4개의 다른 BP 어빌리티를 4개의 다른 InputID에 바인딩합니다.
 * 태그를 사용하지 않습니다.
 */
void ACharacterBase::GiveMoveAbilities()
{
	if (AbilitySystem && HasAuthority())
	{
		if (MoveAbility_Up)
		{
			FGameplayAbilitySpec MoveUpSpec(MoveAbility_Up, 1, PlayerAbilityInputID::MoveUp, this);
			AbilitySystem->GiveAbility(MoveUpSpec);
		}
		if (MoveAbility_Down)
		{
			FGameplayAbilitySpec MoveDownSpec(MoveAbility_Down, 1, PlayerAbilityInputID::MoveDown, this);
			AbilitySystem->GiveAbility(MoveDownSpec);
		}
		if (MoveAbility_Left)
		{
			FGameplayAbilitySpec MoveLeftSpec(MoveAbility_Left, 1, PlayerAbilityInputID::MoveLeft, this);
			AbilitySystem->GiveAbility(MoveLeftSpec);
		}
		if (MoveAbility_Right)
		{
			FGameplayAbilitySpec MoveRightSpec(MoveAbility_Right, 1, PlayerAbilityInputID::MoveRight, this);
			AbilitySystem->GiveAbility(MoveRightSpec);
		}
	}
}


// ───────── 턴 관리 ─────────

void ACharacterBase::StartAction()
{
	bCanAct = true;
	//ExecuteSkillQueue();
}

void ACharacterBase::EndAction()
{
	bCanAct = false;
	if (BattleManagerRef)
	{
		BattleManagerRef->EndCharacterTurn(this);
	}
}

// ───────── 스킬 큐 ─────────

void ACharacterBase::EnqueueSkill(TSubclassOf<UGameplayAbility> SkillClass)
{
	if (!SkillClass) return;
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
 * (수정됨) 캐릭터의 논리적 위치(좌표와 인덱스)를 업데이트합니다.
 */
void ACharacterBase::MoveToCell(FIntPoint TargetCoord, int32 TargetIndex)
{
    // 1. 이전 칸 HP바 끄기
    if (BattleManagerRef && BattleManagerRef->GridActorRef)
    {
        AGridISM* Grid = Cast<AGridISM>(BattleManagerRef->GridActorRef);
        if (Grid)
        {
            Grid->UpdateTileHPBar(CachedGridIndex, false); // 끄기
        }
    }

    // 2. 좌표 갱신
    GridCoord = TargetCoord;
    GridIndex = TargetIndex;
    CachedGridIndex = TargetIndex;

    // 3. 새 칸 HP바 켜기 (값 갱신)
    if (BattleManagerRef && BattleManagerRef->GridActorRef)
    {
        AGridISM* Grid = Cast<AGridISM>(BattleManagerRef->GridActorRef);
        if (Grid && Attributes)
        {
            int32 Cur = FMath::RoundToInt(Attributes->GetHealth_BP());
            int32 Max = FMath::RoundToInt(Attributes->GetMaxHealth_BP());
            Grid->UpdateTileHPBar(GridIndex, true, Cur, Max);
        }
    }
}

void ACharacterBase::OnHealthChanged_Wrapper(int32 CurrentHP, int32 MaxHP)
{
	if (BattleManagerRef && BattleManagerRef->GridActorRef)
	{
		AGridISM* Grid = Cast<AGridISM>(BattleManagerRef->GridActorRef);
		if (Grid)
		{
			// 죽으면 끄기, 살았으면 갱신
			bool bShow = (CurrentHP > 0);
			Grid->UpdateTileHPBar(GridIndex, bShow, CurrentHP, MaxHP);
		}
	}
}

void ACharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 변경된 값 가져오기
	float NewHealth = Data.NewValue;
	float MaxHealth = Attributes->GetMaxHP(); // MaxHealth는 변동 없다고 가정, 필요시 이것도 감지해야 함

	// 기존에 만들어둔 델리게이트 방송 -> GridISM의 UpdateTileHPBar가 호출됨
	OnHealthChanged.Broadcast((int32)NewHealth, (int32)MaxHealth);
}

void ACharacterBase::Turn(bool bRight)
{
	bFacingRight = bRight;
}

/**
 * (신규) GridIndex 변수를 반환합니다.
 */
int32 ACharacterBase::GetGridIndex() const
{
	return GridIndex;
}

// ───────── 스탯/데미지 (오류 수정) ─────────

/**
 * (신규) EnemyCharacter가 호출할 수 있도록 ApplyDamage를 다시 구현합니다.
 */
void ACharacterBase::ApplyDamage(float Damage)
{
	if (Attributes)
	{
		// BaseAttributeSet.h에 정의된 인라인 함수 사용
		Attributes->ApplyDamage(Damage);

		int32 CurrentHP = FMath::RoundToInt(Attributes->GetHealth_BP());
		int32 MaxHP = FMath::RoundToInt(Attributes->GetMaxHealth_BP());

		// UpdateTileHPBar가 즉시 호출
		OnHealthChanged.Broadcast(CurrentHP, MaxHP);
	}
}

// ❌ (오류 수정) FOnAttributeChangeData 델리게이트 함수 구현부 제거