// PlayerCharacter.cpp

#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "PortfolioGameInstance.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h" // GAS 입력 바인딩을 위해 포함
#include "EnhancedInputSubsystems.h"
#include "PlayerSkillDataLibrary.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h" // GetLocalPlayer()를 위해 필요

APlayerCharacter::APlayerCharacter()
{
	bCanAct = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName("WeaponSocket"));
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
}

void APlayerCharacter::StartAction()
{
	Super::StartAction();

	bHasCommittedAction = false;

	SetInputEnabled(true);
}

void APlayerCharacter::EndAction()
{

	SetInputEnabled(false);

	bHasCommittedAction = true; // 혹시 모르니 잠금

	Super::EndAction();
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 1. ASC 초기화 (기존 유지)
	if (AbilitySystem)
	{
		AbilitySystem->InitAbilityActorInfo(this, this);
	}

	// 2. 데이터 로드 (기존 유지)
	if (HasAuthority())
	{
		UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());
		if (GI && GI->HasSavedData())
		{
			if (Attributes)
			{
				Attributes->InitHealth(GI->SavedMaxHP);
				Attributes->SetHealth_Internal(GI->SavedCurrentHP);\

				int32 CurrentHP = FMath::RoundToInt(GI->SavedCurrentHP);
				int32 MaxHP = FMath::RoundToInt(GI->SavedMaxHP);
				OnHealthChanged.Broadcast(CurrentHP, MaxHP);
			}
			OwnedSkills = GI->SavedSkills;

			for (FPlayerSkillData& Skill : OwnedSkills)
			{
				Skill.CurrentCooldown = 0;
			}
		}
		else
		{
			InitAttributes();
			GiveAllSkills();
		}
	}

	// 3. 입력 설정 & ★ 마우스 무조건 켜기 (통합) ★
	APlayerController* PC = Cast<APlayerController>(NewController);
	if (PC)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (PlayerMoveContext)
			{
				Subsystem->AddMappingContext(PlayerMoveContext, 1);
			}
		}
	}

	PC->bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false); // 클릭 시 사라짐 방지

	PC->SetInputMode(InputMode);

	// 상태 초기화
	bInputLocked = false;
	bHasCommittedAction = false;
}


void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_MoveUp)
		{
			EnhancedInputComponent->BindAction(IA_MoveUp, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveUp);
		}
		if (IA_MoveDown)
		{
			EnhancedInputComponent->BindAction(IA_MoveDown, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveDown);
		}
		if (IA_MoveLeft)
		{
			EnhancedInputComponent->BindAction(IA_MoveLeft, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveLeft);
		}
		if (IA_MoveRight)
		{
			EnhancedInputComponent->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_MoveRight);
		}
		if (IA_ExecuteSkills)
		{
			EnhancedInputComponent->BindAction(IA_ExecuteSkills, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_ExecuteSkills);
		}
		if (IA_CancelSkills)
		{
			EnhancedInputComponent->BindAction(IA_CancelSkills, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_CancelSkills);
		}

		// [신규] 회전 바인딩
		if (IA_RotateCCW)
			EnhancedInputComponent->BindAction(IA_RotateCCW, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_RotateCCW);
		if (IA_RotateCW) 
			EnhancedInputComponent->BindAction(IA_RotateCW, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_RotateCW);
		if (IA_Rotate180)
			EnhancedInputComponent->BindAction(IA_Rotate180, ETriggerEvent::Triggered, this, &APlayerCharacter::Input_Rotate180);
		if (IA_DebugClear)
		{
			EnhancedInputComponent->BindAction(IA_DebugClear, ETriggerEvent::Started, this, &APlayerCharacter::Input_DebugStageClear);
		}

		if (IA_Pause)
		{
			EnhancedInputComponent->BindAction(IA_Pause, ETriggerEvent::Started, this, &APlayerCharacter::Input_Pause);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Enhanced Input Component를 찾지 못했습니다! PlayerController 또는 Project 세팅을 확인하세요."), *GetNameSafe(this));
	}
}

void APlayerCharacter::SetInputEnabled(bool bEnabled)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (bEnabled)
			{
				// 중복 방지를 위해 일단 지웠다 추가 (안전빵)
				Subsystem->RemoveMappingContext(PlayerMoveContext);
				Subsystem->AddMappingContext(PlayerMoveContext, 1);
			}
			else
			{
				Subsystem->RemoveMappingContext(PlayerMoveContext);
			}
		}
	}
}

// (기존) 입력 래퍼 함수 구현 
void APlayerCharacter::Input_MoveUp()
{
	if (bIsSkillQueueRunning) return;
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;
	if (AbilitySystem)
	{
		bHasCommittedAction = true;
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveUp);
		LockInputTemporarily();
	}
}

