// (오류 수정) BattleManager.h가 항상 첫 번째 include여야 합니다.
#include "BattleManager.h" 
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "CharacterBase.h"      // GetCharacterAt, GetWorldLocationForCharacter를 위해 포함
#include "GridDataInterface.h"  // 인터페이스 헤더 포함
#include "Kismet/GameplayStatics.h"

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = false;
	// (수정) 7x5 그리드에 맞게 스폰 위치 재조정 (예: 5, 6열)
	EnemySpawnColumns = { 5, 6 };
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
	// 1. (수정) 에디터에서 GridActorRef가 연결되었는지 확인
	if (!GridActorRef)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 'GridActorRef'가 맵에서 연결되지 않았습니다! 디테일 패널에서 BP_GridISM을 연결하세요."));
		return;
	}

	// 2. (수정) 연결된 액터가 'GridDataInterface'를 구현했는지 확인
	if (!GridActorRef->Implements<UGridDataInterface>())
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 'GridActorRef'(%s)가 'IGridDataInterface'를 구현하지 않았습니다! BP_GridISM의 클래스 세팅을 확인하세요."), *GridActorRef->GetName());
		return;
	}

	CurrentRound = 0;
	TurnCount = 0;
	TurnsSinceSingleEnemy = 0;

	UE_LOG(LogTemp, Warning, TEXT("=== Battle Begin ==="));
	SpawnPlayer();
	StartRound();
}

// ──────────────────────────────
// 플레이어 스폰 (7x5 중앙)
// ──────────────────────────────
void ABattleManager::SpawnPlayer()
{
	if (!PlayerClass || !GridActorRef) return;

	// (수정) 7x5 중앙 좌표 계산 (가정: 7x5)
	const int32 CenterRow = IGridDataInterface::Execute_GetGridHeight(GridActorRef) / 2; // 5 -> 2
	const int32 CenterCol = IGridDataInterface::Execute_GetGridWidth(GridActorRef) / 2;  // 7 -> 3
	FIntPoint SpawnGridPos(CenterCol, CenterRow); // (3, 2)

	// (수정) GetWorldLocation (오프셋 없는 중앙 위치)
	FVector SpawnLoc = GetWorldLocation(SpawnGridPos);
	FRotator SpawnRot = FRotator::ZeroRotator;

	PlayerCharacter = GetWorld()->SpawnActor<APlayerCharacter>(PlayerClass, SpawnLoc, SpawnRot);
	if (PlayerCharacter)
	{
		PlayerCharacter->GridCoord = SpawnGridPos;
		PlayerCharacter->bFacingRight = true;
		PlayerCharacter->InitAttributes();

		UE_LOG(LogTemp, Warning, TEXT("Player spawned at (%d,%d)"), CenterCol, CenterRow);
	}
}

// ──────────────────────────────
// 턴 관리 (기존과 동일)
// ──────────────────────────────
void ABattleManager::StartRound()
{
	CurrentRound++;
	TurnsSinceSingleEnemy = 0;
	UE_LOG(LogTemp, Warning, TEXT("=== ROUND %d START ==="), CurrentRound);
	SpawnEnemiesForRound();
	StartPlayerTurn();
}

void ABattleManager::EndRound()
{
	UE_LOG(LogTemp, Warning, TEXT("=== ROUND %d END ==="), CurrentRound);
}

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

void ABattleManager::StartEnemyTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("Enemy Turn Start"));
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (!Enemy || Enemy->bDead) continue;
		UE_LOG(LogTemp, Warning, TEXT("%s attacks!"), *Enemy->GetName());
		// (참고) EnemyAI 로직은 EnemyCharacter.cpp에 있어야 함
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
	if (!TestEnemyClass || !GridActorRef) return;

	int32 NumToSpawn = FMath::RandRange(2, 3);
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(GridActorRef);
	const int32 GridWidth = IGridDataInterface::Execute_GetGridWidth(GridActorRef);

	for (int32 i = 0; i < NumToSpawn; ++i)
	{
		// 7x5 그리드에 맞게 스폰 위치 재조정 (예: 5, 6열)
		int32 Column = EnemySpawnColumns[FMath::RandRange(0, EnemySpawnColumns.Num() - 1)];
		int32 Row = FMath::RandRange(0, GridHeight - 1); // 0~4
		FIntPoint SpawnGridPos(Column, Row);

		// (수정) 스폰하려는 위치가 비어있는지 확인 (겹치기 방지)
		if (GetCharacterAt(SpawnGridPos) != nullptr)
		{
			i--; // 재시도
			continue;
		}

		// (수정) GetWorldLocation (오프셋 없는 중앙 위치)
		FVector SpawnLoc = GetWorldLocation(SpawnGridPos);
		AEnemyCharacter* Enemy = GetWorld()->SpawnActor<AEnemyCharacter>(TestEnemyClass, SpawnLoc, FRotator::ZeroRotator);

		if (Enemy)
		{
			Enemy->GridCoord = SpawnGridPos;
			Enemies.Add(Enemy);
			UE_LOG(LogTemp, Warning, TEXT("Spawned Enemy at (%d,%d)"), Column, Row);
		}
	}
}

