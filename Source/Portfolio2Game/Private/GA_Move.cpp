#include "GA_Move.h"
#include "PlayerCharacter.h"    // APlayerCharacter의 EndAction(), bCanAct를 사용
#include "BattleManager.h"      // BattleManager의 좌표 계산 함수 사용
#include "GridDataInterface.h"  // BattleManager의 GridActorRef에서 그리드 크기를 가져오기 위함

UGA_Move::UGA_Move()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// (신규) 어빌리티 태그 설정
	FGameplayTag MoveTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Move"));
	AbilityTags.AddTag(MoveTag);
}

void UGA_Move::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 컴포넌트 가져오기
	ACharacterBase* Character = Cast<ACharacterBase>(ActorInfo->AvatarActor.Get());

	// 디버그 로그
	if (Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Move Try Activate: %s, bCanAct: %d"), *Character->GetName(), Character->bCanAct);
	}

	if (!Character || !Character->bCanAct)
	{
		// 실패했을 때만 턴을 종료해야 함 (게임 멈춤 방지)
		if (Character) Character->EndAction();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABattleManager* BattleManagerRef = Character->BattleManagerRef;
	TScriptInterface<IGridDataInterface> GridInterface = BattleManagerRef ? BattleManagerRef->GridInterface : nullptr;

	if (!BattleManagerRef || !GridInterface)
	{
		Character->EndAction(); // 안전 장치
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 방향 결정 (태그 기반)
	if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Up"))))
		MoveDirection = EGridDirection::Up;
	else if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Down"))))
		MoveDirection = EGridDirection::Down;
	else if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Left"))))
		MoveDirection = EGridDirection::Left;
	else if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Right"))))
		MoveDirection = EGridDirection::Right;
	else
	{
		Character->EndAction();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 목표 인덱스 및 경계 검사
	const int32 CurrentIndex = Character->GridIndex;
	int32 TargetIndex = CurrentIndex;
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(GridInterface.GetObject());
	const int32 GridWidth = IGridDataInterface::Execute_GetGridWidth(GridInterface.GetObject());

	if (GridHeight <= 0 || GridWidth <= 0)
	{
		Character->EndAction();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	switch (MoveDirection)
	{
	case EGridDirection::Up:	TargetIndex = CurrentIndex - 1; break;
	case EGridDirection::Down:	TargetIndex = CurrentIndex + 1; break;
	case EGridDirection::Left:	TargetIndex = CurrentIndex - GridHeight; break;
	case EGridDirection::Right: TargetIndex = CurrentIndex + GridHeight; break;
	}

	// 4. 경계 검사 (실패 시 턴 종료 처리 필수)
	const int32 MaxIndex = (GridWidth * GridHeight) - 1;
	bool bIsValidMove = true;

	if (MoveDirection == EGridDirection::Up && (CurrentIndex % GridHeight == 0)) bIsValidMove = false;
	if (MoveDirection == EGridDirection::Down && (CurrentIndex % GridHeight == GridHeight - 1)) bIsValidMove = false;
	if (TargetIndex < 0 || TargetIndex > MaxIndex) bIsValidMove = false;

	if (!bIsValidMove)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Move: 이동 불가 (맵 경계)"));

		if (APlayerCharacter* Player = Cast<APlayerCharacter>(Character))
		{
			Player->bHasCommittedAction = false; // ★ 잠금 해제!
		}
		// 적이라면 강제 턴 종료 (게임 멈춤 방지)
		else
		{
			Character->EndAction();
		}

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 5. 점유 검사
	FIntPoint TargetCoord = IGridDataInterface::Execute_GetGridCoordFromIndex(GridInterface.GetObject(), TargetIndex);

	if (BattleManagerRef->GetCharacterAt(TargetCoord) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Move: 이동 불가 (장애물)"));

		if (APlayerCharacter* Player = Cast<APlayerCharacter>(Character))
		{
			Player->bHasCommittedAction = false; // ★ 잠금 해제!
		}
		else
		{
			Character->EndAction();
		}

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 6. 이동 실행 (성공)


	Character->MoveToCell(TargetCoord, TargetIndex);

	// GAS 어빌리티 자체는 여기서 종료해도 됨 (캐릭터의 이동 상태는 Tick에서 관리하므로)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}