#include "BattleManager.h"
#include "PlayerCharacter.h"
#include "GridISM.h"
#include "EnemyCharacter.h"
#include "CharacterBase.h"
#include "GridDataInterface.h"
#include "TimerManager.h"
#include "PortfolioGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include <Misc/OutputDeviceNull.h>

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = false;

	EnemySpawnIndices = { 25, 26, 27, 28, 29, 30, 31, 32, 33, 34 };
	PlayerSpawnIndex = 10;
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();

	// 1. 레벨 전환 연출 (기존 코드 유지)
	UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());
	if (GI && GI->bIsLevelTransitioning)
	{
		GI->bIsLevelTransitioning = false;
		if (TransitionWidgetClass)
		{
			CurrentTransitionWidget = CreateWidget<UUserWidget>(GetWorld(), TransitionWidgetClass);
			if (CurrentTransitionWidget)
			{
				CurrentTransitionWidget->AddToViewport(9999);
				FTimerHandle Handle;
				GetWorld()->GetTimerManager().SetTimer(Handle, this, &ABattleManager::ExecuteUncover, 0.2f, false);
			}
		}
	}

	// 2. 카메라 설정 (타이머 없이 순서 수정)
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		// ★ [핵심] "빙의할 때 카메라 뺏어가지 마!" 옵션 끄기
		PC->bAutoManageActiveCameraTarget = false;

		if (GridActorRef)
		{
			// 이제 바로 설정해도 씹히지 않습니다.
			PC->SetViewTargetWithBlend(GridActorRef, 0.0f);
		}
	}

	// 3. 그리드 인터페이스 캐싱 & UI 생성
	// (GridActorRef가 없어도 UI는 나오게 하기 위해 if문 구조 변경)
	if (GridActorRef)
	{
		GridInterface.SetObject(GridActorRef);
		GridInterface.SetInterface(Cast<IGridDataInterface>(GridActorRef));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GridActorRef is NULL! Camera & Grid Interface setup failed."));
	}

	// 4. 상단 바 생성
	if (TopBarWidgetClass)
	{
		UUserWidget* TopBar = CreateWidget<UUserWidget>(GetWorld(), TopBarWidgetClass);
		if (TopBar) TopBar->AddToViewport(9000);
	}
}

void ABattleManager::ExecuteUncover()
{
	if (CurrentTransitionWidget)
	{
		FOutputDeviceNull Ar;
		CurrentTransitionWidget->CallFunctionByNameWithArguments(TEXT("PlayUncover"), Ar, nullptr, true);

		FTimerHandle KillHandle;
		GetWorld()->GetTimerManager().SetTimer(KillHandle, [this]()
			{
				if (CurrentTransitionWidget)
				{
					CurrentTransitionWidget->RemoveFromParent();
					CurrentTransitionWidget = nullptr;
				}
			}, 2.0f, false);
	}
}

void ABattleManager::BeginBattle()
{
	if (!GridInterface) return;

	// 1. 스테이지 데이터 선택
	if (PossibleStages.Num() > 0)
	{
		int32 RandIdx = FMath::RandRange(0, PossibleStages.Num() - 1);
		CurrentStageData = PossibleStages[RandIdx];
		UE_LOG(LogTemp, Warning, TEXT("Selected Stage: %s"), *CurrentStageData->GetName());
	}

	if (!CurrentStageData)
	{
		UE_LOG(LogTemp, Error, TEXT("No Stage Data Selected! Check PossibleStages."));
		return;
	}

	// 2. 초기화
	CurrentRound = 1;
	CurrentRoundIndex = 0;
	TurnCount = 0;
	TurnsSinceSingleEnemy = 0;

	SpawnPlayer();
	SpawnCurrentRoundEnemies(); // (신규 함수 사용)

	if (PlayerRef)
	{
		CurrentState = EBattleState::PlayerTurn;
		PlayerRef->StartAction();
	}
}

void ABattleManager::SpawnCurrentRoundEnemies()
{
	if (!CurrentStageData) return;
	if (!CurrentStageData->Rounds.IsValidIndex(CurrentRoundIndex)) return;

	const FRoundDef& RoundInfo = CurrentStageData->Rounds[CurrentRoundIndex];

	UE_LOG(LogTemp, Warning, TEXT("=== Spawning Round %d Enemies (%d mobs) ==="),
		CurrentRoundIndex + 1, RoundInfo.EnemiesToSpawn.Num());

	TArray<int32> ValidIndices = EnemySpawnIndices;

	// 이미 있는 적 위치는 제외
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead)
		{
			ValidIndices.Remove(Enemy->GridIndex);
		}
	}

	// 소환 루프
	for (TSubclassOf<AEnemyCharacter> EnemyClassToSpawn : RoundInfo.EnemiesToSpawn)
	{
		if (!EnemyClassToSpawn) continue;
		if (ValidIndices.Num() == 0) break;

		int32 Rnd = FMath::RandRange(0, ValidIndices.Num() - 1);
		int32 SpawnIndex = ValidIndices[Rnd];
		ValidIndices.RemoveAt(Rnd);

		FIntPoint Coord = GetGridCoordFromIndex(SpawnIndex);
		FVector Loc = GetWorldLocation(Coord);

		// Z오프셋 적용
		float ZOffset = 100.f;
		if (EnemyClassToSpawn.GetDefaultObject())
		{
			ZOffset = EnemyClassToSpawn.GetDefaultObject()->SpawnZOffset;
		}
		Loc.Z += ZOffset;

		FRotator SpawnRot = FRotator(0.0f, 180.0f, 0.0f);

		FTransform SpawnTransform(FRotator::ZeroRotator, Loc);

		AEnemyCharacter* NewEnemy = GetWorld()->SpawnActorDeferred<AEnemyCharacter>(
			EnemyClassToSpawn,
			SpawnTransform,
			nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (NewEnemy)
		{
			NewEnemy->GridCoord = Coord;
			NewEnemy->GridIndex = SpawnIndex;
			UGameplayStatics::FinishSpawningActor(NewEnemy, SpawnTransform);
			Enemies.Add(NewEnemy);
		}
	}
}