// ──────────────────────────────
// 격자 좌표 → 월드 변환 (셀 중심)
// ──────────────────────────────
FVector ABattleManager::GridToWorld(FIntPoint GridPos) const
{
	if (!GridActorRef) return FVector::ZeroVector;

	const double SizeX = IGridDataInterface::Execute_GetGridSizeX(GridActorRef);
	const double SizeY = IGridDataInterface::Execute_GetGridSizeY(GridActorRef);

	return FVector(GridPos.X * SizeX, GridPos.Y * SizeY, 0.f);
}

// ──────────────────────────────
// (수정됨) 중앙 월드 위치
// ──────────────────────────────
FVector ABattleManager::GetWorldLocation(FIntPoint GridPos) const
{
	if (!GridActorRef) return FVector::ZeroVector;

	// (수정) 오프셋 로직 제거. 무조건 중앙 위치 반환.
	return GridToWorld(GridPos);
}

// ──────────────────────────────
// (수정됨) 캐릭터용 헬퍼 함수
// ──────────────────────────────
FVector ABattleManager::GetWorldLocationForCharacter(ACharacterBase* Character) const
{
	if (!Character) return FVector::ZeroVector;

	// (수정) GetWorldLocation에 bool 인수가 필요 없어짐
	return GetWorldLocation(Character->GridCoord);
}

// ──────────────────────────────
// (수정됨) 좌표 <-> 인덱스 변환 유틸리티
// ──────────────────────────────
int32 ABattleManager::GetGridIndexFromCoord(FIntPoint Coord) const
{
	if (!GridActorRef || !GridActorRef->Implements<UGridDataInterface>()) return -1;
	return IGridDataInterface::Execute_GetGridIndexFromCoord(GridActorRef, Coord);
}

FIntPoint ABattleManager::GetGridCoordFromIndex(int32 Index) const
{
	if (!GridActorRef || !GridActorRef->Implements<UGridDataInterface>()) return FIntPoint::ZeroValue;
	return IGridDataInterface::Execute_GetGridCoordFromIndex(GridActorRef, Index);
}

// ──────────────────────────────
// (신규) 겹치기 및 경계 검사 유틸리티
// ──────────────────────────────
bool ABattleManager::IsValidGridCoord(FIntPoint Coord) const
{
	if (!GridActorRef || !GridActorRef->Implements<UGridDataInterface>())
	{
		return false;
	}

	const int32 GridWidth = IGridDataInterface::Execute_GetGridWidth(GridActorRef);
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(GridActorRef);

	return (Coord.X >= 0 && Coord.X < GridWidth && Coord.Y >= 0 && Coord.Y < GridHeight);
}

ACharacterBase* ABattleManager::GetCharacterAt(FIntPoint Coord) const
{
	// 1. 플레이어 확인
	if (PlayerCharacter && !PlayerCharacter->bDead && PlayerCharacter->GridCoord == Coord)
	{
		return PlayerCharacter;
	}

	// 2. 적 배열 확인
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead && Enemy->GridCoord == Coord)
		{
			return Enemy;
		}
	}

	return nullptr; // 해당 위치에 아무도 없음
}


// ──────────────────────────────
// 전투 종료 조건 (기존과 동일)
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
			StartRound();
		}
	}
	else
	{
		TurnsSinceSingleEnemy = 0;
	}
}

void ABattleManager::CheckBattleResult()
{
	int32 AliveEnemies = 0;
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead)
			AliveEnemies++;
	}

	if (!PlayerCharacter || PlayerCharacter->bDead)
	{
		CurrentState = EBattleState::Defeat;
		UE_LOG(LogTemp, Warning, TEXT("Battle Defeat! Player dead."));
		return;
	}

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