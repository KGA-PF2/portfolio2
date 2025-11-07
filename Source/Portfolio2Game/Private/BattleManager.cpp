// BattleManager.cpp

#include "BattleManager.h" 
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "CharacterBase.h"      
#include "GridDataInterface.h"  // (필수) 인터페이스 헤더 포함
#include "Kismet/GameplayStatics.h"

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// (신규) 7x5 (WxH) 세로 우선 인덱스 기본값
	// (X=5, Y=0~4) -> 5*5+0 = 25 ~ 5*5+4 = 29
	// (X=6, Y=0~4) -> 6*5+0 = 30 ~ 6*5+4 = 34
	EnemySpawnIndices = { 25, 26, 27, 28, 29, 30, 31, 32, 33, 34 };
	PlayerSpawnIndex = 10; // (X=2, Y=0) -> 2*5+0 = 10
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();

	// 1. (신규) GridActorRef 유효성 검사 및 인터페이스 캐시
	if (!GridActorRef)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 'GridActorRef'가 맵에서 연결되지 않았습니다! 디테일 패널에서 BP_GridISM을 연결하세요."));
		return;
	}

	// GridDataInterface를 구현했는지 확인하고 캐시
	GridInterface.SetObject(GridActorRef);
	GridInterface.SetInterface(Cast<IGridDataInterface>(GridActorRef));

	if (!GridInterface)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: GridActorRef(%s)가 GridDataInterface를 구현하지 않았습니다! BP_GridISM 클래스 세팅에서 인터페이스를 추가하세요."), *GridActorRef->GetName());
		return;
	}

	// 2. 전투 시작
	BeginBattle();
}

// ──────────────────────────────
// 전투 시작
// ──────────────────────────────
void ABattleManager::BeginBattle()
{
	if (!GridInterface)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: GridInterface가 유효하지 않아 BeginBattle을 중단합니다."));
		return;
	}

	CurrentRound = 0;
	TurnCount = 0;
	TurnsSinceSingleEnemy = 0;

	SpawnPlayer();
	SpawnEnemiesForRound(); // 첫 라운드 스폰

	if (PlayerRef)
	{
		CurrentState = EBattleState::PlayerTurn;
		PlayerRef->StartAction();
	}
	else
	{
		CurrentState = EBattleState::None;
	}
}

void ABattleManager::EndBattle(bool bPlayerVictory)
{
	if (bPlayerVictory)
	{
		CurrentState = EBattleState::Victory;
		UE_LOG(LogTemp, Warning, TEXT("BATTLE VICTORY"));
	}
	else
	{
		CurrentState = EBattleState::Defeat;
		UE_LOG(LogTemp, Warning, TEXT("BATTLE DEFEAT"));
	}
}

// ──────────────────────────────
// 라운드/턴 흐름
// ──────────────────────────────
void ABattleManager::StartRound()
{
	CurrentRound++;
	TurnCount = 0;
	TurnsSinceSingleEnemy = 0;
	UE_LOG(LogTemp, Warning, TEXT("ROUND %d START"), CurrentRound);
	SpawnEnemiesForRound();

	CurrentState = EBattleState::PlayerTurn;
	if (PlayerRef)
	{
		PlayerRef->StartAction();
	}
}

void ABattleManager::EndRound()
{
	UE_LOG(LogTemp, Warning, TEXT("ROUND %d END"), CurrentRound);
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy) Enemy->Destroy();
	}
	Enemies.Empty();
}

void ABattleManager::StartPlayerTurn()
{
	TurnCount++;
	UE_LOG(LogTemp, Warning, TEXT("TURN %d: PLAYER TURN"), TurnCount);
	CurrentState = EBattleState::PlayerTurn;
	if (PlayerRef)
	{
		PlayerRef->StartAction();
	}
}

void ABattleManager::StartEnemyTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("TURN %d: ENEMY TURN"), TurnCount);
	CurrentState = EBattleState::EnemyTurn;
	ExecuteEnemyActions();
	CheckSingleEnemyTimer();
}

void ABattleManager::EndCharacterTurn(ACharacterBase* Character)
{
	CheckBattleResult();
	if (CurrentState == EBattleState::Victory || CurrentState == EBattleState::Defeat)
	{
		return;
	}

	if (Cast<APlayerCharacter>(Character))
	{
		StartEnemyTurn();
	}
	else
	{
		StartPlayerTurn();
	}
}

void ABattleManager::ExecuteEnemyActions()
{
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead)
		{
			Enemy->ExecuteAIAction();
		}
	}
	EndCharacterTurn(nullptr); // 적 턴 일괄 종료
}

