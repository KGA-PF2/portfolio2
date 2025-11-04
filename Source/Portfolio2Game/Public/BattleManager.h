#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleManager.generated.h"

class APlayerCharacter;
class AEnemyCharacter;

UENUM(BlueprintType)
enum class EBattleState : uint8
{
    None,
    PlayerTurn,
    EnemyTurn,
    Victory,
    Defeat
};

UCLASS()
class ABattleManager : public AActor
{
    GENERATED_BODY()

public:
    ABattleManager();

protected:
    virtual void BeginPlay() override;

    // ──────────────────────────────
    // 전투 상태
    // ──────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
    EBattleState CurrentState = EBattleState::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    int32 CurrentRound = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    int32 TurnCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    int32 TurnsSinceSingleEnemy = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    int32 M_TurnsToNextRound = 3; // 적 1명 남은 뒤 m턴 경과 시 다음 라운드

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    int32 MaxRoundCount = 3; // 마지막 라운드 기준

    // ──────────────────────────────
    // 전장 정보
    // ──────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Grid")
    int32 GridWidth = 14;

    UPROPERTY(EditAnywhere, Category = "Grid")
    int32 GridHeight = 3;

    UPROPERTY(EditAnywhere, Category = "Grid")
    float TileSize = 100.f;

    TArray<int32> EnemySpawnColumns;

    // ──────────────────────────────
    // 캐릭터 참조
    // ──────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    TSubclassOf<APlayerCharacter> PlayerClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
    TSubclassOf<AEnemyCharacter> TestEnemyClass;

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    APlayerCharacter* PlayerCharacter;

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TArray<AEnemyCharacter*> Enemies;

public:
    // ──────────────────────────────
    // 전투 루프
    // ──────────────────────────────
    UFUNCTION(BlueprintCallable)
    void BeginBattle();

    UFUNCTION(BlueprintCallable)
    void StartRound();

    UFUNCTION(BlueprintCallable)
    void EndRound();

    UFUNCTION(BlueprintCallable)
    void StartPlayerTurn();

    UFUNCTION(BlueprintCallable)
    void EndPlayerTurn();

    UFUNCTION(BlueprintCallable)
    void StartEnemyTurn();

    UFUNCTION(BlueprintCallable)
    void EndEnemyTurn();

    // ──────────────────────────────
    // 스폰 관련
    // ──────────────────────────────
    UFUNCTION(BlueprintCallable)
    void SpawnPlayer();

    UFUNCTION(BlueprintCallable)
    void SpawnEnemiesForRound();

    // ──────────────────────────────
    // 유틸리티
    // ──────────────────────────────
    FVector GridToWorld(FIntPoint GridPos) const;

protected:
    void CheckSingleEnemyTimer();
    void CheckBattleResult();
};
