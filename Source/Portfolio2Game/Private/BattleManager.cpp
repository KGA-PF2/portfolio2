#include "BattleManager.h"
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "CharacterBase.h"      // (중요!) GetWorldLocationForCharacter를 위해 포함
#include "GridDataInterface.h"  // (중요!) 인터페이스 헤더 포함
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
	// 1. (수정) 에디터에서 GridActorRef가 연결되었는지 확인합니다.
	if (!GridActorRef)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 'GridActorRef'가 맵에서 연결되지 않았습니다! 디테일 패널에서 BP_GroundGrid를 연결하세요."));
		return; // 그리드 없이는 진행 불가
	}

	// 2. (수정) 연결된 액터가 'GridDataInterface'를 구현했는지 확인합니다.
	if (!GridActorRef->Implements<UGridDataInterface>())
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 'GridActorRef'(%s)가 'IGridDataInterface'를 구현하지 않았습니다! BP_GroundGrid의 클래스 세팅을 확인하세요."), *GridActorRef->GetName());
		return;
	}

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
	if (!PlayerClass || !GridActorRef) return; // GridActorRef 유효성 검사

	// (수정) 인터페이스를 통해 BP_GroundGrid의 데이터를 가져옵니다.
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(GridActorRef);
	const int32 GridWidth = IGridDataInterface::Execute_GetGridWidth(GridActorRef);

	const int32 CenterRow = GridHeight / 2;
	const int32 CenterCol = (GridWidth / 2) - 1;
	FIntPoint SpawnGridPos(CenterCol, CenterRow);

	// (수정) GetWorldLocation 함수로 오프셋이 적용된 위치를 가져옵니다. (bIsPlayer = true)
	FVector SpawnLoc = GetWorldLocation(SpawnGridPos, true);
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
// 라운드 시작/종료 (기존과 동일)
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

// ──────────────────────────────
// 턴 관리 (기존과 동일)
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

void ABattleManager::StartEnemyTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("Enemy Turn Start"));
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (!Enemy || Enemy->bDead) continue;
		UE_LOG(LogTemp, Warning, TEXT("%s attacks!"), *Enemy->GetName());
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
	if (!TestEnemyClass || !GridActorRef) return; // GridActorRef 유효성 검사

	int32 NumToSpawn = FMath::RandRange(2, 3);
	UE_LOG(LogTemp, Warning, TEXT("Round %d: Spawning %d new enemies..."), CurrentRound, NumToSpawn);

	// (수정) 인터페이스를 통해 BP_GroundGrid의 데이터를 가져옵니다.
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(GridActorRef);

	for (int32 i = 0; i < NumToSpawn; ++i)
	{
		int32 Column = EnemySpawnColumns[FMath::RandRange(0, EnemySpawnColumns.Num() - 1)];
		int32 Row = FMath::RandRange(0, GridHeight - 1); // BP_GroundGrid의 GridHeight 사용
		FIntPoint SpawnGridPos(Column, Row);

		// (수정) GetWorldLocation 함수로 오프셋이 적용된 위치를 가져옵니다. (bIsPlayer = false)
		FVector SpawnLoc = GetWorldLocation(SpawnGridPos, false);
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

	// (수정) BP_GroundGrid의 GridSizeX/Y 변수를 인터페이스로 가져옵니다.
	const double SizeX = IGridDataInterface::Execute_GetGridSizeX(GridActorRef);
	const double SizeY = IGridDataInterface::Execute_GetGridSizeY(GridActorRef);

	// BP_GridISM의 생성 로직(X*SizeX, Y*SizeY)을 반영합니다.
	return FVector(GridPos.X * SizeX, GridPos.Y * SizeY, 0.f);
}

// ──────────────────────────────
// (신규) 오프셋 적용된 월드 위치
// ──────────────────────────────
FVector ABattleManager::GetWorldLocation(FIntPoint GridPos, bool bIsPlayer) const
{
	if (!GridActorRef) return FVector::ZeroVector;

	// 1. 셀 중심 위치를 가져옵니다.
	FVector CellCenter = GridToWorld(GridPos);

	// 2. (수정) 인터페이스를 통해 BP_GroundGrid의 오프셋 변수 값을 가져옵니다.
	const double Offset = bIsPlayer
		? IGridDataInterface::Execute_GetPlayerCellOffsetX(GridActorRef)
		: IGridDataInterface::Execute_GetEnemyCellOffsetX(GridActorRef);

	// (가정) X축이 좌/우 오프셋 축이라고 가정합니다.
	CellCenter.X += Offset;

	return CellCenter;
}

// ──────────────────────────────
// (신규) 캐릭터용 헬퍼 함수
// ──────────────────────────────
FVector ABattleManager::GetWorldLocationForCharacter(ACharacterBase* Character) const
{
	if (!Character) return FVector::ZeroVector;

	// 캐릭터가 APlayerCharacter 클래스인지 확인
	const bool bIsPlayer = Character->IsA(APlayerCharacter::StaticClass());

	// 캐릭터의 GridCoord와 bIsPlayer 여부를 바탕으로 최종 위치 계산
	return GetWorldLocation(Character->GridCoord, bIsPlayer);
}

// ──────────────────────────────
// (신규) 좌표 <-> 인덱스 변환 유틸리티
// ──────────────────────────────
int32 ABattleManager::GetGridIndexFromCoord(FIntPoint Coord) const
{
	// (수정) C++이 계산하는 대신, BP_GroundGrid에게 인터페이스로 물어봅니다.
	if (!GridActorRef || !GridActorRef->Implements<UGridDataInterface>())
	{
		return -1;
	}
	return IGridDataInterface::Execute_GetGridIndexFromCoord(GridActorRef, Coord);
}

FIntPoint ABattleManager::GetGridCoordFromIndex(int32 Index) const
{
	// (수정) C++이 계산하는 대신, BP_GroundGrid에게 인터페이스로 물어봅니다.
	if (!GridActorRef || !GridActorRef->Implements<UGridDataInterface>())
	{
		return FIntPoint::ZeroValue;
	}
	return IGridDataInterface::Execute_GetGridCoordFromIndex(GridActorRef, Index);
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