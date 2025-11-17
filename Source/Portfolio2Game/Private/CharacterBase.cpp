// CharacterBase.cpp

#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"
#include "BattleManager.h"      
#include "PlayerCharacter.h"    // PlayerAbilityInputID를 사용하기 위해 포함
#include "Kismet/GameplayStatics.h" 
#include "GA_Move.h" // GA_Move 클래스를 직접 참조하기 위해 포함

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
	}

	if (HasAuthority())
	{
		InitAttributes();
		GiveAllSkills();
		GiveMoveAbilities();
	}


	// 1. HPBarActorClass가 설정되어 있는지 확인
	if (HPBarActorClass)
	{
		// 2. 스폰 위치 계산
		FVector SpawnLocation = GetActorLocation();

		// 3. HP바 액터 스폰
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;

		HPBarActor = GetWorld()->SpawnActor<AActor>(HPBarActorClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		// 4. 스폰 성공 및 AttributeSet 유효성 확인
		if (HPBarActor)
		{
			HPBarActor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

			if (Attributes)
			{
				FScriptDelegate Delegate;
				Delegate.BindUFunction(HPBarActor, FName("HandleHealthChanged"));

				if (!OnHealthChanged.Contains(Delegate))
				{
					OnHealthChanged.Add(Delegate);
				}

				int32 CurrentHP = FMath::RoundToInt(Attributes->GetHealth_BP());
				int32 MaxHP = FMath::RoundToInt(Attributes->GetMaxHealth_BP());
				OnHealthChanged.Broadcast(CurrentHP, MaxHP);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: HPBarActorClass 설정되지 않음"), *GetName());
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
	GridCoord = TargetCoord;
	GridIndex = TargetIndex;
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
	}
}

// ❌ (오류 수정) FOnAttributeChangeData 델리게이트 함수 구현부 제거