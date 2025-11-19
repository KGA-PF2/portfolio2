#include "EnemyCharacter.h"
#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
    bFacingRight = false; // 적은 기본적으로 왼쪽(플레이어 방향)을 바라봄
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    PlayerRef = Cast<APlayerCharacter>(
        UGameplayStatics::GetActorOfClass(GetWorld(), APlayerCharacter::StaticClass()));

    BattleManagerRef = Cast<ABattleManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));
}

// ──────────────────────────────
// 적 턴 행동 (BattleManager에서 호출됨)
// ──────────────────────────────
void AEnemyCharacter::ExecuteAIAction()
{
    if (bDead) return;

    // 1️ 플레이어와 같은 행이면 공격 시도
    if (PlayerRef && PlayerRef->GridCoord.Y == GridCoord.Y)
    {
        AttackNearestPlayer();
    }
    else
    {
        // 2️ 아니면 전진
        MoveUp();
    }
}

// ──────────────────────────────
// 플레이어 공격 (같은 행에서 가장 가까운 전방)
// ──────────────────────────────
void AEnemyCharacter::AttackNearestPlayer()
{
    if (!PlayerRef) return;

    const int Dir = bFacingRight ? 1 : -1;
    const int32 Dist = (PlayerRef->GridCoord.X - GridCoord.X) * Dir;

    // 플레이어가 전방에 있고, 일정 범위 내라면 공격
    if (Dist > 0 && Dist <= 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s attacks %s!"), *GetName(), *PlayerRef->GetName());
        PlayerRef->ApplyDamage(1.0f);
    }
    else
    {
        MoveUp();
    }
}

void AEnemyCharacter::MoveUp()
{
    if (!AbilitySystem) return;

    // 1. 내가 바라보는 방향에 따라 발동할 어빌리티 태그 결정
    // (GA_Move.cpp에서 Ability.Move.Right 태그가 있으면 X+ 방향으로 이동하도록 되어있음)
    FGameplayTag ContainerTag;

    if (bFacingRight)
    {
        // 오른쪽(X+) = 적 기준 전진
        ContainerTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Right"));
    }
    else
    {
        // 왼쪽(X-) = 적 기준 전진 (적이 왼쪽 보고 있을 때)
        ContainerTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Left"));
    }

    // 2. 태그로 어빌리티 발동 시도
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(ContainerTag);

    // 이 함수가 실행되면 GA_Move가 켜지고 -> 거기서 MoveToCell을 호출해서 -> HP바까지 싹 갱신해줌
    AbilitySystem->TryActivateAbilitiesByTag(TagContainer);

    // 로그는 GA_Move 안에서 찍거나, 여기서 단순히 발동 시도 로그만 남김
    // UE_LOG(LogTemp, Warning, TEXT("%s Try Move via GAS"), *GetName());
}
