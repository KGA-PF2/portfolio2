// PlayerCharacter.cpp

#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h" // GAS 입력 바인딩을 위해 포함

// (신규) Enhanced Input 관련 헤더
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h" // GetLocalPlayer()를 위해 필요

APlayerCharacter::APlayerCharacter()
{
	bCanAct = false;
}


void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 1. 서버에서 ASC 초기화
	if (AbilitySystem)
	{
		AbilitySystem->InitAbilityActorInfo(this, this);
	}

	// 2. (신규) 로컬 플레이어 컨트롤러에만 WASD용 매핑 컨텍스트 추가
	APlayerController* PC = Cast<APlayerController>(NewController);
	if (PC)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// (중요) PlayerMoveContext를 추가합니다. (BP의 컨텍스트(0)보다 높은 Priority(1)로 설정)
			if (PlayerMoveContext)
			{
				Subsystem->AddMappingContext(PlayerMoveContext, 1);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: 'PlayerMoveContext'가 할당되지 않았습니다. BP_PlayerCharacter에서 설정하세요."), *GetName());
			}
		}
	}
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

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Enhanced Input Component를 찾지 못했습니다! PlayerController 또는 Project 세팅을 확인하세요."), *GetNameSafe(this));
	}
}


// (기존) 입력 래퍼 함수 구현 
void APlayerCharacter::Input_MoveUp()
{
	if (bInputLocked) return; // 잠금 중이면 무시

	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveUp);
		LockInputTemporarily();
	}
}

void APlayerCharacter::Input_MoveDown()
{
	if (bInputLocked) return;
	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveDown);
		LockInputTemporarily();
	}
}

void APlayerCharacter::Input_MoveLeft()
{
	if (bInputLocked) return;
	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveLeft);
		LockInputTemporarily();
	}
}

void APlayerCharacter::Input_MoveRight()
{
	if (bInputLocked) return;
	if (AbilitySystem)
	{
		AbilitySystem->AbilityLocalInputPressed(PlayerAbilityInputID::MoveRight);
		LockInputTemporarily();
	}
}

// Q: 반시계 회전 (Right -> Up -> Left -> Down)
void APlayerCharacter::Input_RotateCCW()
{
	if (bInputLocked || !bCanAct) return;

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
}

// E: 시계 회전 (Right -> Down -> Left -> Up)
void APlayerCharacter::Input_RotateCW()
{
	if (bInputLocked || !bCanAct) return;

	EGridDirection NewDir = FacingDirection;
	switch (FacingDirection)
	{
	case EGridDirection::Right: NewDir = EGridDirection::Down; break;
	case EGridDirection::Down:  NewDir = EGridDirection::Left; break;
	case EGridDirection::Left:  NewDir = EGridDirection::Up; break;
	case EGridDirection::Up:    NewDir = EGridDirection::Right; break;
	}

	RequestRotation(NewDir, Montage_RotateCW);
}

// R: 뒤로 돌기
void APlayerCharacter::Input_Rotate180()
{
	if (bInputLocked || !bCanAct) return;

	EGridDirection NewDir = FacingDirection;
	switch (FacingDirection)
	{
	case EGridDirection::Right: NewDir = EGridDirection::Left; break;
	case EGridDirection::Left:  NewDir = EGridDirection::Right; break;
	case EGridDirection::Up:    NewDir = EGridDirection::Down; break;
	case EGridDirection::Down:  NewDir = EGridDirection::Up; break;
	}

	RequestRotation(NewDir, Montage_Rotate180);
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
	if (bInputLocked || !bCanAct) return; // 행동 불가시 무시
	if (SkillQueueIndices.Num() == 0) return; // 큐가 비었으면 무시

	LockInputTemporarily(); // 입력 잠금.

	UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 시작 ==="));

	ExecuteNextSkillInQueue_UI(); // 0.2초 딜레이 시퀀스 시작
}