void APlayerCharacter::Input_MoveDown()
{
	if (bIsSkillQueueRunning) return;
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;	if (AbilitySystem)
	{
		bHasCommittedAction = true;
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveDown);
		LockInputTemporarily();
	}
}

void APlayerCharacter::Input_MoveLeft()
{
	if (bIsSkillQueueRunning) return;
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;	if (AbilitySystem)
	{
		bHasCommittedAction = true;
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveLeft);
		LockInputTemporarily();
	}
}

void APlayerCharacter::Input_MoveRight()
{
	if (bIsSkillQueueRunning) return;
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;	if (AbilitySystem)
	{
		bHasCommittedAction = true;
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveRight);
		LockInputTemporarily();
	}
}

// Q: 반시계 회전 (Right -> Up -> Left -> Down)
void APlayerCharacter::Input_RotateCCW()
{
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;
	EGridDirection NewDir = FacingDirection;
	switch (FacingDirection)
	{
	case EGridDirection::Right: NewDir = EGridDirection::Up; break;
	case EGridDirection::Up:    NewDir = EGridDirection::Left; break;
	case EGridDirection::Left:  NewDir = EGridDirection::Down; break;
	case EGridDirection::Down:  NewDir = EGridDirection::Right; break;
	}

	// 몽타주와 함께 요청 (없으면 즉시 회전)
	RequestRotation(NewDir, Montage_RotateCCW);
	bHasCommittedAction = true;
}

// E: 시계 회전 (Right -> Down -> Left -> Up)
void APlayerCharacter::Input_RotateCW()
{
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;
	EGridDirection NewDir = FacingDirection;
	switch (FacingDirection)
	{
	case EGridDirection::Right: NewDir = EGridDirection::Down; break;
	case EGridDirection::Down:  NewDir = EGridDirection::Left; break;
	case EGridDirection::Left:  NewDir = EGridDirection::Up; break;
	case EGridDirection::Up:    NewDir = EGridDirection::Right; break;
	}

	RequestRotation(NewDir, Montage_RotateCW);
	bHasCommittedAction = true;
}

// R: 뒤로 돌기
void APlayerCharacter::Input_Rotate180()
{
	if (!bCanAct || bInputLocked || bHasCommittedAction) return;
	EGridDirection NewDir = FacingDirection;
	switch (FacingDirection)
	{
	case EGridDirection::Right: NewDir = EGridDirection::Left; break;
	case EGridDirection::Left:  NewDir = EGridDirection::Right; break;
	case EGridDirection::Up:    NewDir = EGridDirection::Down; break;
	case EGridDirection::Down:  NewDir = EGridDirection::Up; break;
	}

	RequestRotation(NewDir, Montage_Rotate180);
	bHasCommittedAction = true;
}

void APlayerCharacter::Input_DebugStageClear()
{
	if (BattleManagerRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Debug] Force Stage Clear!"));
		BattleManagerRef->ForceStageClear();
	}
}

void APlayerCharacter::LockInputTemporarily()
{
	bInputLocked = true;
	GetWorld()->GetTimerManager().SetTimer(InputLockTimerHandle, this,
		&APlayerCharacter::UnlockInput, InputCooldown, false);
}

void APlayerCharacter::UnlockInput()
{
	bInputLocked = false;
}

void APlayerCharacter::SelectSkill(int32 SkillIndex)
{
	if (!bCanAct || bInputLocked) return;
	if (BattleManagerRef && BattleManagerRef->CurrentState != EBattleState::PlayerTurn)
	{
		return;
	}
	if (SkillQueueIndices.Num() >= 3) return; // 최대 3개

	// (★수정★) 인덱스 유효성 검사
	if (!OwnedSkills.IsValidIndex(SkillIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("SelectSkill: 잘못된 인덱스(%d)입니다."), SkillIndex);
		return;
	}

	// (★수정★) 쿨타임 검사 (인덱스 기반)
	if (OwnedSkills[SkillIndex].CurrentCooldown > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectSkill: %s (인덱스 %d)는 쿨타임 중입니다."), *OwnedSkills[SkillIndex].GetSkillName().ToString(), SkillIndex);
		return;
	}

	// (★수정★) 중복 검사 (인덱스 기반)
	if (SkillQueueIndices.Contains(SkillIndex))
		return;

	// (★수정★) UI 큐에 '인덱스' 추가
	SkillQueueIndices.Add(SkillIndex);

	// (유지) UI 큐 시각화용 이벤트는 SkillInfo 애셋을 보냄 (아이콘 표시용)
	OnSkillSelected_BPEvent.Broadcast(OwnedSkills[SkillIndex].SkillInfo);

	UE_LOG(LogTemp, Warning, TEXT("%s (인덱스 %d) 스킬 선택됨 (현재 %d개)"), *OwnedSkills[SkillIndex].GetSkillName().ToString(), SkillIndex, SkillQueueIndices.Num());

	LockInputTemporarily();
	EndAction();
}

