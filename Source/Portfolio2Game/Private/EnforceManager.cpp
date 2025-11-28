#include "EnforceManager.h"
#include "PlayerCharacter.h"
#include "PortfolioGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include <Misc/OutputDeviceNull.h>

AEnforceManager::AEnforceManager() {}

void AEnforceManager::BeginPlay()
{
	Super::BeginPlay();

	// 1. 플레이어 스폰 & 데이터 로드
	SpawnPlayerAndInit();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && StageCamera)
	{
		// 블렌드 시간 0초(즉시), 유지
		PC->SetViewTargetWithBlend(StageCamera, 0.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnforceManager: StageCamera is Missing!"));
	}

	// 2. UI (HUD) 생성
	if (EnforceHUDClass)
	{
		HUDRef = CreateWidget<UUserWidget>(GetWorld(), EnforceHUDClass);
		if (HUDRef)
		{
			HUDRef->AddToViewport();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnforceManager: HUD Class is None!"));
	}

	// 3. 레벨 전환 연출 처리
	UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());
	if (GI && GI->bIsLevelTransitioning)
	{
		GI->bIsLevelTransitioning = false; // 플래그 해제

		if (TransitionWidgetClass)
		{
			// 화면을 덮은 상태로 위젯 생성
			CurrentTransitionWidget = CreateWidget<UUserWidget>(GetWorld(), TransitionWidgetClass);
			if (CurrentTransitionWidget)
			{
				CurrentTransitionWidget->AddToViewport(9999); // 최상단

				// 0.2초 뒤에 걷히기 시작 (로딩 튀는 것 방지)
				FTimerHandle Handle;
				GetWorld()->GetTimerManager().SetTimer(Handle, this, &AEnforceManager::ExecuteUncover, 0.2f, false);
			}
		}
	}

	// 4. 마우스 커서 활성화 (드래그 앤 드롭 필수)
	if (PC)
	{
		// 마우스 커서 보이기
		PC->bShowMouseCursor = true;

		// UI와 게임 모두 입력 가능하도록 설정
		FInputModeGameAndUI InputMode;

		// HUD에 포커스를 줘서 바로 키보드/마우스 반응하게 함
		if (HUDRef)
		{
			InputMode.SetWidgetToFocus(HUDRef->TakeWidget());
		}

		// 마우스가 뷰포트 밖으로 나가지 않게 (선택 사항)
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		// 클릭 시 커서가 사라지는 현상 방지
		InputMode.SetHideCursorDuringCapture(false);

		PC->SetInputMode(InputMode);

		UE_LOG(LogTemp, Warning, TEXT("[EnforceManager] Input Mode Set: ShowMouse=True, GameAndUI"));
	}
}


void AEnforceManager::ExecuteUncover()
{
	if (CurrentTransitionWidget)
	{
		FOutputDeviceNull Ar;
		CurrentTransitionWidget->CallFunctionByNameWithArguments(TEXT("PlayUncover"), Ar, nullptr, true);

		CurrentTransitionWidget = nullptr;
	}
}

void AEnforceManager::SpawnPlayerAndInit()
{
	// 1. 이미 플레이어가 있다면 중복 스폰 방지
	if (PlayerRef) return;

	// 2. 기존 플레이어 정리 (레벨에 PlayerStart로 자동 생성된 폰이 있다면 제거)
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->GetPawn())
	{
		PC->GetPawn()->Destroy();
	}

	// 클래스 설정 확인
	if (!PlayerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("EnforceManager: Player Class is None! Check BP_EnforceManager settings."));
		return;
	}

	// 3. 지연 스폰 (Deferred Spawn) 시작
	// FinishSpawningActor를 호출하기 전까지 BeginPlay가 실행되지 않습니다.
	PlayerRef = GetWorld()->SpawnActorDeferred<APlayerCharacter>(
		PlayerClass,
		PlayerSpawnTransform
	);

	if (PlayerRef)
	{
		// ───────── [여기서 필요한 초기화 수행] ─────────

		// 이동 입력 차단 (강화 맵에서는 캐릭터가 걸어다니면 안 됨)
		// (PlayerCharacter의 Input 함수들은 bCanAct가 false면 작동하지 않음)
		PlayerRef->bCanAct = false;

		// 스폰 마무리 (이 시점에 BeginPlay 호출됨)
		UGameplayStatics::FinishSpawningActor(PlayerRef, PlayerSpawnTransform);

		if (PC)
		{
			PC->Possess(PlayerRef);
		}
	}
}

