// Copyright Epic Games, Inc. All Rights Reserved.

#include "Portfolio2GameGameMode.h"
#include "Portfolio2GamePlayerController.h"
#include "Portfolio2GameCharacter.h"
#include "UObject/ConstructorHelpers.h"

APortfolio2GameGameMode::APortfolio2GameGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = APortfolio2GamePlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// set default controller to our Blueprinted controller
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if(PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}