// 턴 종료 후 대기열 초기화
void APlayerCharacter::ClearSkillQueue()
{
	SkillQueueIndices.Empty();
}

// 모든 스킬 쿨타임 감소
void APlayerCharacter::ReduceCooldowns()
{
	for (auto& SkillData : OwnedSkills)
	{
		if (SkillData.CurrentCooldown > 0)
			SkillData.CurrentCooldown--;


	}
}

// 스킬 사용 시 쿨타임 적용
void APlayerCharacter::ApplySkillCooldown(int32 SkillIndex)
{
	// (★수정★)
	if (!OwnedSkills.IsValidIndex(SkillIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("ApplySkillCooldown: 잘못된 인덱스(%d)입니다."), SkillIndex);
		return;
	}

	FPlayerSkillData& SkillData = OwnedSkills[SkillIndex];
	SkillData.CurrentCooldown = SkillData.GetEffectiveTotalCooldown();

	UE_LOG(LogTemp, Warning, TEXT("ApplyCooldown: %s (인덱스 %d)에 쿨타임 %d 적용됨"), *SkillData.GetSkillName().ToString(), SkillIndex, SkillData.CurrentCooldown);
}

void APlayerCharacter::Input_ExecuteSkills()
{
	if (bIsSkillQueueRunning) return;
	if (bInputLocked || !bCanAct) return; // 행동 불가시 무시
	if (SkillQueueIndices.Num() == 0) return; // 큐가 비었으면 무시

	if (GetWorld()->GetTimerManager().IsTimerActive(SkillQueueTimerHandle))
	{
		return;
	}

	bIsSkillQueueRunning = true;

	bHasCommittedAction = true;
	LockInputTemporarily(); // 입력 잠금.

	UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 시작 ==="));

	ExecuteNextSkillInQueue_UI(); // 0.2초 딜레이 시퀀스 시작
}

void APlayerCharacter::Input_CancelSkills()
{
	if (bIsSkillQueueRunning) return;
	if (bInputLocked || !bCanAct) return;
	if (SkillQueueIndices.Num() == 0) return;

	GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
	ClearSkillQueue();

	UE_LOG(LogTemp, Warning, TEXT("모든 스킬 큐가 취소되었습니다."));

	OnSkillQueueCleared_BPEvent.Broadcast();

	LockInputTemporarily();
}

void APlayerCharacter::ExecuteNextSkillInQueue_UI()
{
	// 1. 큐가 비었으면 진짜로 종료 (타이머 타고 들어온 마지막 호출)
	if (SkillQueueIndices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 완료 ==="));
		GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
		OnSkillQueueCleared_BPEvent.Broadcast(); // UI 큐 비우기 신호

		bIsSkillQueueRunning = false;
		EndAction(); // 턴 종료
		return;
	}

	// 2. 인덱스 꺼내기
	int32 SkillIndexToFire = SkillQueueIndices[0];
	SkillQueueIndices.RemoveAt(0);

	// (안전 장치)
	if (!OwnedSkills.IsValidIndex(SkillIndexToFire))
	{
		// 에러나면 다음거 바로 실행
		ExecuteNextSkillInQueue_UI();
		return;
	}

	FPlayerSkillData& SkillData = OwnedSkills[SkillIndexToFire];

	// 필수 체크
	if (!GenericAttackAbilityClass || !SkillData.SkillInfo)
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR: 데이터 누락"));
		return;
	}

	// ★ [수정 1] 대기 시간 계산 (애니메이션 길이)
	float AnimDuration = 0.2f; // 기본값 (몽타주 없으면 이거 씀)

	if (SkillData.SkillInfo->SkillMontage)
	{
		// 애니메이션 길이 + 0.1초(안전 여유값)
		AnimDuration = SkillData.SkillInfo->SkillMontage->GetPlayLength() + 0.3f;
	}

	bool bSuccess = false;

	// 3. GAS 실행
	if (AbilitySystem)
	{
		FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(GenericAttackAbilityClass);
		if (Spec)
		{
			// ★ [수정 2] 강화된 데미지 계산해서 Payload에 담기
			float FinalDmg = (float)UPlayerSkillDataLibrary::GetEffectiveDamage(SkillData);

			FGameplayEventData Payload;
			Payload.OptionalObject = SkillData.SkillInfo;
			Payload.Instigator = this;
			Payload.Target = this;
			Payload.EventMagnitude = FinalDmg; // <-- 여기에 데미지 전달

			FGameplayTag TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Attack"));

			AbilitySystem->TriggerAbilityFromGameplayEvent(
				Spec->Handle,
				AbilitySystem->AbilityActorInfo.Get(),
				TriggerTag,
				&Payload,
				*AbilitySystem
			);

			bSuccess = true;
		}
	}

	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("SKILL FIRED: %s (Duration: %.2f)"), *SkillData.GetSkillName().ToString(), AnimDuration);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SKILL FIRED FAILED"));
	}

	// 4. 쿨타임 적용
	ApplySkillCooldown(SkillIndexToFire);

	// 5. ★ [수정 3] 다음 행동 예약 (재귀 호출)
	// 남은 스킬이 있든 없든, 이번 애니메이션이 끝날 때까지 기다려야 함.
	// 큐가 비어있으면 다음번 호출 때 맨 위 'if (Num == 0)'에 걸려서 종료됨.
	GetWorld()->GetTimerManager().SetTimer(
		SkillQueueTimerHandle,
		this,
		&APlayerCharacter::ExecuteNextSkillInQueue_UI,
		AnimDuration, // <--- 0.2초 대신 애니메이션 길이만큼 기다림
		false
	);
}

