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

void APlayerCharacter::SelectSkill(USkillBase* Skill)
{
	// 1. (신규) 턴이 아니거나, 입력이 잠겼으면 무시
	if (!bCanAct || bInputLocked)
	{
		return;
	}

	// 2. (신규) 최대 3개까지만 선택 가능
	if (SkillQueue.Num() >= 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 큐가 가득 찼습니다 (최대 3개)."));
		return;
	}

	// 3. (유지) 유효성 검사 및 중복 방지
	if (!Skill || SkillQueue.Contains(Skill))
		return;

	// 4. (수정) UI 큐에만 추가
	SkillQueue.Add(Skill);

	// 5. ✅ [신규] UI 위젯에 "스킬 추가됨" 이벤트 발송
	OnSkillSelected_BPEvent.Broadcast(Skill);

	UE_LOG(LogTemp, Warning, TEXT("%s 스킬 선택됨 (현재 %d개)"), *Skill->SkillName.ToString(), SkillQueue.Num());

	// 6. (유지) 턴 종료
	LockInputTemporarily();
	EndAction();
}

// 턴 종료 후 대기열 초기화
void APlayerCharacter::ClearSkillQueue()
{
	SkillQueue.Empty();
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
void APlayerCharacter::ApplySkillCooldown(USkillBase* UsedSkill)
{
	for (auto& SkillData : OwnedSkills)
	{
		if (SkillData.SkillInfo == UsedSkill)
		{
			SkillData.CurrentCooldown = SkillData.GetEffectiveTotalCooldown();
			break;
		}
	}
}

void APlayerCharacter::Input_ExecuteSkills()
{
	if (bInputLocked || !bCanAct) return; // 행동 불가시 무시
	if (SkillQueue.Num() == 0) return; // 큐가 비었으면 무시

	LockInputTemporarily(); // 입력 잠금.

	UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 시작 ==="));
	OnSkillQueueCleared_BPEvent.Broadcast();
	ExecuteNextSkillInQueue_UI(); // 0.2초 딜레이 시퀀스 시작
}

void APlayerCharacter::Input_CancelSkills()
{
	if (bInputLocked || !bCanAct) return;
	if (SkillQueue.Num() == 0) return;

	GetWorld()->GetTimerManager().ClearTimer(SkillQueueTimerHandle);
	ClearSkillQueue();

	UE_LOG(LogTemp, Warning, TEXT("모든 스킬 큐가 취소되었습니다."));

	OnSkillQueueCleared_BPEvent.Broadcast();

	LockInputTemporarily();
}

void APlayerCharacter::ExecuteNextSkillInQueue_UI()
{
	// 1. 큐에 남은 스킬이 없으면 턴 종료
	if (SkillQueue.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== 스킬 큐 실행 완료 ==="));
		EndAction(); // ⬅️ "행동 3"의 턴 종료
		return;
	}

	// 2. 큐의 맨 앞 스킬을 꺼냄 (FIFO)
	USkillBase* SkillToFire = SkillQueue[0];
	SkillQueue.RemoveAt(0);

	// 3. 스킬 발동 (Print String) 및 쿨다운 적용
	if (SkillToFire)
	{
		UE_LOG(LogTemp, Warning, TEXT("SKILL FIRED: %s"), *SkillToFire->SkillName.ToString());

		// (중요) 이미 존재하는 쿨다운 함수를 호출합니다
		ApplySkillCooldown(SkillToFire);
	}

	// 4. 큐에 스킬이 더 남아있다면 0.2초 뒤에 이 함수를 다시 호출
	if (SkillQueue.Num() > 0)
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
		EndAction(); // ⬅️ "행동 3"의 턴 종료
	}
}