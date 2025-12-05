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

	// (유지) 기본 매핑 컨텍스트를 추가하기 위해 BeginPlay는 남겨둡니다.
	virtual void BeginPlay() override;

private:

	UPROPERTY()
	TScriptInterface<IMouseOverGridInterface> LastHoveredGrid;
};