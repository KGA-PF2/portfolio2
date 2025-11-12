#include "BattleManager.h" 
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "CharacterBase.h"      
#include "GridDataInterface.h" 
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// (신규) 7x5 (WxH) 세로 우선 인덱스 기본값
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

	GridInterface.SetObject(GridActorRef);
	GridInterface.SetInterface(Cast<IGridDataInterface>(GridActorRef));

	if (!GridInterface)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: GridActorRef(%s)가 GridDataInterface를 구현하지 않았습니다! BP_GridISM 클래스 세팅에서 인터페이스를 추가하세요."), *GridActorRef->GetName());
		return;
	}

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

	SpawnPlayer(); // (수정됨) PlayerStart 스폰 문제를 해결하기 위해 먼저 스폰
	SpawnEnemiesForRound();

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
		// 1. (신규) 쿨타임 먼저 감소
		PlayerRef->ReduceCooldowns();

		// 2. (신규) C++ 이벤트 방송 -> UI가 이 신호를 받음
		PlayerRef->OnTurnStart_BPEvent.Broadcast();
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

	// 전체 입력 잠금 0.3초
	GetWorld()->GetTimerManager().SetTimerForNextTick([this, Character]()
		{
			GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, [this, Character]()
				{
					if (Cast<APlayerCharacter>(Character))
						StartEnemyTurn();
					else
						StartPlayerTurn();
				}, 0.3f, false);
		});
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
// 스폰 관련 (인덱스 기반 및 Possess 수정)
// ──────────────────────────────

void ABattleManager::SpawnPlayer_Implementation()
{
	// (오류 수정) PlayerClass가 None이면 스폰을 중단합니다.
	if (!PlayerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 'Player Class'가 None입니다! 레벨의 BattleManager 디테일 패널에서 'BP_PlayerCharacter'를 할당하세요."));
		return;
	}
	if (!GridInterface) return;

	// (PlayerStart 스폰 문제 해결) 
	// 1. 컨트롤러를 미리 찾습니다.
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// 2. 기존 플레이어(PlayerStart에서 스폰된)가 있다면 파괴
	if (PC && PC->GetPawn())
	{
		PC->GetPawn()->Destroy();
	}
	if (PlayerRef)
	{
		PlayerRef->Destroy();
	}

	// 3. 인덱스(칸 번호)로부터 FIntPoint 좌표 획득
	FIntPoint SpawnCoord = GetGridCoordFromIndex(PlayerSpawnIndex);

	// 4. 좌표로 월드 위치 획득
	FVector SpawnLocation = GetWorldLocation(SpawnCoord);

	// 플레이어 스폰 높이 보정 (BP에서 설정한 값 사용)
	SpawnLocation.Z += (PlayerClass->GetDefaultObject<APlayerCharacter>())->SpawnZOffset;

	// 5. 스폰
	PlayerRef = GetWorld()->SpawnActor<APlayerCharacter>(PlayerClass, SpawnLocation, FRotator::ZeroRotator);
	if (PlayerRef)
	{
		// 6. 캐릭터의 논리적 위치(좌표와 인덱스) 둘 다 설정
		PlayerRef->GridCoord = SpawnCoord;
		PlayerRef->GridIndex = PlayerSpawnIndex;

		// 7. (가장 중요) 컨트롤러가 새로 스폰된 폰에 즉시 빙의(Possess)
		//    이 코드가 없으면 "아무것도 안 나오는" 상태가 됩니다.
		if (PC)
		{
			PC->Possess(PlayerRef);
		}
		if (FieldCamera)
		{
			// SetViewTargetWithBlend의 두 번째 인자(0.f)는 즉시 변경하라는 의미입니다.
			PC->SetViewTargetWithBlend(FieldCamera, 0.f);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BattleManager: 'FieldCamera'가 None입니다! 레벨의 BattleManager 디테일 패널에서 맵에 배치된 카메라를 연결하세요."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: SpawnActor<APlayerCharacter>가 실패했습니다. PlayerClass가 유효한지 확인하세요."));
	}
}

void ABattleManager::SpawnEnemiesForRound_Implementation()
{
	if (!EnemyClass || !GridInterface) return;


	// (수정) EnemySpawnIndices 배열 사용
	for (const int32 SpawnIndex : EnemySpawnIndices)
	{
		FIntPoint SpawnCoord = GetGridCoordFromIndex(SpawnIndex);
		FVector SpawnLocation = GetWorldLocation(SpawnCoord);
		SpawnLocation.Z += (EnemyClass->GetDefaultObject<AEnemyCharacter>())->SpawnZOffset;


		AEnemyCharacter* NewEnemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator);

		if (NewEnemy)
		{
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
	return IGridDataInterface::Execute_GetGridIndexFromCoord(GridActorRef, Coord);
}

FIntPoint ABattleManager::GetGridCoordFromIndex(int32 Index) const
{
	if (!GridInterface) return FIntPoint(-1, -1);
	return IGridDataInterface::Execute_GetGridCoordFromIndex(GridActorRef, Index);
}

ACharacterBase* ABattleManager::GetCharacterAt(FIntPoint Coord) const
{
	if (PlayerRef && !PlayerRef->bDead && PlayerRef->GridCoord == Coord)
	{
		return PlayerRef;
	}

	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead && Enemy->GridCoord == Coord)
		{
			return Enemy;
		}
	}

	return nullptr;
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