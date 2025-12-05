// CharacterBase.cpp

#include "CharacterBase.h"
#include "GameplayAbilitySpec.h"
#include "BattleManager.h"      
#include "GridDataInterface.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PlayerCharacter.h" 
#include "GameFramework/CharacterMovementComponent.h"
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
		//UE_LOG(LogTemp, Error, TEXT("%s: 맵에서 BattleManager를 찾을 수 없습니다!"), *GetName());
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
	//RotateToDirection(EGridDirection::Right, false);
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
	bCanAct = false;
	PendingRotationDirection = NewDir;
	FacingDirection = NewDir;
	OnBusyStateChanged.Broadcast(true);

	// 목표 각도 계산해두기
	RotationStartQuat = GetActorQuat();
	RotationTargetQuat = GetRotationFromEnum(NewDir).Quaternion();

	// 방해꾼 끄기
	bUseControllerRotationYaw = false;
	if (GetCharacterMovement()) GetCharacterMovement()->bOrientRotationToMovement = false;
	if (GetMesh()->GetAnimInstance()) GetMesh()->GetAnimInstance()->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);

	// 애니메이션 재생
	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay, 2.0f);

		// 종료 델리게이트 (안전장치)
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ACharacterBase::FinalizeRotation);
		GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, MontageToPlay);
	}
	else
	{
		FinalizeRotation(nullptr, false);
	}
}

void ACharacterBase::OnAnimNotify_TurnStart()
{
	if (bIsRotating)
	{
		bCanRotate = true;
		CurrentRotationTime = 0.0f;
		// ★ 회전 구간 길이 설정 (예: 0.3초 동안 휙 돈다)
		// 몽타주에서 노티파이 간격을 잴 수 없으므로 하드코딩하거나 파라미터로 받아야 함.
		// 여기서는 0.3초(빠름)로 설정합니다.
		ActualRotationDuration = 0.3f;
	}
}

void ACharacterBase::OnAnimNotify_TurnEnd()
{
	// 회전 강제 완료
	if (bIsRotating)
	{
		bCanRotate = false;
		// 혹시 덜 돌았으면 최종 각도로 고정
		SetActorRotation(RotationTargetQuat);
	}
}

void ACharacterBase::FinalizeRotation(UAnimMontage* Montage, bool bInterrupted)
{
	bIsRotationWindowActive = false;
	SetActorRotation(GetRotationFromEnum(PendingRotationDirection));
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}
	EndAction();
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

void ACharacterBase::SetHighlight(bool bEnable)
{
	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(bEnable);
		// (선택) 색상을 구분하고 싶다면 Stencil Value를 변경
		// GetMesh()->SetCustomDepthStencilValue(bEnable ? 1 : 0); 
	}
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
		// 1. 시간 누적 및 진행률(Alpha) 계산
		VisualMoveTimeElapsed += DeltaTime;
		float Alpha = FMath::Clamp(VisualMoveTimeElapsed / VisualMoveDuration, 0.0f, 1.0f);

		// 2. 위치 보간 (Lerp)
		FVector NewLoc = FMath::Lerp(VisualMoveStartLocation, VisualMoveDestination, Alpha);
		SetActorLocation(NewLoc);

		// 3. [도착 판정]
		if (Alpha >= 1.0f)
		{
			// A. 위치 확정 & 이동 플래그 끄기
			SetActorLocation(VisualMoveDestination);
			bIsVisualMoving = false;

			// B. 뛰는 모션(RunMontage) 강제 종료 (잔상/제자리 달리기 방지)
			if (RunMontage && GetMesh()->GetAnimInstance())
			{
				// 0.1초 동안 부드럽게 멈춤 (즉시 뚝 끊기지 않게)
				GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, RunMontage);
			}

			// C. 대기 시간 계산 (기본 0.25초: 숨 고를 시간 확보)
			float WaitTime = 0.25f;

			// D. 정지 몽타주(CurrentStopMontage) 재생
			if (CurrentStopMontage && GetMesh()->GetAnimInstance())
			{
				float Duration = PlayAnimMontage(CurrentStopMontage);

				// 몽타주가 정상 재생됐다면, 그 길이만큼 기다림
				if (Duration > 0.0f)
				{
					WaitTime = Duration;
				}
			}

			// E. 계산된 시간만큼 기다렸다가 턴 종료
			// (정지 모션이 없어도 최소 0.25초는 대기하므로 적이 급발진하지 않음)
			GetWorld()->GetTimerManager().SetTimer(
				StopAnimTimerHandle,
				this,
				&ACharacterBase::OnStopAnimEnded,
				WaitTime,
				false
			);

			// F. HP바 위치 갱신 (기존 로직 유지)
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
		}
	}

}

void ACharacterBase::OnStopAnimEnded()
{
	// 변수 초기화 (다음 턴을 위해)
	CurrentStopMontage = nullptr;
	EndAction();
}

void ACharacterBase::BeginRotationWindow(float Duration)
{
	bIsRotationWindowActive = true;

	// PlayRate가 2.0이면 실제 시간은 절반임.
	// NotifyState의 Duration은 '에셋 기준 시간'이므로 PlayRate로 나눠줘야 실제 시간이 됨.
	float CurrentPlayRate = 1.0f;
	if (GetMesh()->GetAnimInstance() && GetMesh()->GetAnimInstance()->GetCurrentActiveMontage())
	{
		CurrentPlayRate = GetMesh()->GetAnimInstance()->GetCurveValue(TEXT("PlayRate")); // 만약 커브로 제어 안하면 아래 사용
		// 혹은 간단히:
		CurrentPlayRate = 2.0f; // 아까 2.0으로 틀었으니까 (하드코딩 싫으면 변수로 저장해두세요)
	}

	RotationWindowDuration = Duration / CurrentPlayRate;
	RotationWindowElapsed = 0.0f;
}

// 3. [신규] 노티파이가 호출: 매 프레임 회전
void ACharacterBase::TickRotationWindow(float DeltaTime)
{
	if (!bIsRotationWindowActive) return;

	RotationWindowElapsed += DeltaTime;

	// 진행률 (0 ~ 1)
	float Alpha = 0.0f;
	if (RotationWindowDuration > KINDA_SMALL_NUMBER)
	{
		Alpha = FMath::Clamp(RotationWindowElapsed / RotationWindowDuration, 0.0f, 1.0f);
	}

	// Slerp로 부드럽게 돌리기
	FQuat NewQuat = FQuat::Slerp(RotationStartQuat, RotationTargetQuat, Alpha);
	SetActorRotation(NewQuat);
}

// 4. [신규] 노티파이가 호출: 회전 끝
void ACharacterBase::EndRotationWindow()
{
	bIsRotationWindowActive = false;
	// 목표 각도로 확실하게 고정
	SetActorRotation(RotationTargetQuat);
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