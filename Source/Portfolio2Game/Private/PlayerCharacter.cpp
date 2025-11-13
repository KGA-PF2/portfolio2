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

	// 2. 큐의 맨 앞 '인덱스'를 꺼냄
	int32 SkillIndexToFire = SkillQueueIndices[0];
	SkillQueueIndices.RemoveAt(0);

	// (안전 장치)
	if (!OwnedSkills.IsValidIndex(SkillIndexToFire))
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteNextSkillInQueue_UI: 큐에 잘못된 인덱스(%d)가 있었습니다."), SkillIndexToFire);
		// 큐에 스킬이 더 남아있으면 다음 것을 시도
		if (SkillQueueIndices.Num() > 0)
		{
			// (재귀 호출이 아닌 타이머 재설정)
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
			// 마지막 스킬이었으면 턴 종료
			GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
			EndAction();
		}
		return;
	}

	// 3. 스킬 발동 및 쿨다운 적용
	FPlayerSkillData& SkillData = OwnedSkills[SkillIndexToFire];

	UE_LOG(LogTemp, Warning, TEXT("SKILL FIRED: %s (인덱스 %d)"), *SkillData.GetSkillName().ToString(), SkillIndexToFire);

	// 인덱스로 쿨다운 함수 호출
	ApplySkillCooldown(SkillIndexToFire);

	// 4. 큐(SkillQueueIndices)에 스킬이 더 남아있다면 0.2초 뒤에 이 함수를 다시 호출
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
		// 5. 이것이 마지막 스킬이었으므로, 즉시 턴 종료
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