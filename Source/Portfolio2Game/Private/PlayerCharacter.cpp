#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayAbilitySpec.h"

APlayerCharacter::APlayerCharacter()
{
    bCanAct = false;
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 배틀매니저 찾기 (씬에 존재한다고 가정)
    BattleManagerRef = Cast<ABattleManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // 이후 PlayerController에서 입력 연결 예정
}

void APlayerCharacter::EnableAction(bool bEnable)
{
    bCanAct = bEnable;
}

void APlayerCharacter::EndAction()
{
    bCanAct = false;
    if (BattleManagerRef)
    {
        BattleManagerRef->EndPlayerTurn();
    }
}

// ──────────────────────────────
// 이동
// ──────────────────────────────
void APlayerCharacter::HandleMove(FIntPoint TargetCell)
{
    if (!bCanAct) return;

    MoveToCell(TargetCell);
    EndAction();
}

// ──────────────────────────────
// 회전
// ──────────────────────────────
void APlayerCharacter::HandleTurn(bool bRight)
{
    if (!bCanAct) return;

    Turn(bRight);
    EndAction();
}

// ──────────────────────────────
// 스킬 예약
// ──────────────────────────────
void APlayerCharacter::HandleReserveSkill(TSubclassOf<UGameplayAbility> SkillClass)
{
    if (!bCanAct) return;
    if (!SkillClass) return;

    ReserveSkill(SkillClass);
    EndAction();
}

// ──────────────────────────────
// 예약된 스킬 실행
// ──────────────────────────────
void APlayerCharacter::HandleExecuteSkills()
{
    if (!bCanAct) return;

    ExecuteSkillQueue();
    EndAction();
}

// ──────────────────────────────
// 예약된 스킬 취소
// ──────────────────────────────
void APlayerCharacter::HandleCancelSkills()
{
    if (!bCanAct) return;

    CancelSkillQueue();
    // 스킬 취소는 턴 진행 없음
}
