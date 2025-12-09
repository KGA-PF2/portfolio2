// Portfolio2GamePlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "MouseOverGridInterface.h"
#include "Portfolio2GamePlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class APortfolio2GamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APortfolio2GamePlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	virtual void PlayerTick(float DeltaTime) override;

protected:

	virtual void SetupInputComponent() override;

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	TScriptInterface<IMouseOverGridInterface> LastHoveredGrid;
};