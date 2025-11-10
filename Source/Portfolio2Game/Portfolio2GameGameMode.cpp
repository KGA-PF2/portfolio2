// Portfolio2GameGameMode.cpp

#include "Portfolio2GameGameMode.h"
#include "Portfolio2GamePlayerController.h"
#include "Portfolio2GameCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h" // (신규) GetActorOfClass를 위해 필요
#include "PlayerCharacter.h" // (신규) APlayerCharacter 참조를 위해 필요
#include "BattleManager.h" // (신규) BattleManager를 찾기 위해 필요

APortfolio2GameGameMode::APortfolio2GameGameMode()
{

	PlayerControllerClass = APortfolio2GamePlayerController::StaticClass();

	DefaultPawnClass = nullptr;
}

/**
 * (신규) 맵의 모든 액터가 BeginPlay를 마친 후 호출됩니다.
 */
void APortfolio2GameGameMode::StartPlay()
{
	Super::StartPlay(); // 부모 함수 먼저 호출

	// (순서 오류 수정) 
	// 맵에 배치된 BattleManager를 찾습니다.
	ABattleManager* BattleManager = Cast<ABattleManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass())
	);

	if (BattleManager)
	{
		// BattleManager의 BeginPlay (GridInterface 캐시)가 실행된 이후이므로,
		// 여기서 BeginBattle을 호출하는 것이 안전합니다.
		BattleManager->BeginBattle();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode: 맵에서 BattleManager를 찾을 수 없습니다! 레벨에 배치했는지 확인하세요."));
	}
}