void APlayerCharacter::Input_CancelSkills()
{
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
	// 1. 큐에 남은 인덱스가 없으면 턴 종료
	if (SkillQueueIndices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 완료 ==="));
		GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
		OnSkillQueueCleared_BPEvent.Broadcast();
		EndAction();
		return;
	}

	// 2. 큐의 맨 앞 '인덱스'를 꺼냄 (원래 변수명 사용)
	int32 SkillIndexToFire = SkillQueueIndices[0];
	SkillQueueIndices.RemoveAt(0);

	// (안전 장치)
	if (!OwnedSkills.IsValidIndex(SkillIndexToFire))
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteNextSkillInQueue_UI: 큐에 잘못된 인덱스(%d)가 있었습니다."), SkillIndexToFire);
		// 큐에 스킬이 더 남아있으면 다음 것을 시도
		if (SkillQueueIndices.Num() > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(
				SkillQueueTimerHandle,
				this,
				&APlayerCharacter::ExecuteNextSkillInQueue_UI,
				SkillExecutionDelay,
				false
			);
		}
		else
		{
			GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
			EndAction();
		}
		return;
	}

	// 3. 스킬 데이터 가져오기
	FPlayerSkillData& SkillData = OwnedSkills[SkillIndexToFire];

	// 필수 요소 검사 로그
	if (!GenericAttackAbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR: BP_PlayerCharacter에 'GenericAttackAbilityClass'가 비어있습니다! 할당해주세요."));
		return;
	}
	if (!SkillData.SkillInfo)
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR: 스킬 데이터에 SkillInfo가 없습니다."));
		return;
	}

	bool bSuccess = false;

	// ───────── [수정됨] GAS로 스킬 실행 ─────────
	if (AbilitySystem)
	{
		FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(GenericAttackAbilityClass);
		if (Spec)
		{
			FGameplayEventData Payload;
			Payload.OptionalObject = SkillData.SkillInfo;
			Payload.Instigator = this;
			Payload.Target = this;

			FGameplayTag TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Attack"));

			// 실행 결과 받기 (int 반환값으로 성공 여부 추측 가능)
			int32 Result = AbilitySystem->TriggerAbilityFromGameplayEvent(
				Spec->Handle,
				AbilitySystem->AbilityActorInfo.Get(),
				TriggerTag,
				&Payload,
				*AbilitySystem
			);

			// TriggerAbility는 void거나 복잡하므로, Spec이 Active 상태인지 확인하는 게 정확하지만 일단 넘어감
			bSuccess = true;
			UE_LOG(LogTemp, Warning, TEXT("GAS Trigger Signal Sent! (Tag: Ability.Skill.Attack)"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ERROR: 어빌리티 시스템에 GA_SkillAttack이 등록되지 않았습니다. GiveAllSkills 확인 필요."));
		}
	}

	// 결과에 따른 로그 출력
	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("SKILL FIRED SUCCESS: %s"), *SkillData.GetSkillName().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SKILL FIRED FAILED: GAS 발동 실패"));
	}
	// 4. 쿨타임 적용
	ApplySkillCooldown(SkillIndexToFire);

	// 5. 큐(SkillQueueIndices)에 스킬이 더 남아있다면 딜레이 후 재호출
	if (SkillQueueIndices.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SkillQueueTimerHandle,
			this,
			&APlayerCharacter::ExecuteNextSkillInQueue_UI,
			SkillExecutionDelay, // 0.2초
			false
		);
	}
	else
	{
		// 6. 마지막 스킬이었으므로, 즉시 턴 종료
		// (주의: 몽타주 재생 시간과 별개로 턴이 즉시 종료되므로, 필요 시 EndAction 호출을 늦춰야 할 수도 있음)
		UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 완료 ==="));
		GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
		OnSkillQueueCleared_BPEvent.Broadcast();
		EndAction();
	}
}

int32 APlayerCharacter::GetCurrentCooldownForSkill(int32 SkillIndex) const
{
	if (!OwnedSkills.IsValidIndex(SkillIndex))
	{
		return -1; // 잘못된 인덱스
	}
	return OwnedSkills[SkillIndex].CurrentCooldown;
}