// ──────────────────────────────
// 스폰 관련 (인덱스 기반으로 수정됨)
// ──────────────────────────────

void ABattleManager::SpawnPlayer()
{
	if (!PlayerClass || !GridInterface) return;

	if (PlayerRef) PlayerRef->Destroy(); // 기존 플레이어 제거

	// 1. (수정) 인덱스(칸 번호)로부터 FIntPoint 좌표 획득
	FIntPoint SpawnCoord = GetGridCoordFromIndex(PlayerSpawnIndex);

	// 2. 좌표로 월드 위치 획득
	FVector SpawnLocation = GetWorldLocation(SpawnCoord);

	// 3. 스폰
	PlayerRef = GetWorld()->SpawnActor<APlayerCharacter>(PlayerClass, SpawnLocation, FRotator::ZeroRotator);
	if (PlayerRef)
	{
		// 4. (중요) 캐릭터의 논리적 위치(좌표와 인덱스) 둘 다 설정
		PlayerRef->GridCoord = SpawnCoord;
		PlayerRef->GridIndex = PlayerSpawnIndex;
	}
}

void ABattleManager::SpawnEnemiesForRound()
{
	if (!EnemyClass || !GridInterface) return;

	// 기존 적 제거
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy) Enemy->Destroy();
	}
	Enemies.Empty();

	// (수정) EnemySpawnIndices 배열 사용
	for (const int32 SpawnIndex : EnemySpawnIndices)
	{
		// 1. 인덱스 -> 좌표
		FIntPoint SpawnCoord = GetGridCoordFromIndex(SpawnIndex);

		// 2. 좌표 -> 월드 위치
		FVector SpawnLocation = GetWorldLocation(SpawnCoord);

		// 3. 스폰
		AEnemyCharacter* NewEnemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator);
		if (NewEnemy)
		{
			// 4. (중요) 논리적 위치(좌표와 인덱스) 둘 다 설정
			NewEnemy->GridCoord = SpawnCoord;
			NewEnemy->GridIndex = SpawnIndex;
			Enemies.Add(NewEnemy);
		}
	}
}

// ──────────────────────────────
// 유틸리티 (수정됨)
// ──────────────────────────────

FVector ABattleManager::GridToWorld(FIntPoint GridPos) const
{
	if (!GridInterface) return FVector::ZeroVector;

	// BP_GridISM의 변수(사이즈)를 인터페이스를 통해 가져옴
	const double SizeX = IGridDataInterface::Execute_GetGridSizeX(GridActorRef);
	const double SizeY = IGridDataInterface::Execute_GetGridSizeY(GridActorRef);

	return FVector(GridPos.X * SizeX, GridPos.Y * SizeY, 0.f);
}

FVector ABattleManager::GetWorldLocation(FIntPoint GridPos) const
{
	if (!GridInterface) return FVector::ZeroVector;

	// (오류 수정) BP_GridISM의 GridLocationOffset을 사용합니다.
	const FVector ActorOffset = IGridDataInterface::Execute_GetGridLocationOffset(GridActorRef);
	const FVector CellWorldPos = GridToWorld(GridPos);

	return ActorOffset + CellWorldPos;
}

FVector ABattleManager::GetWorldLocationForCharacter(ACharacterBase* Character) const
{
	if (!Character) return FVector::ZeroVector;
	return GetWorldLocation(Character->GridCoord);
}

int32 ABattleManager::GetGridIndexFromCoord(FIntPoint Coord) const
{
	if (!GridInterface) return -1;
	// BP_GridISM의 BP 함수를 직접 호출
	return IGridDataInterface::Execute_GetGridIndexFromCoord(GridActorRef, Coord);
}

FIntPoint ABattleManager::GetGridCoordFromIndex(int32 Index) const
{
	if (!GridInterface) return FIntPoint(-1, -1);
	// BP_GridISM의 BP 함수를 직접 호출
	return IGridDataInterface::Execute_GetGridCoordFromIndex(GridActorRef, Index);
}

ACharacterBase* ABattleManager::GetCharacterAt(FIntPoint Coord) const
{
	// 1. 플레이어 확인
	if (PlayerRef && !PlayerRef->bDead && PlayerRef->GridCoord == Coord)
	{
		return PlayerRef;
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
// 전투 종료 조건
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

	if (PlayerRef && PlayerRef->bDead)
	{
		EndBattle(false); // 패배
	}
	else if (AliveEnemies == 0)
	{
		EndBattle(true); // 승리
	}
}