TArray<FEnforceRewardInfo> AEnforceManager::GetRandomRewards()
{
    TArray<FEnforceRewardInfo> Rewards;

    // 중복 방지를 위한 임시 배열 (원본 보존)
    TArray<UEnforceCardData*> TempEnforce = AllEnforceCards;
    TArray<USkillBase*> TempSkills = AllSkillCards;

    // 3번 뽑기
    for (int32 i = 0; i < 3; ++i)
    {
        FEnforceRewardInfo Info;
        int32 Chance = FMath::RandRange(1, 100);

        // [옵션 1] 신규 스킬 (30%)
        // 단, 스킬 풀에 남은 게 있어야 함
        if (Chance <= 30 && TempSkills.Num() > 0)
        {
            Info.Type = ERewardType::NewSkill;

            // 랜덤 뽑기 & ★목록에서 제거 (중복 방지)
            int32 RandIdx = FMath::RandRange(0, TempSkills.Num() - 1);
            Info.NewSkillData = TempSkills[RandIdx];
            TempSkills.RemoveAt(RandIdx);
        }
        // [옵션 2] 강화 카드 (나머지)
        else if (TempEnforce.Num() > 0)
        {
            Info.Type = ERewardType::EnforceItem;

            // 랜덤 뽑기 & ★목록에서 제거 (중복 방지)
            int32 RandIdx = FMath::RandRange(0, TempEnforce.Num() - 1);
            Info.EnforceData = TempEnforce[RandIdx];
            TempEnforce.RemoveAt(RandIdx);
        }
        // [예외] 강화 카드도 다 떨어졌으면? -> 스킬에서 다시 시도
        else if (TempSkills.Num() > 0)
        {
            Info.Type = ERewardType::NewSkill;
            int32 RandIdx = FMath::RandRange(0, TempSkills.Num() - 1);
            Info.NewSkillData = TempSkills[RandIdx];
            TempSkills.RemoveAt(RandIdx);
        }

        // 유효한 데이터가 있을 때만 추가
        if (Info.EnforceData || Info.NewSkillData)
        {
            Rewards.Add(Info);
        }
    }

    return Rewards;
}

void AEnforceManager::ApplyEnforce(int32 SkillIndex, UEnforceCardData* CardData)
{
	if (!CardData) return;
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

	if (Player->OwnedSkills.IsValidIndex(SkillIndex))
    {
        FPlayerSkillData& Skill = Player->OwnedSkills[SkillIndex];

        Skill.DamageDelta += CardData->AtkEnforceValue;
        Skill.CooldownDelta += CardData->CoolEnforceValue;

        Skill.UpgradeLevel++;
        
        UE_LOG(LogTemp, Warning, TEXT("Skill Enforced! Index: %d"), SkillIndex);

        // 2. ★ [신규] UI 갱신 요청 방송!
        Player->OnSkillDataUpdated.Broadcast(SkillIndex);

        // 3. ★ [수정] 바로 이동하지 않고, 1.5초 뒤에 스테이지 클리어 (결과 확인 시간)
        FTimerHandle WaitHandle;
        GetWorld()->GetTimerManager().SetTimer(WaitHandle, this, &AEnforceManager::CompleteStage, 1.5f, false);
    }
}

void AEnforceManager::AcquireNewSkill(USkillBase* NewSkill)
{
	if (!NewSkill) return;
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

	FPlayerSkillData NewData;
	NewData.SkillInfo = NewSkill;
	NewData.InitializeFromBase();

	int32 NewIndex = Player->OwnedSkills.Add(NewData);

	Player->OwnedSkills.Add(NewData);

	UE_LOG(LogTemp, Warning, TEXT("New Skill Acquired!"));

	Player->OnNewSkillAcquired.Broadcast(NewIndex);
	FTimerHandle WaitHandle;
	GetWorld()->GetTimerManager().SetTimer(WaitHandle, this, &AEnforceManager::CompleteStage, 1.5f, false);
}

void AEnforceManager::CompleteStage()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());

	// 1. 저장
	if (Player && GI && Player->Attributes)
	{
		GI->SavePlayerData(
			Player->Attributes->GetHealth_BP(),
			Player->Attributes->GetMaxHealth_BP(),
			Player->OwnedSkills
		);
		GI->bIsLevelTransitioning = true; // 이동 연출 플래그
	}

	// 2. 화면 덮기 (모래바람)
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

	// 3. 이동
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &AEnforceManager::MoveToNextLevel, 2.6f, false);
}

void AEnforceManager::MoveToNextLevel()
{
	UPortfolioGameInstance* GI = Cast<UPortfolioGameInstance>(GetGameInstance());
	if (GI)
	{
		// ★ GI에게 다음 맵 이름 요청
		FName NextMap = GI->GetNextStageName();

		if (NextMap != NAME_None)
		{
			UGameplayStatics::OpenLevel(this, NextMap);
		}
	}
}