int32 APlayerCharacter::GetCurrentCooldownForSkill(int32 SkillIndex) const
{
	if (!OwnedSkills.IsValidIndex(SkillIndex))
	{
		return -1; // 잘못된 인덱스
	}
	return OwnedSkills[SkillIndex].CurrentCooldown;
}

void APlayerCharacter::Input_Pause()
{
	// 이미 일시정지 상태라면 -> 재개
	if (UGameplayStatics::IsGamePaused(this))
	{
		ResumeGame();
	}
	// 게임 중이라면 -> 일시정지
	else
	{
		if (!PauseMenuWidgetClass) return;

		// 1. 게임 멈춤
		UGameplayStatics::SetGamePaused(this, true);

		// 2. UI 띄우기
		if (!PauseMenuInstance)
		{
			PauseMenuInstance = CreateWidget<UUserWidget>(GetWorld(), PauseMenuWidgetClass);
		}

		if (PauseMenuInstance)
		{
			PauseMenuInstance->AddToViewport(9999); // 최상단
		}

		// 3. 입력 모드를 UI 전용으로 변경 (게임 조작 막기)
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;

			if (PauseMenuInstance)
			{
				InputMode.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
			}

			PC->SetInputMode(InputMode);
		}
	}
}

void APlayerCharacter::Test_InstantlyDie()
{
	if (bDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is already dead, ignoring Test_InstantlyDie."));
		return;
	}

	// 1. AbilitySystem이 유효한지 확인 (ACharacterBase에서 상속)
	if (AbilitySystem)
	{
		// 2. ★ 핵심: AbilitySystemComponent를 통해 HP 속성 값을 0으로 설정 ★
		// GetHPAttribute()는 UBaseAttributeSet에 정의된 static 함수입니다.
		AbilitySystem->SetNumericAttributeBase(UBaseAttributeSet::GetHPAttribute(), 0.0f);

		// SetNumericAttributeBase()는 변경 델리게이트를 발생시키므로
		// OnHealthChanged_Wrapper -> OnDeath() 로직이 실행됩니다.

		UE_LOG(LogTemp, Log, TEXT("Player HP set to 0.0f via ASC. Triggering Death Logic."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent is null! Cannot test death."));
	}
}

void APlayerCharacter::ResumeGame()
{
	// 1. 게임 재개
	UGameplayStatics::SetGamePaused(this, false);

	// 2. UI 끄기
	if (PauseMenuInstance)
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr; // 매번 새로 만들 거면 초기화, 재활용할 거면 유지
	}

	// 3. 입력 모드를 게임+UI로 복구 (우리가 쓰던 설정)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);

		PC->SetInputMode(InputMode);
	}
}

void APlayerCharacter::Die()
{
	if (bDead) return;
	bDead = true;

	// 1. 입력 막기
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// 2. 사망 몽타주 재생
	float Duration = 2.0f; // 기본 대기 시간
	if (DeathMontage)
	{
		Duration = PlayAnimMontage(DeathMontage);

		// 몽타주 길이가 너무 짧으면 최소 1초는 보장
		if (Duration < 1.0f) Duration = 1.0f;
	}

	UE_LOG(LogTemp, Warning, TEXT("Player Died! Waiting %.2f seconds..."), Duration);

	// 3. 타이머 설정 (애니메이션 길이만큼 대기)
	FTimerHandle DeathTimer;
	GetWorld()->GetTimerManager().SetTimer(
		DeathTimer,
		this,
		&APlayerCharacter::FinishDying,
		Duration,
		false
	);

	// (선택) BP의 OnDeath 이벤트도 호출하고 싶다면 여기서 호출
	OnDeath();
}

void APlayerCharacter::FinishDying()
{
	// 배틀 매니저에게 "나 끝났어, 게임 오버 띄워" 라고 요청
	if (BattleManagerRef)
	{
		BattleManagerRef->OnPlayerDeathFinished();
	}
}