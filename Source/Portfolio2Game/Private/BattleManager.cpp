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
	for (int32 i = Enemies.Num() - 1; i >= 0; --i)
	{
		if (!Enemies[i] || Enemies[i]->bDead)
		{
			Enemies.RemoveAt(i);
		}
	}
}

void ABattleManager::StartPlayerTurn()
{
	TurnCount++;
	UE_LOG(LogTemp, Warning, TEXT("TURN %d: PLAYER TURN"), TurnCount);
	CurrentState = EBattleState::PlayerTurn;

	// 모든 적에게 "다음 턴에 뭐 할지 미리 생각해놔!" 명령
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead)
		{
			Enemy->DecideNextAction(); // 계산 후 PendingAction에 저장
		}
	}

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
	CurrentEnemyActionIndex = 0;

	// 첫 번째 적부터 실행 시작
	ProcessNextEnemyAction();
}

// 순차 실행
void ABattleManager::ProcessNextEnemyAction()
{
	// 1. 인덱스가 범위를 넘어가면 모든 적 행동 완료 -> 턴 종료
	if (CurrentEnemyActionIndex >= Enemies.Num())
	{
		// 적 턴 끝났으니 플레이어 턴 준비
		CheckSingleEnemyTimer(); // (기존 로직)

		// 바로 넘기거나 딜레이 후 넘김
		GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, this, &ABattleManager::StartPlayerTurn, 0.2f, false);
		return;
	}

	// 2. 현재 순번 적 가져오기
	AEnemyCharacter* CurrentEnemy = Enemies[CurrentEnemyActionIndex];

	// 죽었거나 없으면 다음 놈으로 패스
	if (!CurrentEnemy || CurrentEnemy->bDead)
	{
		CurrentEnemyActionIndex++;
		ProcessNextEnemyAction(); // 재귀 호출
		return;
	}
	CurrentEnemy->StartAction();

	// 3. 행동 실행! (ExecutePlannedAction 호출)
	// 이 함수가 끝나면 적은 EndAction()을 부르고 -> 그게 EndCharacterTurn()을 부름
	CurrentEnemy->ExecutePlannedAction();
}

void ABattleManager::EndCharacterTurn(ACharacterBase* Character)
{
	CheckBattleResult();
	if (CurrentState == EBattleState::Victory || CurrentState == EBattleState::Defeat) return;

	// 누가 턴을 끝냈는지 확인
	if (Cast<APlayerCharacter>(Character))
	{
		// 플레이어가 끝냄 -> 적 턴 시작
		GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, this, &ABattleManager::StartEnemyTurn, 0.2f, false);
	}
	else if (Cast<AEnemyCharacter>(Character))
	{
		// 적 하나가 행동을 끝냄 -> 다음 적 실행!
		CurrentEnemyActionIndex++;

		// 약간의 딜레이(0.2초)를 주고 다음 적 행동 (너무 빠르면 정신없음)
		GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, this, &ABattleManager::ProcessNextEnemyAction, 0.2f, false);
	}
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

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	// 5. 지연 스폰(Deferred Spawn) 사용
	PlayerRef = GetWorld()->SpawnActorDeferred<APlayerCharacter>(
		PlayerClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (PlayerRef)
	{
		// 6. BeginPlay가 실행되기 "전"에 데이터 주입
		PlayerRef->GridCoord = SpawnCoord;
		PlayerRef->GridIndex = PlayerSpawnIndex; // 이제 BeginPlay에서 올바른 인덱스를 참조

		// 스폰 마무리 (이때 BeginPlay 호출됨 -> HP바 갱신 로직 작동)
		UGameplayStatics::FinishSpawningActor(PlayerRef, SpawnTransform);

		// 7. 빙의 (Possess)
		if (PC)
		{
			PC->Possess(PlayerRef);
		}

		// 카메라 설정 (기존 코드 유지)
		if (FieldCamera)
		{
			PC->SetViewTargetWithBlend(FieldCamera, 0.f);
		}
	}
}

void ABattleManager::SpawnEnemiesForRound_Implementation()
{
	if (!EnemyClass || !GridInterface) return;

	TArray<int32> AvailableSpawnIndices = EnemySpawnIndices;
	
	// 이미 있는 적들의 위치를 후보에서 제거
	for (AEnemyCharacter* ExistingEnemy : Enemies)
	{
		if (ExistingEnemy && !ExistingEnemy->bDead)
		{
			AvailableSpawnIndices.Remove(ExistingEnemy->GridIndex);
		}
	}

	int32 NumEnemiesToSpawn = 2;
	int32 MaxSpawns = FMath::Min(NumEnemiesToSpawn, AvailableSpawnIndices.Num());

	for (int32 i = 0; i < MaxSpawns; ++i)
	{
		int32 RandomArrayIndex = FMath::RandRange(0, AvailableSpawnIndices.Num() - 1);
		int32 SpawnIndex = AvailableSpawnIndices[RandomArrayIndex];
		AvailableSpawnIndices.RemoveAt(RandomArrayIndex);

		FIntPoint SpawnCoord = GetGridCoordFromIndex(SpawnIndex);
		FVector SpawnLocation = GetWorldLocation(SpawnCoord);
		float ZOffset = EnemyClass->GetDefaultObject<AEnemyCharacter>()->SpawnZOffset;
		SpawnLocation.Z += ZOffset;

		// FTransform 생성
		FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

		AEnemyCharacter* NewEnemy = GetWorld()->SpawnActorDeferred<AEnemyCharacter>(
			EnemyClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (NewEnemy)
		{
			// BeginPlay 실행 "전"에 위치 정보 주입!
			NewEnemy->GridCoord = SpawnCoord;
			NewEnemy->GridIndex = SpawnIndex;

			UGameplayStatics::FinishSpawningActor(NewEnemy, SpawnTransform);

			Enemies.Add(NewEnemy);
		}
	}

	Enemies.RemoveAll([](AEnemyCharacter* E) { return E == nullptr || E->bDead; });
}


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