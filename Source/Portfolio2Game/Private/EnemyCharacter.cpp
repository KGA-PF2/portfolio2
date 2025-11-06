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

    // 1️⃣ 플레이어와 같은 행이면 공격 시도
    if (PlayerRef && PlayerRef->GridCoord.Y == GridCoord.Y)
    {
        AttackNearestPlayer();
    }
    else
    {
        // 2️⃣ 아니면 전진
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

// ──────────────────────────────
// 단순 전진 (격자 기준 1칸)
// ──────────────────────────────
void AEnemyCharacter::MoveUp()
{
    const int Dir = bFacingRight ? 1 : -1;

    GridCoord.X += Dir; // 전방으로 한 칸
    FVector NewLoc = FVector(GridCoord.X * 100.f, GridCoord.Y * 100.f, GetActorLocation().Z);
    SetActorLocation(NewLoc);

    UE_LOG(LogTemp, Warning, TEXT("%s moves to (%d,%d)"), *GetName(), GridCoord.X, GridCoord.Y);
}
