// Portfolio2GamePlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
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

	// ❌ (제거) ShortPressThreshold

	// ❌ (제거) FXCursor

	/** * (유지) BP_TestPlayerController가 카메라/하이라이트용으로 사용할 기본 매핑 컨텍스트
	 * (BP_TestPlayerController의 클래스 디폴트에서 할당됨)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// ❌ (제거) SetDestinationClickAction

	// ❌ (제거) SetDestinationTouchAction

protected:
	// ❌ (제거) bMoveToMouseCursor

	virtual void SetupInputComponent() override;

	// (유지) 기본 매핑 컨텍스트를 추가하기 위해 BeginPlay는 남겨둡니다.
	virtual void BeginPlay() override;

	// ❌ (제거) OnInputStarted 및 모든 SetDestination/Touch 관련 함수 선언

private:
	// ❌ (제거) CachedDestination, bIsTouch, FollowTime
};