void ABattleManager::StartNextRound()
{
	if (!CurrentStageData) return;

	// 다음 라운드가 있으면 진행
	if (CurrentStageData->Rounds.IsValidIndex(CurrentRoundIndex + 1))
	{
		Enemies.RemoveAll([](AEnemyCharacter* E) { return E == nullptr || E->bDead; });

		CurrentRoundIndex++;
		CurrentRound++;
		TurnsSinceSingleEnemy = 0;

		UE_LOG(LogTemp, Warning, TEXT(">>> Next Round Started! (Round %d)"), CurrentRound);
		SpawnCurrentRoundEnemies();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No More Rounds."));
	}
}

void ABattleManager::OnEnemyKilled(AEnemyCharacter* DeadEnemy)
{
	CurrentKillCount++;

	if (UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance()))
	{
		GI->TotalKillCount++;
		UE_LOG(LogTemp, Warning, TEXT("Total Game Kills: %d"), GI->TotalKillCount);
	}

	// 살아있는 적 수 계산 (방금 죽은 적 제외)
	int32 AliveCount = 0;
	for (AEnemyCharacter* E : Enemies)
	{
		if (E && !E->bDead && E != DeadEnemy) AliveCount++;
	}

	if (AliveCount == 0)
	{
		bool bHasNextRound = (CurrentStageData && CurrentStageData->Rounds.IsValidIndex(CurrentRoundIndex + 1));

		if (!bHasNextRound)
		{
			// 마지막 라운드까지 다 잡았으면 -> 클리어 대기 (2초 뒤 이동 등)
			FTimerHandle ClearHandle;
			GetWorld()->GetTimerManager().SetTimer(ClearHandle, this, &ABattleManager::ForceStageClear, 2.0f, false);
		}
	}
}

void ABattleManager::CheckSingleEnemyTimer()
{
	int32 AliveCount = 0;
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead) AliveCount++;
	}

	if (AliveCount == 1)
	{
		TurnsSinceSingleEnemy++;
		UE_LOG(LogTemp, Warning, TEXT("1 enemy left for %d turn(s)"), TurnsSinceSingleEnemy);

		// 2턴 지남 & 다음 라운드 존재 시 강제 진행
		if (TurnsSinceSingleEnemy >= 2)
		{
			if (CurrentStageData && CurrentStageData->Rounds.IsValidIndex(CurrentRoundIndex + 1))
			{
				StartNextRound(); // 강제 증원
			}
		}
	}
	else
	{
		TurnsSinceSingleEnemy = 0;
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

void ABattleManager::StartPlayerTurn()
{
	int32 AliveCount = 0;
	for (AEnemyCharacter* E : Enemies)
	{
		if (E && !E->bDead) AliveCount++;
	}
	if (AliveCount == 0 && CurrentStageData && CurrentStageData->Rounds.IsValidIndex(CurrentRoundIndex + 1))
	{
		StartNextRound();
	}


	TurnCount++;
	UE_LOG(LogTemp, Warning, TEXT("TURN %d: PLAYER TURN"), TurnCount);
	CurrentState = EBattleState::PlayerTurn;

	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy) Enemy->HideActionOrder();
	}

	int32 AliveEnemyCount = 0;

	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		AEnemyCharacter* Enemy = Enemies[i];

		if (Enemy && !Enemy->bDead)
		{
			AliveEnemyCount++;
			Enemy->DecideNextAction();

			UTexture2D* SubIcon = nullptr;
			if (OrderManagerRef)
			{
				SubIcon = OrderManagerRef->GetIconForAction(Enemy);
			}

			bool bIsDangerous = (Enemy->ReservedSkill != nullptr) ||
				(Enemy->PendingAction == EAIActionType::FireReserved);

			Enemy->SetActionOrder(AliveEnemyCount, SubIcon, bIsDangerous);
		}
	}

	if (OrderManagerRef)
	{
		OrderManagerRef->UpdateActionQueue(Enemies);
	}

	if (PlayerRef)
	{
		PlayerRef->ReduceCooldowns();
		PlayerRef->OnTurnStart_BPEvent.Broadcast();
		PlayerRef->StartAction();
	}

	
}

