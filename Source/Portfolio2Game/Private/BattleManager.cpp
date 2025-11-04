#include "BattleManager.h"
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"

ABattleManager::ABattleManager()
{
    PrimaryActorTick.bCanEverTick = false;

    // 홀수열(적 칸) 등록
    EnemySpawnColumns = { 1, 3, 5, 7, 9, 11, 13 };
}

void ABattleManager::BeginPlay()
{
    Super::BeginPlay();
    BeginBattle();
}

// ──────────────────────────────
// 전투 시작
// ──────────────────────────────
void ABattleManager::BeginBattle()
{
    CurrentRound = 0;
    TurnCount = 0;
    TurnsSinceSingleEnemy = 0;

    UE_LOG(LogTemp, Warning, TEXT("=== Battle Begin ==="));

    // 플레이어 스폰
    SpawnPlayer();

    // 첫 라운드 시작
    StartRound();
}

// ──────────────────────────────
// 플레이어 스폰
// ──────────────────────────────
void ABattleManager::SpawnPlayer()
{
    if (!PlayerClass) return;

    const int32 CenterRow = GridHeight / 2;      // Y=1
    const int32 CenterCol = (GridWidth / 2) - 1; // X=6 (짝수 = 플레이어 칸)

    FVector SpawnLoc = GridToWorld(FIntPoint(CenterCol, CenterRow));
    FRotator SpawnRot = FRotator::ZeroRotator;

    PlayerCharacter = GetWorld()->SpawnActor<APlayerCharacter>(PlayerClass, SpawnLoc, SpawnRot);
    if (PlayerCharacter)
    {
        PlayerCharacter->GridCoord = FIntPoint(CenterCol, CenterRow);
        PlayerCharacter->bFacingRight = true;
        PlayerCharacter->InitAttributes();

        UE_LOG(LogTemp, Warning, TEXT("Player spawned at (%d,%d)"), CenterCol, CenterRow);
    }
}

// ──────────────────────────────
// 라운드 시작
// ──────────────────────────────
void ABattleManager::StartRound()
{
    CurrentRound++;
    TurnsSinceSingleEnemy = 0;

    UE_LOG(LogTemp, Warning, TEXT("=== ROUND %d START ==="), CurrentRound);

    // ✅ 기존 적은 유지한 채 새 적 추가
    SpawnEnemiesForRound();

    StartPlayerTurn();
}

// ──────────────────────────────
// 라운드 종료
// ──────────────────────────────
void ABattleManager::EndRound()
{
    UE_LOG(LogTemp, Warning, TEXT("=== ROUND %d END ==="), CurrentRound);
    // ❌ 기존 적 유지
}

// ──────────────────────────────
// 플레이어 턴
// ──────────────────────────────
void ABattleManager::StartPlayerTurn()
{
    CurrentState = EBattleState::PlayerTurn;
    TurnCount++;

    UE_LOG(LogTemp, Warning, TEXT("Player Turn %d"), TurnCount);

    if (PlayerCharacter)
        PlayerCharacter->EnableAction(true);
}

void ABattleManager::EndPlayerTurn()
{
    CurrentState = EBattleState::EnemyTurn;
    StartEnemyTurn();
}

// ──────────────────────────────
// 적 턴
// ──────────────────────────────
void ABattleManager::StartEnemyTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("Enemy Turn Start"));

    for (AEnemyCharacter* Enemy : Enemies)
    {
        if (!Enemy || Enemy->bDead) continue;
        UE_LOG(LogTemp, Warning, TEXT("%s attacks!"), *Enemy->GetName());
        // 실제 행동 로직은 이후 구현
    }

    EndEnemyTurn();
}

void ABattleManager::EndEnemyTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("Enemy Turn End"));

    CheckSingleEnemyTimer();
    CheckBattleResult();

    if (CurrentState != EBattleState::Victory && CurrentState != EBattleState::Defeat)
        StartPlayerTurn();
}

// ──────────────────────────────
// 적 스폰
// ──────────────────────────────
void ABattleManager::SpawnEnemiesForRound()
{
    if (!TestEnemyClass) return;

    int32 NumToSpawn = FMath::RandRange(2, 3);
    UE_LOG(LogTemp, Warning, TEXT("Round %d: Spawning %d new enemies..."), CurrentRound, NumToSpawn);

    for (int32 i = 0; i < NumToSpawn; ++i)
    {
        int32 Column = EnemySpawnColumns[FMath::RandRange(0, EnemySpawnColumns.Num() - 1)];
        int32 Row = FMath::RandRange(0, GridHeight - 1);

        FVector SpawnLoc = GridToWorld(FIntPoint(Column, Row));
        AEnemyCharacter* Enemy = GetWorld()->SpawnActor<AEnemyCharacter>(TestEnemyClass, SpawnLoc, FRotator::ZeroRotator);

        if (Enemy)
        {
            Enemy->GridCoord = FIntPoint(Column, Row);
            Enemies.Add(Enemy);

            UE_LOG(LogTemp, Warning, TEXT("Spawned Enemy at (%d,%d)"), Column, Row);
        }
    }
}

// ──────────────────────────────
// 격자 좌표 → 월드 변환
// ──────────────────────────────
FVector ABattleManager::GridToWorld(FIntPoint GridPos) const
{
    return FVector(GridPos.X * TileSize, GridPos.Y * TileSize, 0.f);
}

// ──────────────────────────────
// 적 1명 남은 뒤 m턴 카운트
// ──────────────────────────────
void ABattleManager::CheckSingleEnemyTimer()
{
    int32 AliveCount = 0;
    for (AEnemyCharacter* Enemy : Enemies)
    {
        if (Enemy && !Enemy->bDead)
            AliveCount++;
    }

    if (AliveCount == 1)
    {
        TurnsSinceSingleEnemy++;
        UE_LOG(LogTemp, Warning, TEXT("1 enemy left for %d turn(s)"), TurnsSinceSingleEnemy);

        if (TurnsSinceSingleEnemy >= M_TurnsToNextRound && CurrentRound < MaxRoundCount)
        {
            EndRound();
            StartRound(); // ✅ 기존 적 유지, 새 적 추가
        }
    }
    else
    {
        TurnsSinceSingleEnemy = 0;
    }
}

// ──────────────────────────────
// 전투 종료 조건
// ──────────────────────────────
void ABattleManager::CheckBattleResult()
{
    int32 AliveEnemies = 0;
    for (AEnemyCharacter* Enemy : Enemies)
    {
        if (Enemy && !Enemy->bDead)
            AliveEnemies++;
    }

    // 패배 조건
    if (!PlayerCharacter || PlayerCharacter->bDead)
    {
        CurrentState = EBattleState::Defeat;
        UE_LOG(LogTemp, Warning, TEXT("Battle Defeat! Player dead."));
        return;
    }

    // 적 전멸
    if (AliveEnemies == 0)
    {
        if (CurrentRound < MaxRoundCount)
        {
            UE_LOG(LogTemp, Warning, TEXT("Round %d cleared. Next round incoming..."), CurrentRound);
            EndRound();
            StartRound();
        }
        else
        {
            CurrentState = EBattleState::Victory;
            UE_LOG(LogTemp, Warning, TEXT("Battle Victory! All enemies defeated in final round."));
        }
    }
}
