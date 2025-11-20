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
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

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

	// 게임 시작 시 기본 방향(Right)으로 정렬
	RotateToDirection(EGridDirection::Right, false);
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
		// 1. 기존 스킬 리스트 등록 (유지)
		for (TSubclassOf<UGameplayAbility> SkillClass : SkillList)
		{
			if (SkillClass)
			{
				FGameplayAbilitySpec AbilitySpec(SkillClass, 1, 0, this);
				AbilitySystem->GiveAbility(AbilitySpec);
			}
		}

		// 2. [★추가★] 공용 공격 어빌리티 등록! (이게 빠져서 에러가 난 것)
		if (GenericAttackAbilityClass)
		{
			// InputID는 0(None)으로 해도 됨 (태그나 이벤트로 발동하니까)
			FGameplayAbilitySpec AttackSpec(GenericAttackAbilityClass, 1, 0, this);
			AbilitySystem->GiveAbility(AttackSpec);
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


// 회전

FRotator ACharacterBase::GetRotationFromEnum(EGridDirection Dir) const
{
	// 언리얼 월드 좌표계 기준 (X가 전방일 때)
	// 상황에 따라(카메라 각도 등) 각도는 수정 필요할 수 있음
	switch (Dir)
	{
	case EGridDirection::Right: return FRotator(0, 0, 0);      // X+
	case EGridDirection::Left:  return FRotator(0, 180, 0);    // X-
	case EGridDirection::Up:    return FRotator(0, 270, 0);    // Y- (화면 위쪽)
	case EGridDirection::Down:  return FRotator(0, 90, 0);     // Y+ (화면 아래쪽)
	}
	return FRotator::ZeroRotator;
}

void ACharacterBase::RequestRotation(EGridDirection NewDir, UAnimMontage* MontageToPlay)
{
	// 1. 일단 턴 행동 시작으로 간주 (입력 잠금 등)
	// (PlayerCharacter 등에서 이미 잠금 처리했겠지만 확실하게)
	bCanAct = false;
	PendingRotationDirection = NewDir;
	FacingDirection = NewDir; // 논리적 방향은 미리 업데이트 (UI 등 반영)

	OnBusyStateChanged.Broadcast(true);

	// 2. 애니메이션이 유효한지 체크
	if (MontageToPlay && GetMesh()->GetAnimInstance())
	{
		// A. 애니메이션 재생
		float Duration = PlayAnimMontage(MontageToPlay);

		if (Duration > 0.f)
		{
			// B. 종료 델리게이트 연결 (애니메이션 끝나면 FinalizeRotation 실행)
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ACharacterBase::FinalizeRotation);
			GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

			// 로그
			// UE_LOG(LogTemp, Log, TEXT("Rotation Anim Started: %s"), *MontageToPlay->GetName());
			return; // 여기서 리턴하면 FinalizeRotation이 나중에 호출됨
		}
	}

	// 3. 애니메이션이 없거나 재생 실패 시 -> 즉시 회전
	FinalizeRotation(nullptr, false);
}

void ACharacterBase::FinalizeRotation(UAnimMontage* Montage, bool bInterrupted)
{
	// 1. 메쉬를 실제 방향으로 회전시킴 (Snap)
	SetActorRotation(GetRotationFromEnum(PendingRotationDirection));

	// 2. 턴 종료 처리
	// (EndAction 내부에서 BattleManager에게 턴 넘김을 알림)
	EndAction();

	// UE_LOG(LogTemp, Log, TEXT("Rotation Finalized."));
}

void ACharacterBase::RotateToDirection(EGridDirection NewDir, bool bConsumeTurn)
{
	// 외부에서 강제로 돌릴 때(초기화 등)를 위해 남겨둠
	FacingDirection = NewDir;
	SetActorRotation(GetRotationFromEnum(FacingDirection));

	if (bConsumeTurn) EndAction();
}



// ───────── 턴 관리 ─────────

void ACharacterBase::StartAction()
{
	bCanAct = true;
	OnBusyStateChanged.Broadcast(false);
}

void ACharacterBase::EndAction()
{
	bCanAct = false;
	OnBusyStateChanged.Broadcast(true);
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

void ACharacterBase::MoveToCell(FIntPoint TargetCoord, int32 TargetIndex)
{
	

	// 2. 논리적 좌표 갱신 (이건 즉시 바뀜)
	GridCoord = TargetCoord;
	GridIndex = TargetIndex;


	// BattleManager를 통해 목표 좌표의 월드 위치를 가져옴
	if (BattleManagerRef)
	{
		FVector WorldDest = BattleManagerRef->GetWorldLocation(TargetCoord);
		WorldDest.Z += SpawnZOffset; // 높이 보정

		StartVisualMove(WorldDest);
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

// 매 프레임 호출
void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsVisualMoving)
	{
		// 시간 누적
		VisualMoveTimeElapsed += DeltaTime;

		// 진행률 계산 (0.0 ~ 1.0)
		// 0.1초가 지나면 무조건 1.0이 됨
		float Alpha = FMath::Clamp(VisualMoveTimeElapsed / VisualMoveDuration, 0.0f, 1.0f);

		// 위치 보간 (Lerp: Start에서 Dest까지 Alpha만큼 이동)
		FVector NewLoc = FMath::Lerp(VisualMoveStartLocation, VisualMoveDestination, Alpha);
		SetActorLocation(NewLoc);

		// 0.1초가 지났다면 (Alpha가 1.0이면) 도착 처리
		if (Alpha >= 1.0f)
		{
			// 1. 위치를 목표점에 강제로 박아넣음
			SetActorLocation(VisualMoveDestination);
			bIsVisualMoving = false;

			// 2. 애니메이션 즉시 차단
			if (RunMontage && GetMesh()->GetAnimInstance())
			{
				GetMesh()->GetAnimInstance()->Montage_Stop(0.0f, RunMontage);
			}

			// 3. HP바 위치 갱신 및 켜기
			if (BattleManagerRef && BattleManagerRef->GridActorRef)
			{
				AGridISM* Grid = Cast<AGridISM>(BattleManagerRef->GridActorRef);
				if (Grid && Attributes)
				{
					// 출발지 끄기
					Grid->UpdateTileHPBar(CachedGridIndex, false);

					// 도착지 켜기
					int32 Cur = FMath::RoundToInt(Attributes->GetHealth_BP());
					int32 Max = FMath::RoundToInt(Attributes->GetMaxHealth_BP());
					Grid->UpdateTileHPBar(GridIndex, true, Cur, Max);

					// 캐시 갱신
					CachedGridIndex = GridIndex;
				}
			}

			// 4. 턴 종료
			EndAction();
		}
	}
}

// 이동 시작 (내부 함수)
void ACharacterBase::StartVisualMove(const FVector& TargetLocation)
{
	// 시작점과 목표점 기록
	VisualMoveStartLocation = GetActorLocation();
	VisualMoveDestination = TargetLocation;

	// 시간 초기화
	VisualMoveTimeElapsed = 0.0f;
	bIsVisualMoving = true;

	OnBusyStateChanged.Broadcast(true);

	// 애니메이션 재생
	if (RunMontage)
	{
		PlayAnimMontage(RunMontage);
	}
}