void ABattleManager::StartEnemyTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("TURN %d: ENEMY TURN"), TurnCount);
	CurrentState = EBattleState::EnemyTurn;
	CurrentEnemyActionIndex = 0;
	ProcessNextEnemyAction();
}

void ABattleManager::ProcessNextEnemyAction()
{
	if (CurrentEnemyActionIndex >= Enemies.Num())
	{
		CheckSingleEnemyTimer(); // 라운드 체크
		GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, this, &ABattleManager::StartPlayerTurn, 0.2f, false);
		return;
	}

	AEnemyCharacter* CurrentEnemy = Enemies[CurrentEnemyActionIndex];

	if (!CurrentEnemy || CurrentEnemy->bDead)
	{
		CurrentEnemyActionIndex++;
		ProcessNextEnemyAction();
		return;
	}
	CurrentEnemy->StartAction();
	CurrentEnemy->ExecutePlannedAction();
}

void ABattleManager::EndCharacterTurn(ACharacterBase* Character)
{
	CheckBattleResult();
	if (CurrentState == EBattleState::Victory || CurrentState == EBattleState::Defeat) return;

	if (Cast<APlayerCharacter>(Character))
	{
		GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, this, &ABattleManager::StartEnemyTurn, 0.2f, false);
	}
	else if (Cast<AEnemyCharacter>(Character))
	{
		CurrentEnemyActionIndex++;
		GetWorld()->GetTimerManager().SetTimer(TurnDelayHandle, this, &ABattleManager::ProcessNextEnemyAction, 0.2f, false);
	}
}

void ABattleManager::SpawnPlayer_Implementation()
{
	if (!PlayerClass) return;
	if (!GridInterface) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (PC && PC->GetPawn()) PC->GetPawn()->Destroy();
	if (PlayerRef) PlayerRef->Destroy();

	FIntPoint SpawnCoord = GetGridCoordFromIndex(PlayerSpawnIndex);
	FVector SpawnLocation = GetWorldLocation(SpawnCoord);
	SpawnLocation.Z += (PlayerClass->GetDefaultObject<APlayerCharacter>())->SpawnZOffset;

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	PlayerRef = GetWorld()->SpawnActorDeferred<APlayerCharacter>(
		PlayerClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (PlayerRef)
	{
		PlayerRef->GridCoord = SpawnCoord;
		PlayerRef->GridIndex = PlayerSpawnIndex;

		UGameplayStatics::FinishSpawningActor(PlayerRef, SpawnTransform);

		if (PC) PC->Possess(PlayerRef);
	}
}

void ABattleManager::ForceStageClear()
{
	if (CurrentState == EBattleState::Victory) return;
	EndBattle(true);

	if (PlayerRef && PlayerRef->Attributes)
	{
		UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());
		if (GI)
		{
			float HP = PlayerRef->Attributes->GetHealth_BP();
			float MaxHP = PlayerRef->Attributes->GetMaxHealth_BP();
			GI->SavePlayerData(HP, MaxHP, PlayerRef->OwnedSkills);
			GI->bIsLevelTransitioning = true;
		}
	}

	if (TransitionWidgetClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), TransitionWidgetClass);
		if (Widget)
		{
			Widget->AddToViewport(9999);
			FOutputDeviceNull Ar;
			Widget->CallFunctionByNameWithArguments(TEXT("PlayCover"), Ar, nullptr, true);
		}
	}

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &ABattleManager::MoveToNextLevel, 2.6f, false);
}

void ABattleManager::MoveToNextLevel()
{
	UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());
	if (GI)
	{
		FName NextMap = GI->GetNextStageName();
		if (NextMap != NAME_None)
		{
			UGameplayStatics::OpenLevel(this, NextMap);
		}
	}
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

	// 1. 순수 로컬 좌표 계산 (X * Size, Y * Size)
	const FVector CellLocalPos = GridToWorld(GridPos);

	// 2. 로컬 오프셋 더하기 (미세 조정값)
	const FVector LocalOffset = IGridDataInterface::Execute_GetGridLocationOffset(GridActorRef);
	FVector FinalLocalPos = CellLocalPos + LocalOffset;

	// 3. ★ [핵심] 로컬 좌표 -> 월드 좌표 변환 (TransformPosition)
	// GridActorRef가 회전해 있거나 이동해 있어도, 그 기준으로 변환해 줍니다.
	if (GridActorRef)
	{
		return GridActorRef->GetActorTransform().TransformPosition(FinalLocalPos);
	}

	return FinalLocalPos;
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
	if (PlayerRef && !PlayerRef->bDead && PlayerRef->GridCoord == Coord) return PlayerRef;
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead && Enemy->GridCoord == Coord) return Enemy;
	}
	return nullptr;
}

void ABattleManager::CheckBattleResult()
{
	int32 AliveEnemies = 0;
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->bDead) AliveEnemies++;
	}

	if (PlayerRef && PlayerRef->bDead)
	{
		EndBattle(false);
	}
}